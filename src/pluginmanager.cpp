#include "pluginmanager.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QProcess>
#include <QSaveFile>
#include <QSet>
#include <QSet>
#include <QStandardPaths>
#include <QUrl>

namespace {
bool isNativeExecutable(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QByteArray magic = file.read(4);
    if (magic.size() < 4)
        return false;
    if (magic.startsWith(QByteArray("\x7f" "ELF")))
        return true;
    if (magic == QByteArray::fromHex("feedface") || magic == QByteArray::fromHex("feedfacf")
        || magic == QByteArray::fromHex("cefaedfe") || magic == QByteArray::fromHex("cffaedfe"))
        return true;
    return magic.startsWith("MZ");
}

const QList<PluginManager::ArtCraft> &profiles() {
    static const QList<PluginManager::ArtCraft> list = {
        {QStringLiteral("cadcraft"), QStringLiteral("CADCraft"), QStringLiteral("cadcraft-cli"),
         QStringLiteral("Dessin technique, plans, calques et export DXF/SVG."), {QStringLiteral("dxf"), QStringLiteral("dwg")}},
        {QStringLiteral("gridcraft"), QStringLiteral("GridCraft"), QStringLiteral("gridcraft-cli"),
         QStringLiteral("Tableurs, formules, graphiques et export XLSX/CSV."), {QStringLiteral("xlsx"), QStringLiteral("csv")}},
        {QStringLiteral("soundcraft"), QStringLiteral("SoundCraft"), QStringLiteral("soundcraft-cli"),
         QStringLiteral("Station audio, mixage, automation et bounce."), {QStringLiteral("wav"), QStringLiteral("aiff")}},
        {QStringLiteral("deckcraft"), QStringLiteral("DeckCraft"), QStringLiteral("deckcraft-cli"),
         QStringLiteral("Présentations, slides, thèmes et export PPTX."), {QStringLiteral("pptx")}},
        {QStringLiteral("wordcraft"), QStringLiteral("WordCraft"), QStringLiteral("wordcraft-cli"),
         QStringLiteral("Traitement de texte, styles, tables et export DOCX/PDF."), {QStringLiteral("docx"), QStringLiteral("odt")}},
        {QStringLiteral("photocraft"), QStringLiteral("PhotoCraft"), QStringLiteral("photocraft-cli"),
         QStringLiteral("Édition raster, calques, sélections et filtres."), {QStringLiteral("png"), QStringLiteral("psd")}},
        {QStringLiteral("lightcraft"), QStringLiteral("LightCraft"), QStringLiteral("lightcraft-cli"),
         QStringLiteral("Bibliothèque photo et développement RAW."), {QStringLiteral("jpg"), QStringLiteral("dng")}},
        {QStringLiteral("pdfcraft"), QStringLiteral("PdfCraft"), QStringLiteral("pdfcraft-cli"),
         QStringLiteral("Lecture, édition, rendu et extraction PDF."), {QStringLiteral("pdf")}},
        {QStringLiteral("vectorcraft"), QStringLiteral("VectorCraft"), QStringLiteral("vectorcraft-cli"),
         QStringLiteral("Illustration vectorielle, SVG, EPS et PDF."), {QStringLiteral("svg"), QStringLiteral("eps")}},
        {QStringLiteral("effectcraft"), QStringLiteral("EffectCraft"), QStringLiteral("effectcraft-cli"),
         QStringLiteral("Motion design, compositions et effets animés."), {QStringLiteral("ecproj")}},
        {QStringLiteral("filmcraft"), QStringLiteral("FilmCraft"), QStringLiteral("filmcraft-cli"),
         QStringLiteral("Montage vidéo non linéaire et projets fcproj."), {QStringLiteral("fcproj")}},
        {QStringLiteral("designcraft"), QStringLiteral("DesignCraft"), QStringLiteral("designcraft-cli"),
         QStringLiteral("Mise en page, gabarits et export PDF."), {QStringLiteral("pdf")}},
        {QStringLiteral("figcraft"), QStringLiteral("FigCraft"), QStringLiteral("figcraft-cli"),
         QStringLiteral("Design d’interface et prototypes."), {QStringLiteral("fig")}},
        {QStringLiteral("texturecraft"), QStringLiteral("TextureCraft"), QStringLiteral("texturecraft-cli"),
         QStringLiteral("Textures, matériaux et rendus."), {QStringLiteral("png")}}
    };
    return list;
}
struct DshCapability {
    const char *id;
    const char *name;
    const char *category;
    const char *description;
    bool enabledByDefault;
    bool implemented;
    const char *tool;
    const char *view;
};

const QList<DshCapability> &dshCapabilities() {
    static const QList<DshCapability> list = {
        {"agent-loop", "Boucle agent", "Noyau", "Boucle tool-calling native, rounds et arrêt propre.", true, true, "ultimate_status", "agent"},
        {"sessions", "Sessions", "Noyau", "Conversations persistantes, reprise, historique et recherche.", true, true, "ultimate_agent", "agent"},
        {"tools", "Outils", "Noyau", "Registre d’outils intégrés et dynamiques avec permissions.", true, true, "ultimate_plugins", "plugins"},
        {"fs", "Fichiers", "Noyau", "Lecture, écriture, listage et recherche dans le workspace.", true, true, "ultimate_files", "workspace"},
        {"bash", "Shell", "Noyau", "Terminal PTY réel et exécution contrôlée.", true, true, "ultimate_terminal", "terminal"},
        {"web-search", "Recherche web", "Noyau", "DeepSeek Search et secours DuckDuckGo.", true, true, "web_search", "plugins"},
        {"web-fetch", "Lecture web", "Noyau", "Fetch HTTP borné, texte propre et cache.", true, true, "web_fetch", "plugins"},
        {"mcp", "MCP", "Noyau", "Serveurs MCP stdio/HTTP et outils associés.", true, true, "ultimate_mcp", "mcp"},
        {"skills", "Skills", "Noyau", "Bibliothèque de compétences injectées dans le prompt.", true, true, "ultimate_skills", "agents"},
        {"goals", "Objectifs", "Noyau", "Missions Plan → Exécution → Revue.", true, true, "ultimate_mission", "agents"},
        {"plan", "Plan", "Noyau", "Mode planification avant action.", true, true, "ultimate_status", "agent"},
        {"jobs", "Jobs", "Noyau", "Autopilote, files et reprise.", true, true, "ultimate_status", "autopilot"},
        {"subagents", "Sous-agents", "Noyau", "Rôles planner, builder, reviewer et chercheur.", true, true, "ultimate_mission", "agents"},
        {"schedule", "Planification", "Noyau", "Planifications récurrentes des bots.", true, true, "ultimate_bots", "bots"},
        {"token-meter", "Token meter", "Noyau", "Jauge de contexte et compaction.", true, true, "ultimate_status", "agent"},
        {"compaction", "Compaction", "Noyau", "Compaction du contexte et pruning des outils.", true, true, "ultimate_status", "agent"},
        {"time-context", "Contexte temporel", "Noyau", "Date, heure et espace de travail dans le prompt.", true, true, "ultimate_status", "agent"},
        {"persona", "Persona", "Noyau", "Instructions et profils de comportement.", true, true, "ultimate_agents", "agents"},
        {"ask-user", "Ask user", "Noyau", "Confirmation explicite des actions destructrices.", true, true, "ultimate_status", "agent"},
        {"present", "Présentation", "Expérimental", "Artefacts et rendus de résultats.", false, false, "", "workspace"},
        {"todo", "Todo", "Noyau", "Suivi de tâches dans une mission.", true, true, "ultimate_mission", "agents"},
        {"workflow", "Workflow", "Noyau", "Orchestration multi-étapes native.", true, true, "ultimate_mission", "agents"},
        {"workspace", "Workspace", "Noyau", "Projet, Git et fichiers partagés.", true, true, "ultimate_git", "workspace"},
        {"notes", "Notes", "Noyau", "Vault Markdown, wikilinks, backlinks et graphe.", true, true, "ultimate_notes", "notes"},
        {"memory", "Mémoire", "Noyau", "Mémoire persistante SQLite/FTS5.", true, true, "ultimate_memory", "memory"},
        {"providers", "Providers", "Noyau", "Catalogue, clé DeepSeek et modèle par défaut.", true, true, "ultimate_connections", "connections"},
        {"github", "GitHub", "Noyau", "Connexion gh, dépôts, clonage, issues et pull requests.", true, true, "github_repos", "github"},
        {"4crasy", "4Crasy", "Noyau", "Routeur libre, fallback et API OpenAI-compatible.", true, true, "ultimate_router", "crasy"},
        {"learning", "Apprentissage", "Noyau", "Boucle locale d’auto-apprentissage et de leçons.", true, true, "ultimate_learning", "learning"},
        {"plugins", "Plugins", "Noyau", "Registre natif et manifestes externes.", true, true, "ultimate_plugins", "plugins"},
        {"artcraft", "ArtCraft", "Noyau", "14 CLI créatives exposées comme outils.", true, true, "artcraft_gridcraft", "plugins"},
        {"agent-team", "Agent team", "Expérimental", "Équipe d’agents avec rôles parallèles.", false, false, "", "agents"},
        {"voice-input", "Voice input", "Expérimental", "Entrée vocale et dictée.", false, false, "", "agent"},
        {"auto-review", "Auto review", "Expérimental", "Revue automatique des changements.", false, true, "", "agents"},
        {"cot-translation", "CoT translation", "Expérimental", "Traduction du raisonnement.", false, false, "", "agent"},
        {"ralph", "Ralph", "Expérimental", "Boucle autonome de construction.", false, false, "", "autopilot"},
        {"liveshare", "Live share", "Expérimental", "Partage de session en direct.", false, false, "", "agent"},
        {"plugin-market", "Plugin market", "Expérimental", "Marché de plugins Cordis.", false, false, "", "plugins"},
        {"worktree", "Worktree", "Expérimental", "Isolation Git par worktree.", false, false, "", "workspace"},
        {"forge", "Forge", "Expérimental", "Boucle de forge expérimentale.", false, false, "", "agents"},
        {"image-offload", "Image offload", "Expérimental", "Déport des images hors contexte.", false, false, "", "agent"},
        {"speech-to-text", "Speech to text", "Expérimental", "Transcription audio.", false, false, "", "agent"},
        {"translator", "Translator", "Expérimental", "Traduction expérimentale.", false, false, "", "agent"}
    };
    return list;
}

} // namespace

PluginManager::PluginManager(const QString &configDir, QObject *parent)
    : QObject(parent),
      m_configDir(configDir.isEmpty() ? defaultConfigDir() : configDir) {
    loadSettings();
    refresh();
}

QString PluginManager::defaultConfigDir() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/plugins");
}

QString PluginManager::settingsPath() const {
    return m_configDir + QStringLiteral("/settings.json");
}

QString PluginManager::externalDir() const {
    return m_configDir + QStringLiteral("/manifests");
}

QString PluginManager::artcraftPath() const {
    const QString override = qEnvironmentVariable("ULTIMATE_IAH_ARTCRAFT_DIR");
    if (!override.isEmpty())
        return override;
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/artcraft/bin");
}

QString PluginManager::artcraftSuitePath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/artcraft/suite");
}

QString PluginManager::artcraftVersion() const {
    QFile file(artcraftSuitePath() + QStringLiteral("/package.json"));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object()
        .value(QStringLiteral("version")).toString();
}

int PluginManager::activeCapabilities() const {
    int count = 0;
    for (const QVariant &value : m_capabilities)
        if (value.toMap().value(QStringLiteral("enabled")).toBool()
            && value.toMap().value(QStringLiteral("implemented")).toBool()) ++count;
    return count;
}
int PluginManager::coreCapabilities() const {
    int count = 0;
    for (const QVariant &value : m_capabilities)
        if (value.toMap().value(QStringLiteral("category")).toString() == QLatin1String("Noyau")) ++count;
    return count;
}

int PluginManager::installedCount() const {
    int count = 0;
    for (const QVariant &value : m_plugins)
        if (value.toMap().value(QStringLiteral("installed")).toBool()) ++count;
    return count;
}
int PluginManager::enabledCount() const {
    int count = 0;
    for (const QVariant &value : m_plugins)
        if (value.toMap().value(QStringLiteral("installed")).toBool()
            && value.toMap().value(QStringLiteral("enabled")).toBool()) ++count;
    return count;
}

QString PluginManager::findBinary(const QString &name) const {
    const QFileInfo direct(name);
    if (direct.isAbsolute())
        return direct.isFile() && direct.isExecutable() ? direct.absoluteFilePath() : QString();
    const QStringList dirs = {
        artcraftPath(),
        QDir::homePath() + QStringLiteral("/.dsh/crafts/bin"),
        QDir::homePath() + QStringLiteral("/.local/bin"),
    };
    for (const QString &dir : dirs) {
        const QString candidate = dir + QLatin1Char('/') + name;
        const QFileInfo info(candidate);
        if (info.isFile() && info.isExecutable() && info.size() > 1024 * 1024)
            return info.absoluteFilePath();
    }
    const QString path = QStandardPaths::findExecutable(name);
    return path;
}

QVariantMap PluginManager::buildArtcraft(const ArtCraft &craft) const {
    const QString binary = findBinary(craft.binary);
    QString version;
    QFile versionFile(artcraftPath() + QLatin1Char('/') + craft.id + QStringLiteral(".version.json"));
    if (versionFile.open(QIODevice::ReadOnly)) {
        version = QJsonDocument::fromJson(versionFile.readAll()).object()
                      .value(QStringLiteral("version")).toString();
    }
    return QVariantMap{
        {QStringLiteral("id"), craft.id},
        {QStringLiteral("name"), craft.product},
        {QStringLiteral("kind"), QStringLiteral("artcraft")},
        {QStringLiteral("description"), craft.description},
        {QStringLiteral("version"), version},
        {QStringLiteral("binary"), binary},
        {QStringLiteral("installed"), !binary.isEmpty() && isNativeExecutable(binary)},
        {QStringLiteral("enabled"), !binary.isEmpty() && isNativeExecutable(binary)
             && !m_disabled.contains(QStringLiteral("artcraft:") + craft.id)},
        {QStringLiteral("toolName"), QStringLiteral("artcraft_") + craft.id},
        {QStringLiteral("extensions"), craft.extensions},
        {QStringLiteral("capabilities"), QStringList{QStringLiteral("cli"), QStringLiteral("mcp"), QStringLiteral("headless")}},
    };
}

QVariantMap PluginManager::buildExternal(const QString &path) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QJsonParseError error;
    const QJsonObject root = QJsonDocument::fromJson(file.readAll(), &error).object();
    if (error.error != QJsonParseError::NoError || root.value(QStringLiteral("id")).toString().trimmed().isEmpty())
        return {};
    const QString id = root.value(QStringLiteral("id")).toString().trimmed().toLower();
    const QString command = root.value(QStringLiteral("command")).toString();
    const QString binary = command.isEmpty() ? QString() : findBinary(command);
    return QVariantMap{
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), root.value(QStringLiteral("name")).toString(id)},
        {QStringLiteral("kind"), root.value(QStringLiteral("kind")).toString(QStringLiteral("command"))},
        {QStringLiteral("description"), root.value(QStringLiteral("description")).toString()},
        {QStringLiteral("version"), root.value(QStringLiteral("version")).toString()},
        {QStringLiteral("binary"), binary},
        {QStringLiteral("installed"), !binary.isEmpty()},
        {QStringLiteral("enabled"), !binary.isEmpty() && !m_disabled.contains(QStringLiteral("external:") + id)},
        {QStringLiteral("toolName"), QStringLiteral("plugin_") + id},
        {QStringLiteral("manifest"), path},
        {QStringLiteral("capabilities"), QStringList{QStringLiteral("command")}},
    };
}

void PluginManager::refresh() {
    m_error.clear();
    m_plugins.clear();
    QDir().mkpath(artcraftPath());
    QDir().mkpath(externalDir());
    m_capabilities.clear();
    for (const DshCapability &capability : dshCapabilities()) {
        m_capabilities.append(QVariantMap{
            {QStringLiteral("id"), QString::fromUtf8(capability.id)},
            {QStringLiteral("name"), QString::fromUtf8(capability.name)},
            {QStringLiteral("category"), QString::fromUtf8(capability.category)},
            {QStringLiteral("description"), QString::fromUtf8(capability.description)},
            {QStringLiteral("defaultEnabled"), capability.enabledByDefault},
            {QStringLiteral("enabled"), capability.enabledByDefault && capability.implemented},
            {QStringLiteral("implemented"), capability.implemented},
            {QStringLiteral("tool"), QString::fromUtf8(capability.tool)},
            {QStringLiteral("view"), QString::fromUtf8(capability.view)},
        });
    }
    QSet<QString> knownCrafts;
    for (const ArtCraft &craft : profiles()) {
        knownCrafts.insert(craft.id);
        m_plugins.append(buildArtcraft(craft));
    }
    const QFileInfoList installed = QDir(artcraftPath())
        .entryInfoList(QDir::Files | QDir::Executable, QDir::Name);
    for (const QFileInfo &entry : installed) {
        const QString fileName = entry.fileName();
        if (!fileName.endsWith(QLatin1String("-cli")))
            continue;
        const QString id = fileName.left(fileName.size() - 4);
        if (id.isEmpty() || knownCrafts.contains(id))
            continue;
        knownCrafts.insert(id);
        ArtCraft craft{id, id, fileName,
                       tr("Craft ArtCraft détecté automatiquement."), {}};
        m_plugins.append(buildArtcraft(craft));
    }
    QDir directory(externalDir());
    const QFileInfoList manifests = directory.entryInfoList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);
    for (const QFileInfo &manifest : manifests) {
        const QVariantMap plugin = buildExternal(manifest.absoluteFilePath());
        if (!plugin.isEmpty())
            m_plugins.append(plugin);
    }
    emit changed();
}

void PluginManager::loadSettings() {
    QDir().mkpath(m_configDir);
    QFile file(settingsPath());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    for (const QJsonValue &value : root.value(QStringLiteral("disabled")).toArray())
        m_disabled.insert(value.toString());
}

bool PluginManager::saveSettings() const {
    QDir().mkpath(m_configDir);
    QJsonArray disabled;
    for (const QString &id : m_disabled)
        disabled.append(id);
    QSaveFile file(settingsPath());
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(QJsonObject{{QStringLiteral("disabled"), disabled}}).toJson(QJsonDocument::Indented));
    return file.commit();
}

bool PluginManager::setEnabled(const QString &id, bool enabled) {
    QVariantMap plugin = pluginById(id);
    if (plugin.isEmpty())
        return false;
    const QString kind = plugin.value(QStringLiteral("kind")).toString();
    const QString key = (kind == QLatin1String("artcraft") ? QStringLiteral("artcraft:")
                                                           : QStringLiteral("external:")) + id;
    if (enabled)
        m_disabled.remove(key);
    else
        m_disabled.insert(key);
    if (!saveSettings()) {
        setError(tr("Impossible d'enregistrer l'état du plugin."));
        return false;
    }
    refresh();
    return true;
}

void PluginManager::openPluginFolder() {
    QDir().mkpath(m_configDir);
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_configDir));
}

void PluginManager::openArtcraftInstallGuide() {
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/lznnnnn1998/dsh-artcraft")));
}

QVariantMap PluginManager::pluginById(const QString &id) const {
    for (const QVariant &value : m_plugins) {
        const QVariantMap plugin = value.toMap();
        if (plugin.value(QStringLiteral("id")).toString() == id)
            return plugin;
    }
    return {};
}

QJsonArray PluginManager::toolDefinitions() const {
    QJsonArray out;
    for (const QVariant &value : m_plugins) {
        const QVariantMap plugin = value.toMap();
        if (!plugin.value(QStringLiteral("installed")).toBool()
            || !plugin.value(QStringLiteral("enabled")).toBool())
            continue;
        QJsonObject properties{
            {QStringLiteral("action"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                                                    {QStringLiteral("enum"), QJsonArray{QStringLiteral("commands"), QStringLiteral("run"), QStringLiteral("version")}},
                                                    {QStringLiteral("description"), QStringLiteral("commands: catalog, run: execute CLI args, version: binary version.")}}},
            {QStringLiteral("arguments"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                                                       {QStringLiteral("description"), QStringLiteral("CLI arguments for run/commands. Example: run info out.xlsx")}}},
            {QStringLiteral("timeoutSeconds"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
                                                            {QStringLiteral("minimum"), 5}, {QStringLiteral("maximum"), 180}}},
        };
        QJsonObject function{
            {QStringLiteral("name"), plugin.value(QStringLiteral("toolName")).toString()},
            {QStringLiteral("description"), QStringLiteral("%1 — %2. ArtCraft CLI via Ultimate harness.")
                 .arg(plugin.value(QStringLiteral("name")).toString(),
                      plugin.value(QStringLiteral("description")).toString())},
            {QStringLiteral("parameters"), QJsonObject{{QStringLiteral("type"), QStringLiteral("object")},
                                                        {QStringLiteral("properties"), properties},
                                                        {QStringLiteral("required"), QJsonArray{QStringLiteral("action")}}}},
        };
        out.append(QJsonObject{{QStringLiteral("type"), QStringLiteral("function")},
                               {QStringLiteral("function"), function}});
    }
    return out;
}

bool PluginManager::isPluginTool(const QString &name) const {
    for (const QVariant &value : m_plugins) {
        if (value.toMap().value(QStringLiteral("toolName")).toString() == name)
            return true;
    }
    return false;
}

QJsonObject PluginManager::processResult(const QString &program, const QStringList &arguments,
                                         const QString &workspace, int timeoutMs) const {
    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.setWorkingDirectory(workspace.isEmpty() ? QDir::homePath() : workspace);
    process.start(program, arguments);
    if (!process.waitForStarted(5000))
        return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Plugin introuvable ou non exécutable.")}};
    if (!process.waitForFinished(qBound(5000, timeoutMs, 180000))) {
        process.kill();
        process.waitForFinished(2000);
        return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Délai dépassé.")}};
    }
    const QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QString error = QString::fromUtf8(process.readAllStandardError()).trimmed();
    return QJsonObject{{QStringLiteral("ok"), process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0},
                       {QStringLiteral("exitCode"), process.exitCode()},
                       {QStringLiteral("output"), output.left(30000)},
                       {QStringLiteral("error"), error.left(6000)}};
}

QJsonObject PluginManager::runArtcraft(const QString &craft, const QString &argumentsText,
                                       const QString &workspace, const QString &permission) {
    const QVariantMap plugin = pluginById(craft);
    if (plugin.isEmpty() || !plugin.value(QStringLiteral("installed")).toBool())
        return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Craft introuvable ou non installé.")}};
    if (!plugin.value(QStringLiteral("enabled")).toBool())
        return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Plugin désactivé.")}};
    const QString binary = plugin.value(QStringLiteral("binary")).toString();
    QStringList args = QProcess::splitCommand(argumentsText);
    if (args.isEmpty())
        args << QStringLiteral("commands");
    if (permission == QLatin1String("read-only")
        && !(args.first() == QLatin1String("commands") || args.first() == QLatin1String("--version")))
        return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Action ArtCraft refusée en lecture seule.")}};
    QJsonObject result = processResult(binary, args, workspace, 180000);
    if (result.value(QStringLiteral("ok")).toBool()
        && result.value(QStringLiteral("output")).toString().trimmed().isEmpty()
        && (args.first() == QLatin1String("commands") || args.first() == QLatin1String("--version")))
        result = QJsonObject{{QStringLiteral("ok"), false},
                             {QStringLiteral("error"),
                              tr("Le craft n'a produit aucune sortie (binaire incomplet ou stub).")}};
    result.insert(QStringLiteral("craft"), craft);
    result.insert(QStringLiteral("arguments"), args.join(QLatin1Char(' ')));
    return result;
}

QJsonObject PluginManager::executeTool(const QString &name, const QJsonObject &arguments,
                                       const QString &workspace, const QString &permission) {
    const QString action = arguments.value(QStringLiteral("action")).toString(QStringLiteral("commands"));
    QString argumentsText = arguments.value(QStringLiteral("arguments")).toString();
    if (action == QLatin1String("version"))
        argumentsText = QStringLiteral("--version");
    else if (action == QLatin1String("commands") && argumentsText.trimmed().isEmpty())
        argumentsText = QStringLiteral("commands");
    for (const QVariant &value : m_plugins) {
        const QVariantMap plugin = value.toMap();
        if (plugin.value(QStringLiteral("toolName")).toString() != name)
            continue;
        if (plugin.value(QStringLiteral("kind")).toString() == QLatin1String("artcraft"))
            return runArtcraft(plugin.value(QStringLiteral("id")).toString(), argumentsText, workspace, permission);
        const QString binary = plugin.value(QStringLiteral("binary")).toString();
        if (binary.isEmpty())
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Commande du plugin introuvable.")}};
        QJsonObject result = processResult(binary, QProcess::splitCommand(argumentsText), workspace, 60000);
        result.insert(QStringLiteral("plugin"), plugin.value(QStringLiteral("id")).toString());
        return result;
    }
    return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Outil de plugin inconnu.")}};
}

void PluginManager::installOrUpdateArtcraft(bool update) {
    if (m_artcraftBusy)
        return;
    if (QStandardPaths::findExecutable(QStringLiteral("node")).isEmpty()) {
        setError(tr("Node.js est requis pour installer la suite ArtCraft."));
        return;
    }
    m_artcraftBusy = true;
    m_artcraftUpdate = update;
    m_artcraftTarget.clear();
    m_artcraftStatus = update ? tr("Mise à jour de la suite ArtCraft…")
                              : tr("Complétion des crafts manquants…");
    setError({});
    emit artcraftChanged();

    QDir().mkpath(QFileInfo(artcraftSuitePath()).absolutePath());
    const bool suitePresent = QFileInfo(artcraftSuitePath() + QStringLiteral("/.git")).exists();
    if (!suitePresent) {
        m_artcraftStatus = tr("Téléchargement de la suite ArtCraft…");
        emit artcraftChanged();
        runArtcraftProcess(QStringLiteral("git"),
                           {QStringLiteral("clone"), QStringLiteral("--depth"), QStringLiteral("1"),
                            QStringLiteral("https://github.com/lznnnnn1998/dsh-artcraft"),
                            artcraftSuitePath()}, 0);
    } else if (update) {
        runArtcraftProcess(QStringLiteral("git"),
                           {QStringLiteral("-C"), artcraftSuitePath(),
                            QStringLiteral("pull"), QStringLiteral("--ff-only")}, 0);
    } else {
        startArtcraftNode();
    }
}

void PluginManager::installArtcraftCraft(const QString &id) {
    if (m_artcraftBusy)
        return;
    bool known = false;
    for (const QVariant &value : m_plugins) {
        const QVariantMap plugin = value.toMap();
        if (plugin.value(QStringLiteral("kind")).toString() == QLatin1String("artcraft")
            && plugin.value(QStringLiteral("id")).toString() == id) {
            known = true;
            break;
        }
    }
    if (!known) {
        setError(tr("Craft ArtCraft inconnu : %1.").arg(id));
        return;
    }
    if (QStandardPaths::findExecutable(QStringLiteral("node")).isEmpty()) {
        setError(tr("Node.js est requis pour installer la suite ArtCraft."));
        return;
    }
    m_artcraftBusy = true;
    m_artcraftUpdate = false;
    m_artcraftTarget = id;
    m_artcraftStatus = tr("Installation de %1…").arg(id);
    setError({});
    emit artcraftChanged();
    QDir().mkpath(QFileInfo(artcraftSuitePath()).absolutePath());
    if (!QFileInfo(artcraftSuitePath() + QStringLiteral("/.git")).exists()) {
        runArtcraftProcess(QStringLiteral("git"),
                           {QStringLiteral("clone"), QStringLiteral("--depth"), QStringLiteral("1"),
                            QStringLiteral("https://github.com/lznnnnn1998/dsh-artcraft"),
                            artcraftSuitePath()}, 0);
    } else {
        startArtcraftNode();
    }
}

void PluginManager::startArtcraftNode() {
    m_artcraftStage = 1;
    m_artcraftStatus = m_artcraftTarget.isEmpty()
        ? (m_artcraftUpdate ? tr("Installation des mises à jour…") : tr("Installation des crafts…"))
        : tr("Installation de %1…").arg(m_artcraftTarget);
    emit artcraftChanged();
    QStringList arguments{artcraftSuitePath() + QStringLiteral("/scripts/install-craft.mjs")};
    if (!m_artcraftTarget.isEmpty())
        arguments << m_artcraftTarget;
    arguments << QStringLiteral("--dir") << artcraftPath();
    if (m_artcraftUpdate)
        arguments << QStringLiteral("--force");
    runArtcraftProcess(QStringLiteral("node"), arguments, 1);
}

void PluginManager::runArtcraftProcess(const QString &program, const QStringList &arguments, int stage) {
    m_artcraftProcess = new QProcess(this);
    m_artcraftProcess->setProcessChannelMode(QProcess::MergedChannels);
    m_artcraftProcess->setWorkingDirectory(QFileInfo(artcraftSuitePath()).absolutePath());
    connect(m_artcraftProcess, &QProcess::finished, this,
            [this, stage](int code, QProcess::ExitStatus status) {
                QProcess *process = m_artcraftProcess;
                const QString output = process
                    ? QString::fromUtf8(process->readAll()).trimmed() : QString();
                if (process)
                    process->deleteLater();
                m_artcraftProcess = nullptr;
                if (status != QProcess::NormalExit || code != 0) {
                    finishArtcraft(tr("Échec ArtCraft : %1").arg(output.right(400)), false);
                    return;
                }
                if (stage == 0) {
                    startArtcraftNode();
                    return;
                }
                int installed = 0;
                for (const QVariant &value : m_plugins) {
                    const QVariantMap plugin = value.toMap();
                    if (plugin.value(QStringLiteral("kind")).toString() == QLatin1String("artcraft")
                        && plugin.value(QStringLiteral("installed")).toBool())
                        ++installed;
                }
                finishArtcraft(tr("Suite ArtCraft %1 · %2 crafts installés")
                                   .arg(artcraftVersion().isEmpty() ? QStringLiteral("locale")
                                                                    : artcraftVersion())
                                   .arg(installed), true);
            });
    m_artcraftProcess->start(program, arguments);
    if (!m_artcraftProcess->waitForStarted(2000))
        finishArtcraft(tr("Impossible de lancer %1.").arg(program), false);
}

void PluginManager::finishArtcraft(const QString &status, bool ok) {
    m_artcraftStatus = status;
    m_artcraftBusy = false;
    if (m_artcraftProcess) {
        m_artcraftProcess->deleteLater();
        m_artcraftProcess = nullptr;
    }
    if (!ok)
        setError(status);
    refresh();
    emit artcraftChanged();
}

void PluginManager::setError(const QString &error) {
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}

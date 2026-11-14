#include "notepluginmanager.h"

#include "memorystore.h"
#include "notestore.h"
#include "skillmanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

namespace {
QString firstLine(const QString &text) {
    return text.section(QLatin1Char('\n'), 0, 0).trimmed();
}

QString categoryFor(const QString &id) {
    if (id == QLatin1String("task-extract") || id == QLatin1String("task-sync")
        || id == QLatin1String("taskify") || id == QLatin1String("section-tasks"))
        return NotePluginManager::tr("Tâches");
    if (id == QLatin1String("frontmatter") || id == QLatin1String("toc")
        || id == QLatin1String("link-audit"))
        return NotePluginManager::tr("Qualité");
    if (id == QLatin1String("skill-suggest"))
        return NotePluginManager::tr("Skills");
    if (id == QLatin1String("memory-capture"))
        return NotePluginManager::tr("Mémoire");
    return NotePluginManager::tr("Contexte");
}
} // namespace

NotePluginManager::NotePluginManager(NoteStore *notes, const QString &workspace,
                                     const QString &configDir, QObject *parent)
    : QObject(parent),
      m_notes(notes),
      m_workspace(workspace),
      m_configDir(configDir.isEmpty() ? defaultConfigDir() : configDir) {
    QDir().mkpath(m_configDir);
    loadSettings();
    refresh();
}

QString NotePluginManager::defaultConfigDir() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + QStringLiteral("/note-plugins");
}

QVariantList NotePluginManager::plugins() const { return m_plugins; }

QString NotePluginManager::settingsPath() const {
    return m_configDir + QStringLiteral("/settings.json");
}

void NotePluginManager::loadSettings() {
    QFile file(settingsPath());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    for (auto it = root.constBegin(); it != root.constEnd(); ++it)
        m_enabled.insert(it.key(), it.value().toBool());
}

bool NotePluginManager::saveSettings() const {
    QDir().mkpath(m_configDir);
    QJsonObject root;
    for (auto it = m_enabled.constBegin(); it != m_enabled.constEnd(); ++it)
        root.insert(it.key(), it.value());
    QSaveFile file(settingsPath());
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return file.commit();
}

void NotePluginManager::loadManifests() {
    const QDir directory(m_configDir);
    const QFileInfoList files = directory.entryInfoList({QStringLiteral("*.json")},
                                                        QDir::Files, QDir::Name);
    for (const QFileInfo &file : files) {
        if (file.fileName() == QLatin1String("settings.json"))
            continue;
        QFile manifest(file.absoluteFilePath());
        if (!manifest.open(QIODevice::ReadOnly))
            continue;
        const QJsonObject root = QJsonDocument::fromJson(manifest.readAll()).object();
        Plugin plugin;
        plugin.id = root.value(QStringLiteral("id")).toString().trimmed().toLower();
        plugin.name = root.value(QStringLiteral("name")).toString(plugin.id);
        plugin.description = root.value(QStringLiteral("description")).toString();
        plugin.command = root.value(QStringLiteral("command")).toString();
        plugin.mode = root.value(QStringLiteral("mode")).toString(QStringLiteral("append"));
        plugin.readOnly = root.value(QStringLiteral("readOnly")).toBool(false);
        if (plugin.id.isEmpty() || plugin.command.isEmpty())
            continue;
        if (!m_enabled.contains(plugin.id))
            m_enabled.insert(plugin.id, true);
        plugin.enabled = m_enabled.value(plugin.id, true);
        m_plugins.append(QVariantMap{
            {QStringLiteral("id"), plugin.id},
            {QStringLiteral("name"), plugin.name},
            {QStringLiteral("description"), plugin.description},
            {QStringLiteral("mode"), plugin.mode},
            {QStringLiteral("readOnly"), plugin.readOnly},
            {QStringLiteral("builtin"), false},
            {QStringLiteral("category"), tr("Externe")},
            {QStringLiteral("enabled"), plugin.enabled},
            {QStringLiteral("command"), plugin.command},
        });
    }
}

void NotePluginManager::refresh() {
    m_plugins.clear();
    const QList<Plugin> builtins{
        {QStringLiteral("task-extract"), tr("Extraire les tâches"),
         tr("Transforme chaque case à cocher en note-tâche harness (statut todo)."),
         {}, QStringLiteral("append"), false, true, true},
        {QStringLiteral("task-sync"), tr("Synchroniser les tâches"),
         tr("Passe en terminé les notes-tâches dont la case est cochée."),
         {}, QStringLiteral("append"), false, true, true},
        {QStringLiteral("taskify"), tr("Transformer en tâche"),
         tr("Crée une note-tâche à partir de la note courante et la relie."),
         {}, QStringLiteral("append"), false, true, true},
        {QStringLiteral("section-tasks"), tr("Sections en tâches"),
         tr("Crée une note-tâche par section de niveau 2 de la note."),
         {}, QStringLiteral("append"), false, true, true},
        {QStringLiteral("harness-brief"), tr("Brief harness"),
         tr("Ajoute un brief workspace, modèle et tâches ouvertes à la note."),
         {}, QStringLiteral("append"), false, true, true},
        {QStringLiteral("frontmatter"), tr("Statut automatique"),
         tr("Déduit le statut depuis les cases à cocher et l'écrit dans le frontmatter."),
         {}, QStringLiteral("replace"), false, true, true},
        {QStringLiteral("toc"), tr("Sommaire automatique"),
         tr("Insère ou met à jour un sommaire des titres de la note."),
         {}, QStringLiteral("replace"), false, true, true},
        {QStringLiteral("link-audit"), tr("Audit des liens"),
         tr("Liste les wikilinks qui ne correspondent à aucune note."),
         {}, QStringLiteral("append"), true, true, true},
        {QStringLiteral("skill-suggest"), tr("Skills suggérés"),
         tr("Ajoute les skills du harness les plus pertinents pour cette note."),
         {}, QStringLiteral("append"), true, true, true},
        {QStringLiteral("memory-capture"), tr("Capturer en mémoire"),
         tr("Enregistre l'essentiel de la note dans la mémoire du projet."),
         {}, QStringLiteral("append"), false, true, true},
    };
    for (Plugin plugin : builtins) {
        if (!m_enabled.contains(plugin.id))
            m_enabled.insert(plugin.id, true);
        plugin.enabled = m_enabled.value(plugin.id, true);
        m_plugins.append(QVariantMap{
            {QStringLiteral("id"), plugin.id},
            {QStringLiteral("name"), plugin.name},
            {QStringLiteral("description"), plugin.description},
            {QStringLiteral("mode"), plugin.mode},
            {QStringLiteral("readOnly"), plugin.readOnly},
            {QStringLiteral("builtin"), true},
            {QStringLiteral("category"), categoryFor(plugin.id)},
            {QStringLiteral("enabled"), plugin.enabled},
            {QStringLiteral("command"), QString()},
        });
    }
    loadManifests();
    emit changed();
}

bool NotePluginManager::setEnabled(const QString &id, bool enabled) {
    if (id.isEmpty())
        return false;
    m_enabled.insert(id, enabled);
    for (int i = 0; i < m_plugins.size(); ++i) {
        QVariantMap plugin = m_plugins[i].toMap();
        if (plugin.value(QStringLiteral("id")).toString() != id)
            continue;
        plugin.insert(QStringLiteral("enabled"), enabled);
        m_plugins[i] = plugin;
    }
    saveSettings();
    emit changed();
    return true;
}

QVariantMap NotePluginManager::run(const QString &id, const QString &path,
                                   const QString &mode) {
    setError({});
    setLastOutput({});
    if (!m_notes)
        return {{QStringLiteral("ok"), false},
                {QStringLiteral("error"), tr("Vault indisponible.")}};
    if (path.isEmpty() || !QFileInfo::exists(m_notes->vaultPath() + QLatin1Char('/') + path))
        return {{QStringLiteral("ok"), false},
                {QStringLiteral("error"), tr("Note introuvable.")}};
    for (const QVariant &value : m_plugins) {
        const QVariantMap item = value.toMap();
        if (item.value(QStringLiteral("id")).toString() != id)
            continue;
        if (!item.value(QStringLiteral("enabled")).toBool()) {
            setError(tr("Plugin désactivé : %1.").arg(id));
            return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), m_error}};
        }
        Plugin plugin;
        plugin.id = id;
        plugin.name = item.value(QStringLiteral("name")).toString();
        plugin.mode = mode.isEmpty() ? item.value(QStringLiteral("mode")).toString()
                                     : mode;
        plugin.readOnly = item.value(QStringLiteral("readOnly")).toBool();
        plugin.builtin = item.value(QStringLiteral("builtin")).toBool();
        plugin.command = item.value(QStringLiteral("command")).toString();
        QVariantMap result = plugin.builtin ? runBuiltin(plugin, path)
                                            : runCommand(plugin, path);
        result.insert(QStringLiteral("plugin"), id);
        return result;
    }
    setError(tr("Plugin de note inconnu : %1.").arg(id));
    return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), m_error}};
}

QVariantMap NotePluginManager::runBuiltin(const Plugin &plugin, const QString &path) {
    if (!m_notes)
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Vault indisponible.")}};
    const QString content = m_notes->contentOf(path);
    const QString title = m_notes->selectedNote().value(QStringLiteral("title")).toString();
    const QString workspace = m_workspace;
    const QVariantList notes = m_notes->notes();

    const auto resolveTarget = [&notes](const QString &target) {
        const QString normal = target.trimmed().section('#', 0, 0).section('|', 0, 0).trimmed().toLower();
        if (normal.isEmpty())
            return QString();
        for (const QVariant &value : notes) {
            const QVariantMap note = value.toMap();
            const QString notePath = note.value(QStringLiteral("path")).toString();
            if (note.value(QStringLiteral("title")).toString().toLower() == normal
                || QFileInfo(notePath).completeBaseName().toLower() == normal
                || notePath.toLower() == normal + QStringLiteral(".md")
                || notePath.toLower() == normal)
                return notePath;
        }
        return QString();
    };
    const auto saveTransformed = [&](const QString &updated, const QString &message) {
        if (!m_notes->saveNote(path, updated))
            return QVariantMap{{QStringLiteral("ok"), false},
                               {QStringLiteral("error"), m_notes->error()}};
        setLastOutput(message);
        return QVariantMap{{QStringLiteral("ok"), true},
                           {QStringLiteral("output"), message},
                           {QStringLiteral("path"), path},
                           {QStringLiteral("mode"), QStringLiteral("replace")}};
    };

    QString output;
    int created = 0;
    if (plugin.id == QLatin1String("task-extract")) {
        const QRegularExpression expression(QStringLiteral("(?m)^\\s*[-*]\\s*\\[ \\]\\s+(.+)$"));
        auto matches = expression.globalMatch(content);
        QStringList lines;
        while (matches.hasNext()) {
            const QString text = matches.next().captured(1).trimmed();
            if (text.isEmpty())
                continue;
            const QString taskPath = m_notes->createTaskNote(text.left(60), text, workspace);
            if (!taskPath.isEmpty()) {
                ++created;
                lines << QStringLiteral("- [[%1]]").arg(QFileInfo(taskPath).completeBaseName());
            }
        }
        output = QStringLiteral("## Tâches extraites\n\n%1\n")
                     .arg(lines.isEmpty() ? tr("Aucune case à cocher ouverte.") : lines.join(QLatin1Char('\n')));
        if (created > 0)
            output += tr("\n%n note(s)-tâche créée(s).", "", created);
    } else if (plugin.id == QLatin1String("task-sync")) {
        const QRegularExpression expression(QStringLiteral("(?m)^\\s*[-*]\\s*\\[xX]\\s+(.+)$"));
        auto matches = expression.globalMatch(content);
        QStringList lines;
        while (matches.hasNext()) {
            const QString text = matches.next().captured(1).trimmed();
            const QString prefix = text.left(40).toLower();
            for (const QVariant &value : notes) {
                const QVariantMap note = value.toMap();
                if (note.value(QStringLiteral("path")).toString() == path)
                    continue;
                const QString status = note.value(QStringLiteral("status")).toString();
                if (status != QLatin1String("todo") && status != QLatin1String("doing"))
                    continue;
                const QString noteTitle = note.value(QStringLiteral("title")).toString().toLower();
                if (noteTitle.isEmpty() || !prefix.startsWith(noteTitle.left(40)))
                    continue;
                if (m_notes->setNoteStatus(note.value(QStringLiteral("path")).toString(),
                                           QStringLiteral("done"))) {
                    ++created;
                    lines << QStringLiteral("- [[%1]]").arg(note.value(QStringLiteral("title")).toString());
                }
                break;
            }
        }
        output = QStringLiteral("## Tâches synchronisées\n\n%1\n")
                     .arg(lines.isEmpty() ? tr("Aucune tâche liée à clôturer.") : lines.join(QLatin1Char('\n')));
        if (created > 0)
            output += tr("\n%n tâche(s) passée(s) en terminé.", "", created);
    } else if (plugin.id == QLatin1String("taskify")) {
        const QString taskPath = m_notes->createTaskNote(
            title.isEmpty() ? tr("Tâche depuis une note") : title, content, workspace);
        if (taskPath.isEmpty())
            return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), m_notes->error()}};
        output = tr("## Tâche créée\n\n- [[%1]]\n")
                     .arg(QFileInfo(taskPath).completeBaseName());
        created = 1;
    } else if (plugin.id == QLatin1String("section-tasks")) {
        const QRegularExpression heading(QStringLiteral("^##\\s+(.+)$"), QRegularExpression::MultilineOption);
        QVector<QPair<int, QString>> sections;
        auto matches = heading.globalMatch(content);
        while (matches.hasNext()) {
            const auto match = matches.next();
            sections.append({match.capturedStart(), match.captured(1).trimmed()});
        }
        QStringList links;
        for (int i = 0; i < sections.size(); ++i) {
            const QString name = sections[i].second;
            if (name.compare(tr("Journal"), Qt::CaseInsensitive) == 0
                || name.compare(tr("Demande"), Qt::CaseInsensitive) == 0
                || name.compare(tr("Sommaire"), Qt::CaseInsensitive) == 0)
                continue;
            const int bodyStart = content.indexOf('\n', sections[i].first);
            const int bodyEnd = i + 1 < sections.size() ? sections[i + 1].first : content.size();
            const QString body = content.mid(bodyStart + 1, bodyEnd - bodyStart - 1).trimmed();
            const QString taskPath = m_notes->createTaskNote(name, body, workspace);
            if (!taskPath.isEmpty()) {
                ++created;
                links << QStringLiteral("- [[%1]]").arg(QFileInfo(taskPath).completeBaseName());
            }
        }
        output = QStringLiteral("## Tâches par section\n\n%1\n")
                     .arg(links.isEmpty() ? tr("Aucune section à transformer.") : links.join(QLatin1Char('\n')));
    } else if (plugin.id == QLatin1String("harness-brief")) {
        int open = 0;
        int total = 0;
        for (const QVariant &value : notes) {
            const QVariantMap note = value.toMap();
            ++total;
            const QString status = note.value(QStringLiteral("status")).toString();
            if (status == QLatin1String("todo") || status == QLatin1String("doing"))
                ++open;
        }
        output = tr("## Brief harness\n\n- Workspace : %1\n- Modèle : %2\n- Notes : %3\n- Tâches ouvertes : %4\n")
                     .arg(workspace.isEmpty() ? tr("aucun") : workspace)
                     .arg(QFileInfo(m_workspace).fileName().isEmpty()
                              ? tr("inconnu") : QFileInfo(m_workspace).fileName())
                     .arg(total)
                     .arg(open);
    } else if (plugin.id == QLatin1String("frontmatter")) {
        const QString status = m_notes->selectedNote().value(QStringLiteral("status")).toString();
        if (!status.isEmpty()) {
            output = tr("Statut déjà défini : %1.").arg(status);
            setLastOutput(output);
            return {{QStringLiteral("ok"), true}, {QStringLiteral("output"), output},
                    {QStringLiteral("path"), path}, {QStringLiteral("mode"), QStringLiteral("preview")}};
        }
        const QRegularExpression unchecked(QStringLiteral("(?m)^\\s*[-*]\\s*\\[ \\]\\s+.+$"));
        const QRegularExpression checked(QStringLiteral("(?m)^\\s*[-*]\\s*\\[xX]\\s+.+$"));
        QString inferred;
        if (unchecked.match(content).hasMatch())
            inferred = QStringLiteral("todo");
        else if (checked.match(content).hasMatch())
            inferred = QStringLiteral("done");
        if (inferred.isEmpty()) {
            output = tr("Aucune case à cocher pour déduire un statut.");
            setLastOutput(output);
            return {{QStringLiteral("ok"), true}, {QStringLiteral("output"), output},
                    {QStringLiteral("path"), path}, {QStringLiteral("mode"), QStringLiteral("preview")}};
        }
        QString updated = content;
        if (updated.startsWith(QStringLiteral("---\n"))) {
            const int close = updated.indexOf(QStringLiteral("\n---"), 4);
            if (close > 0)
                updated.insert(close, QStringLiteral("\nstatus: ") + inferred);
        } else {
            updated.prepend(QStringLiteral("---\nstatus: ") + inferred + QStringLiteral("\n---\n\n"));
        }
        return saveTransformed(updated, tr("Statut déduit : %1.").arg(inferred));
    } else if (plugin.id == QLatin1String("toc")) {
        const QRegularExpression heading(QStringLiteral("^(#{1,6})\\s+(.+?)\\s*$"),
                                         QRegularExpression::MultilineOption);
        QStringList entries;
        auto matches = heading.globalMatch(content);
        while (matches.hasNext()) {
            const auto match = matches.next();
            entries << QStringLiteral("%1- [[#%2]]")
                           .arg(QString(match.captured(1).size() - 1, QLatin1Char(' ')),
                                match.captured(2).trimmed());
        }
        if (entries.isEmpty()) {
            output = tr("Aucun titre à indexer.");
            setLastOutput(output);
            return {{QStringLiteral("ok"), true}, {QStringLiteral("output"), output},
                    {QStringLiteral("path"), path}, {QStringLiteral("mode"), QStringLiteral("preview")}};
        }
        const QString toc = QStringLiteral("## Sommaire\n\n") + entries.join(QLatin1Char('\n')) + QStringLiteral("\n");
        QString updated = content;
        const int existing = updated.indexOf(QStringLiteral("## Sommaire"));
        if (existing >= 0) {
            int next = updated.indexOf(QStringLiteral("\n## "), existing + 3);
            if (next < 0)
                next = updated.size();
            updated = updated.left(existing) + toc + updated.mid(next);
        } else {
            int position = 0;
            if (updated.startsWith(QStringLiteral("---\n"))) {
                const int close = updated.indexOf(QStringLiteral("\n---"), 4);
                if (close > 0)
                    position = close + 4;
            }
            updated.insert(position, QStringLiteral("\n") + toc + QStringLiteral("\n"));
        }
        return saveTransformed(updated, tr("Sommaire mis à jour (%1 titres).").arg(entries.size()));
    } else if (plugin.id == QLatin1String("link-audit")) {
        const QRegularExpression link(QStringLiteral("\\[\\[([^\\]|#]+)"));
        auto matches = link.globalMatch(content);
        QStringList missing;
        QSet<QString> seen;
        while (matches.hasNext()) {
            const QString target = matches.next().captured(1).trimmed();
            if (target.isEmpty() || seen.contains(target.toLower()))
                continue;
            seen.insert(target.toLower());
            if (resolveTarget(target).isEmpty())
                missing << QStringLiteral("- [[%1]]").arg(target);
        }
        output = QStringLiteral("## Audit des liens\n\n%1\n")
                     .arg(missing.isEmpty() ? tr("Tous les wikilinks sont résolus.")
                                            : tr("Cibles manquantes :\n%1").arg(missing.join(QLatin1Char('\n'))));
    } else if (plugin.id == QLatin1String("skill-suggest")) {
        QStringList lines;
        if (m_skills) {
            for (const QVariant &value : m_skills->suggest(content.left(4000), 3)) {
                const QVariantMap skill = value.toMap();
                lines << QStringLiteral("- **%1** (`%2`) : %3")
                             .arg(skill.value(QStringLiteral("name")).toString(),
                                  skill.value(QStringLiteral("id")).toString(),
                                  skill.value(QStringLiteral("description")).toString());
            }
        }
        output = QStringLiteral("## Skills suggérés\n\n%1\n")
                     .arg(lines.isEmpty() ? tr("Aucun skill pertinent trouvé.") : lines.join(QLatin1Char('\n')));
    } else if (plugin.id == QLatin1String("memory-capture")) {
        if (!m_memory)
            return {{QStringLiteral("ok"), false},
                    {QStringLiteral("error"), tr("Mémoire indisponible.")}};
        const QString body = content.left(1200).trimmed();
        const int id = m_memory->remember(workspace.isEmpty() ? QStringLiteral("notes") : workspace,
                                          QStringLiteral("note"),
                                          title.isEmpty() ? tr("Note") : title,
                                          body, QStringLiteral("note-plugin"),
                                          false, 3, QStringLiteral("note-plugin"));
        if (id <= 0)
            return {{QStringLiteral("ok"), false},
                    {QStringLiteral("error"), m_memory->error()}};
        output = tr("## Mémoire\n\nNote capturée dans la mémoire du projet (entrée %1).\n").arg(id);
    } else {
        return {{QStringLiteral("ok"), false},
                {QStringLiteral("error"), tr("Plugin intégré inconnu.")}};
    }

    if (!plugin.readOnly) {
        const QString next = plugin.mode == QLatin1String("replace")
            ? output : content + QStringLiteral("\n\n") + output;
        if (!m_notes->saveNote(path, next))
            return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), m_notes->error()}};
    }
    setLastOutput(output);
    return {{QStringLiteral("ok"), true},
            {QStringLiteral("output"), output},
            {QStringLiteral("path"), path},
            {QStringLiteral("created"), created},
            {QStringLiteral("mode"), plugin.mode}};
}

QVariantMap NotePluginManager::runCommand(const Plugin &plugin, const QString &path) {
    const QString absolute = m_notes->vaultPath() + QLatin1Char('/') + path;
    const QString title = m_notes->selectedNote().value(QStringLiteral("title")).toString();
    const QString content = m_notes->contentOf(path);
    QProcess process;
    process.setWorkingDirectory(m_notes->vaultPath());
    process.start(plugin.command, {absolute, title});
    if (!process.waitForStarted(3000))
        return {{QStringLiteral("ok"), false},
                {QStringLiteral("error"), tr("Impossible de lancer le plugin.")}};
    process.write(content.toUtf8());
    process.closeWriteChannel();
    if (!process.waitForFinished(8000)) {
        process.kill();
        process.waitForFinished(1000);
        return {{QStringLiteral("ok"), false},
                {QStringLiteral("error"), tr("Délai dépassé pour le plugin.")}};
    }
    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    const QString error = QString::fromUtf8(process.readAllStandardError()).trimmed();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return {{QStringLiteral("ok"), false},
                {QStringLiteral("error"), error.isEmpty()
                     ? tr("Le plugin a échoué (code %1).").arg(process.exitCode())
                     : error}};
    if (plugin.readOnly) {
        setLastOutput(output);
        return {{QStringLiteral("ok"), true},
                {QStringLiteral("output"), output},
                {QStringLiteral("path"), path},
                {QStringLiteral("mode"), QStringLiteral("preview")}};
    }
    const QString next = plugin.mode == QLatin1String("replace")
        ? output
        : (plugin.mode == QLatin1String("create")
               ? QString()
               : content + QStringLiteral("\\n\\n") + output);
    QString target = path;
    if (plugin.mode == QLatin1String("create")) {
        target = m_notes->createNote(
            firstLine(output).isEmpty() ? plugin.name : firstLine(output), QString());
        if (target.isEmpty())
            return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), m_notes->error()}};
        if (!m_notes->saveNote(target, output))
            return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), m_notes->error()}};
    } else if (!m_notes->saveNote(target, next)) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), m_notes->error()}};
    }
    setLastOutput(output);
    return {{QStringLiteral("ok"), true},
            {QStringLiteral("output"), output},
            {QStringLiteral("path"), target},
            {QStringLiteral("mode"), plugin.mode}};
}

void NotePluginManager::setError(const QString &error) {
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}

void NotePluginManager::setLastOutput(const QString &output) {
    if (m_lastOutput == output)
        return;
    m_lastOutput = output;
    emit lastOutputChanged();
}

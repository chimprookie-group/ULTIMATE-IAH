#include "connectiondoctor.h"

#include "backend.h"
#include "harnessconnections.h"
#include "mausbotmanager.h"
#include "mcpmanager.h"
#include "memorystore.h"
#include "nineroutermanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

ConnectionDoctor::ConnectionDoctor(Backend *backend, HarnessConnections *connections,
                                   NineRouterManager *nineRouter, MausBotManager *mausBot,
                                   McpManager *mcp, MemoryStore *memory, QObject *parent)
    : QObject(parent),
      m_backend(backend),
      m_connections(connections),
      m_nineRouter(nineRouter),
      m_mausBot(mausBot),
      m_mcp(mcp),
      m_memory(memory) {}

void ConnectionDoctor::add(const QString &layer, const QString &label, bool ok,
                           const QString &detail, const QString &hint) {
    m_checks.append(QVariantMap{{"layer", layer}, {"label", label}, {"ok", ok},
                                {"detail", detail}, {"hint", hint}});
    if (ok) ++m_passed; else ++m_failed;
}

void ConnectionDoctor::run() {
    if (!m_backend)
        return;
    m_running = true;
    emit runningChanged();
    m_checks.clear();
    m_passed = 0;
    m_failed = 0;

    const QString config = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    const QString dsh = QStandardPaths::findExecutable(m_backend->dshBinary());
    add(tr("Harness"), tr("Binaire dsh"),
        !dsh.isEmpty(), dsh.isEmpty() ? tr("absent du PATH") : dsh);
    add(tr("Harness"), tr("Processus harness"),
        m_backend->harnessState() == "running" || m_backend->demo(),
        m_backend->harnessState() + " · " + m_backend->harnessDetail());

    const QFileInfo agentEnv(config + "/agent.env");
    const QFileInfo creds(qEnvironmentVariable("DSH_HOME", QDir::homePath() + "/.dsh") + "/.credentials.yaml");
    QFile envFile(agentEnv.absoluteFilePath());
    int envKeys = 0;
    bool hasDeepSeek = false, hasNineRouter = false;
    if (envFile.open(QIODevice::ReadOnly)) {
        for (const QString &line : QString::fromUtf8(envFile.readAll()).split('\n')) {
            if (!line.contains('='))
                continue;
            ++envKeys;
            hasDeepSeek = hasDeepSeek || line.startsWith("DEEPSEEK_API_KEY=");
            hasNineRouter = hasNineRouter || line.startsWith("NINE_ROUTER_API_KEY=");
        }
    }
    const auto envPermissions = agentEnv.permissions();
    const bool privateEnv = !(envPermissions & (QFileDevice::ReadGroup | QFileDevice::WriteGroup
                                                 | QFileDevice::ReadOther | QFileDevice::WriteOther));
    add(tr("Credentials"), tr("Environnement agent"),
        envFile.exists() && envKeys > 0,
        envFile.exists() ? tr("%1 clé(s), %2").arg(envKeys).arg(privateEnv ? tr("privé") : tr("permissions larges"))
                         : tr("agent.env absent"));
    add(tr("Credentials"), tr("Credential store DSH"),
        creds.isFile(),
        creds.isFile() ? creds.absoluteFilePath() : tr("fichier absent"));

    const QVariantList providers = m_connections ? m_connections->providers() : QVariantList{};
    add(tr("Providers"), tr("Providers visibles"),
        !providers.isEmpty(), tr("%n provider(s) configuré(s)", "", providers.size()));
    add(tr("Providers"), tr("Modèle par défaut"),
        m_connections && !m_connections->defaultModel().isEmpty(),
        m_connections ? (m_connections->defaultModel().isEmpty() ? tr("aucun") : m_connections->defaultModel())
                      : tr("indisponible"));
    add(tr("Providers"), tr("Ancien overlay 9Router"),
        !QFileInfo::exists(config + "/nine-router.patch.yml"),
        QFileInfo::exists(config + "/nine-router.patch.yml")
            ? tr("nine-router.patch.yml encore présent : il doit être absorbé")
            : tr("aucun conflit d'overlay"));

    if (m_nineRouter) {
        add(tr("9Router"), tr("Service"), m_nineRouter->running(),
            m_nineRouter->detail() + (m_nineRouter->version().isEmpty() ? "" : " · v" + m_nineRouter->version()));
        add(tr("9Router"), tr("Authentification"),
            m_nineRouter->running() && !m_nineRouter->authRequired(),
            m_nineRouter->authRequired() ? tr("mot de passe du dashboard requis")
                                         : (m_nineRouter->running() ? tr("session active") : tr("service arrêté")));
        add(tr("9Router"), tr("Clé locale"),
            !m_nineRouter->keyMasked().isEmpty(),
            m_nineRouter->keyMasked().isEmpty() ? tr("aucune clé créée") : m_nineRouter->keyMasked());
        add(tr("9Router"), tr("Modèles"),
            !m_nineRouter->models().isEmpty(),
            tr("%n modèle(s)", "", m_nineRouter->models().size()));
        add(tr("9Router"), tr("Clé dans l'environnement terminal"),
            hasNineRouter,
            hasNineRouter ? tr("NINE_ROUTER_API_KEY présent") : tr("relancez l'application après application au harness"));
    }

    if (m_mausBot) {
        add(tr("MausBot"), tr("Service"), m_mausBot->running(), m_mausBot->detail());
        add(tr("MausBot"), tr("Pont MCP harness"), m_mausBot->mcpHarness(),
            m_mausBot->mcpHarness() ? tr("patch installé") : tr("non installé"));
    }

    const QFileInfo memoryPatch(config + "/mcp-memory.patch.yml");
    const QFileInfo memoryScript(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/memory_mcp.py");
    const QString memoryDb = m_memory ? m_memory->databasePath() : QString();
    add(tr("MCP"), tr("Mémoire MCP"),
        memoryPatch.isFile() && memoryScript.isFile() && QFileInfo::exists(memoryDb),
        memoryPatch.isFile() ? tr("patch + serveur + base") : tr("patch mémoire absent"));

    QVariantList customMcp;
    if (m_mcp)
        customMcp = m_mcp->servers();
    add(tr("MCP"), tr("Serveurs personnalisés"),
        true, tr("%n serveur(s) configuré(s)", "", customMcp.size()));

    add(tr("Workspace"), tr("Dossier de travail"),
        QFileInfo(m_backend->workspace()).isDir() && QFileInfo(m_backend->workspace()).isWritable(),
        m_backend->workspace());

    m_running = false;
    m_summary = tr("%1 liaison(s) saine(s), %2 à corriger").arg(m_passed).arg(m_failed);
    emit runningChanged();
    emit summaryChanged();
    emit changed();
}

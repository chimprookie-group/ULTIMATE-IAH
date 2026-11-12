#include "installationdoctor.h"

#include "backend.h"
#include "memorystore.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

InstallationDoctor::InstallationDoctor(Backend *backend, MemoryStore *memory,
                                       QObject *parent)
    : QObject(parent), m_backend(backend), m_memory(memory) {}

QString InstallationDoctor::probe(const QString &program, const QStringList &args) const {
    if (program.isEmpty())
        return {};
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(program, args);
    if (!process.waitForStarted(1500))
        return {};
    process.waitForFinished(2500);
    return QString::fromUtf8(process.readAll()).trimmed().split('\n').value(0).trimmed();
}

void InstallationDoctor::add(const QString &layer, const QString &label, bool ok,
                             const QString &detail, const QString &hint) {
    m_checks.append(QVariantMap{{"layer", layer}, {"label", label}, {"ok", ok},
                                {"detail", detail}, {"hint", hint}});
    if (ok) ++m_passed; else ++m_failed;
}

void InstallationDoctor::check() {
    if (!m_backend)
        return;
    m_running = true;
    emit runningChanged();
    m_checks.clear();
    m_passed = 0;
    m_failed = 0;

    const QString dsh = m_backend->dshBinary();
    const bool dshOk = m_backend->dshAvailable();
    add(tr("Harness"), tr("Runtime DSH"), dshOk,
        dshOk ? dsh + " · " + probe(dsh, {"--version"}) : tr("dsh est introuvable"),
        dshOk ? QString() : tr("Installez le runtime DSH ou utilisez « Installer DSH »."));

    const QString node = QStandardPaths::findExecutable(QStringLiteral("node"));
    add(tr("Dépendances"), tr("Node.js"), !node.isEmpty(),
        node.isEmpty() ? tr("absent") : node + " · " + probe(node, {"--version"}),
        node.isEmpty() ? tr("Lancez « Dépendances système ».") : QString());
    const QString npm = QStandardPaths::findExecutable(QStringLiteral("npm"));
    add(tr("Dépendances"), tr("npm"), !npm.isEmpty(),
        npm.isEmpty() ? tr("absent") : npm + " · " + probe(npm, {"--version"}),
        npm.isEmpty() ? tr("Lancez « Dépendances système ».") : QString());
    const QString python = QStandardPaths::findExecutable(QStringLiteral("python3"));
    add(tr("Dépendances"), tr("Python 3"), !python.isEmpty(),
        python.isEmpty() ? tr("absent") : python + " · " + probe(python, {"--version"}),
        python.isEmpty() ? tr("Lancez « Dépendances système ».") : QString());
    const QString yaml = python.isEmpty() ? QString() : probe(python, {"-c", "import yaml; print(yaml.__version__)"});
    add(tr("Dépendances"), tr("PyYAML"), !yaml.isEmpty(),
        yaml.isEmpty() ? tr("module absent") : yaml,
        yaml.isEmpty() ? tr("Lancez « Dépendances système ».") : QString());
    const bool webengine = QFileInfo::exists(QStringLiteral("/usr/lib/qt6/qml/QtWebEngine"));
    add(tr("Dépendances"), tr("QtWebEngine"), webengine,
        webengine ? tr("module QML installé") : tr("qt6-webengine absent"));
    const QString git = QStandardPaths::findExecutable(QStringLiteral("git"));
    add(tr("Dépendances"), tr("Git"), !git.isEmpty(),
        git.isEmpty() ? tr("absent") : probe(git, {"--version"}));
    const QString pkexec = QStandardPaths::findExecutable(QStringLiteral("pkexec"));
    add(tr("Dépendances"), tr("Polkit / pkexec"), !pkexec.isEmpty(),
        pkexec.isEmpty() ? tr("absent") : pkexec,
        pkexec.isEmpty() ? tr("Installez polkit pour les actions administrateur.") : QString());
    const QString xdgOpen = QStandardPaths::findExecutable(QStringLiteral("xdg-open"));
    add(tr("Dépendances"), tr("xdg-open"), !xdgOpen.isEmpty(),
        xdgOpen.isEmpty() ? tr("absent") : xdgOpen,
        xdgOpen.isEmpty() ? tr("Installez xdg-utils pour ouvrir les liens externes.") : QString());

    const bool portal = QDBusConnection::sessionBus().interface()
        && QDBusConnection::sessionBus().interface()->isServiceRegistered(
            QStringLiteral("org.freedesktop.portal.Desktop"));
    add(tr("Système"), tr("Portail fichiers"), portal,
        portal ? tr("xdg-desktop-portal disponible") : tr("portail absent"),
        portal ? QString() : tr("Installez xdg-desktop-portal pour le choix de dossier."));

    const QString config = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    const QString data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    add(tr("Données"), tr("Dossier de configuration"), QFileInfo(config).isWritable(), config);
    add(tr("Données"), tr("Dossier de données"), QFileInfo(data).isWritable(), data);
    add(tr("MCP"), tr("Mémoire locale"),
        QFileInfo::exists(config + "/mcp-memory.patch.yml")
            && QFileInfo::exists(data + "/memory_mcp.py")
            && m_memory && QFileInfo::exists(m_memory->databasePath()),
        tr("patch, serveur et base"));
    add(tr("Système"), tr("Entrée de bureau"),
        QFileInfo::exists(QStringLiteral("/usr/share/applications/ultimate-hai.desktop")),
        tr("ultimate-hai.desktop"));

    m_running = false;
    m_summary = tr("%1 composant(s) prêt(s), %2 manquant(s)").arg(m_passed).arg(m_failed);
    emit runningChanged();
    emit changed();
}

void InstallationDoctor::runAction(const QString &label, const QString &program,
                                   const QStringList &arguments,
                                   const std::function<void(bool)> &done) {
    if (m_process)
        return;
    if (program.isEmpty()) {
        setError(tr("Commande introuvable."));
        return;
    }
    m_actionLabel = label;
    m_actionLog.clear();
    emit installingChanged();
    emit actionLogChanged();
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this] {
        if (!m_process)
            return;
        m_actionLog += QString::fromUtf8(m_process->readAllStandardOutput());
        if (m_actionLog.size() > 20000)
            m_actionLog = m_actionLog.right(20000);
        emit actionLogChanged();
    });
    connect(m_process, &QProcess::finished, this, [this, done](int code, QProcess::ExitStatus status) {
        const bool ok = status == QProcess::NormalExit && code == 0;
        if (!ok)
            setError(tr("L'action d'installation a échoué (code %1).").arg(code));
        QProcess *process = m_process;
        m_process = nullptr;
        if (process)
            process->deleteLater();
        emit installingChanged();
        if (done)
            done(ok);
        check();
    });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            setError(tr("Impossible de démarrer l'installateur."));
    });
    m_process->start(program, arguments);
}

void InstallationDoctor::installSystemDependencies() {
    const QString pkexec = QStandardPaths::findExecutable(QStringLiteral("pkexec"));
    const QString pacman = QStandardPaths::findExecutable(QStringLiteral("pacman"));
    if (pkexec.isEmpty() || pacman.isEmpty()) {
        setError(tr("pkexec ou pacman est introuvable."));
        return;
    }
    runAction(tr("Installation des dépendances système"), pkexec,
              {pacman, QStringLiteral("-S"), QStringLiteral("--needed"), QStringLiteral("--noconfirm"),
               QStringLiteral("qt6-base"), QStringLiteral("qt6-declarative"),
               QStringLiteral("qt6-webengine"), QStringLiteral("qt6-webchannel"),
               QStringLiteral("qt6-wayland"), QStringLiteral("qt6-svg"),
               QStringLiteral("nodejs"), QStringLiteral("npm"), QStringLiteral("python"),
               QStringLiteral("python-yaml"), QStringLiteral("git"), QStringLiteral("xdg-utils"),
               QStringLiteral("polkit"), QStringLiteral("xdg-desktop-portal")});
}

void InstallationDoctor::installDsh() {
    const QString npm = QStandardPaths::findExecutable(QStringLiteral("npm"));
    if (npm.isEmpty()) {
        setError(tr("npm est requis pour installer DSH."));
        return;
    }
    const QString prefix = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/dsh";
    QDir().mkpath(prefix);
    runAction(tr("Installation de DSH"),
              npm, {QStringLiteral("install"), QStringLiteral("--prefix"), prefix,
                    QStringLiteral("@deepseek-ai/dsh@0.2.2-rc.1")},
              [this, prefix](bool ok) {
                  if (ok)
                      m_backend->setDshBinary(prefix + "/node_modules/.bin/dsh");
              });
}

void InstallationDoctor::setError(const QString &error) {
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}

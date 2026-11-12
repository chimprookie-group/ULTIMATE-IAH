#pragma once

#include <QObject>
#include <QProcess>
#include <QVariantList>
#include <functional>

class Backend;
class MausBotManager;
class MemoryStore;
class NineRouterManager;

// Full-install audit: system DSH runtime, Node/npm/Python/PyYAML, QtWebEngine,
// portal, 9Router, MausBot, memory MCP, desktop entry and writable app data.
// It can perform user-level installs and the existing pkexec dependency action.
class InstallationDoctor : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList checks READ checks NOTIFY changed)
    Q_PROPERTY(int passed READ passed NOTIFY changed)
    Q_PROPERTY(int failed READ failed NOTIFY changed)
    Q_PROPERTY(QString summary READ summary NOTIFY changed)
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(bool installing READ installing NOTIFY changed)
    Q_PROPERTY(QString actionLabel READ actionLabel NOTIFY installingChanged)
    Q_PROPERTY(QString actionLog READ actionLog NOTIFY actionLogChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    InstallationDoctor(Backend *backend, NineRouterManager *nineRouter,
                       MausBotManager *mausBot, MemoryStore *memory,
                       QObject *parent = nullptr);

    QVariantList checks() const { return m_checks; }
    int passed() const { return m_passed; }
    int failed() const { return m_failed; }
    QString summary() const { return m_summary; }
    bool running() const { return m_running; }
    bool installing() const { return m_process != nullptr; }
    QString actionLabel() const { return m_actionLabel; }
    QString actionLog() const { return m_actionLog; }
    QString error() const { return m_error; }

    Q_INVOKABLE void check();
    Q_INVOKABLE void installSystemDependencies();
    Q_INVOKABLE void installDsh();
    Q_INVOKABLE void installNineRouter();
    Q_INVOKABLE void installMausBot();
    Q_INVOKABLE void startServices();

signals:
    void changed();
    void runningChanged();
    void installingChanged();
    void actionLogChanged();
    void errorChanged();

private:
    void add(const QString &layer, const QString &label, bool ok,
             const QString &detail, const QString &hint = {});
    void runAction(const QString &label, const QString &program,
                   const QStringList &arguments, const std::function<void(bool)> &done = {});
    void setError(const QString &error);
    QString probe(const QString &program, const QStringList &args) const;

    Backend *m_backend = nullptr;
    NineRouterManager *m_nineRouter = nullptr;
    MausBotManager *m_mausBot = nullptr;
    MemoryStore *m_memory = nullptr;
    QVariantList m_checks;
    int m_passed = 0;
    int m_failed = 0;
    bool m_running = false;
    QProcess *m_process = nullptr;
    QString m_actionLabel;
    QString m_actionLog;
    QString m_summary;
    QString m_error;
};

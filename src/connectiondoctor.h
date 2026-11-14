#pragma once

#include <QObject>
#include <QVariantList>

class Backend;
class BotManager;
class HarnessConnections;
class MemoryStore;
class McpManager;

// Cross-layer connection health: IAH, env, credentials, provider overlay,
// MCP and workspace. Each check carries an actionable detail.
class ConnectionDoctor : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList checks READ checks NOTIFY changed)
    Q_PROPERTY(int passed READ passed NOTIFY changed)
    Q_PROPERTY(int failed READ failed NOTIFY changed)
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(QString summary READ summary NOTIFY changed)

public:
    ConnectionDoctor(Backend *backend, HarnessConnections *connections,
                     McpManager *mcp, MemoryStore *memory, BotManager *bots,
                     QObject *parent = nullptr);

    QVariantList checks() const { return m_checks; }
    int passed() const { return m_passed; }
    int failed() const { return m_failed; }
    bool running() const { return m_running; }
    QString summary() const { return m_summary; }

    Q_INVOKABLE void run();

signals:
    void changed();
    void runningChanged();
    void summaryChanged();

private:
    void add(const QString &layer, const QString &label, bool ok,
             const QString &detail, const QString &hint = {});

    Backend *m_backend = nullptr;
    HarnessConnections *m_connections = nullptr;
    McpManager *m_mcp = nullptr;
    MemoryStore *m_memory = nullptr;
    BotManager *m_bots = nullptr;
    QVariantList m_checks;
    int m_passed = 0;
    int m_failed = 0;
    bool m_running = false;
    QString m_summary;
};

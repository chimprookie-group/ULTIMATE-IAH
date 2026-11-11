#pragma once

#include <QObject>
#include <QString>
#include <QStringDecoder>

class QSocketNotifier;

// A real pseudo-terminal running the user's shell, bridged to the xterm.js
// page over QWebChannel. The shell is a child of the app; output is decoded
// incrementally so multi-byte UTF-8 split across reads stays intact.
class TerminalSession : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString shell READ shell NOTIFY shellChanged)
    Q_PROPERTY(QString cwd READ cwd NOTIFY cwdChanged)

public:
    explicit TerminalSession(QObject *parent = nullptr);
    ~TerminalSession() override;

    bool running() const { return m_pid > 0; }
    QString shell() const;
    QString cwd() const { return m_cwd; }

    Q_INVOKABLE void start(const QString &cwd);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void input(const QString &data);
    Q_INVOKABLE void run(const QString &command);
    Q_INVOKABLE void resize(int columns, int rows);
    Q_INVOKABLE void copy(const QString &text);
    Q_INVOKABLE void paste();

signals:
    void output(const QString &data);
    void exited(int code);
    void started();
    void runningChanged();
    void shellChanged();
    void cwdChanged();

private:
    void closeMaster();
    void readOutput();
    QString resolvedShell() const;

    int m_master = -1;
    int m_pid = -1;
    QSocketNotifier *m_notifier = nullptr;
    QString m_cwd;
    QString m_pendingInput;
    QStringDecoder m_decoder{QStringDecoder::Utf8};
    bool m_stopping = false;
};

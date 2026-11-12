#pragma once

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QTimer>

// Owns the `dsh web` process that serves the DeepSeek Harness browser UI and
// extracts the one-time tokenized URL it prints on startup. The app embeds that
// URL in a WebEngineView, so the harness lives inside Ultimate HAI instead of a
// separate browser tab.
class HarnessServer : public QObject {
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString url READ url NOTIFY urlChanged)
    Q_PROPERTY(QString detail READ detail NOTIFY detailChanged)
    Q_PROPERTY(QString profile READ profile NOTIFY profileChanged)
    Q_PROPERTY(QString logTail READ logTail NOTIFY logChanged)

public:
    enum State { Stopped, Starting, Running, Failed };
    Q_ENUM(State)

    explicit HarnessServer(QObject *parent = nullptr);
    ~HarnessServer() override;

    State state() const { return m_state; }
    QString url() const { return m_url; }
    QString detail() const { return m_detail; }
    QString profile() const { return m_profile; }
    QString logTail() const { return m_log.join('\n'); }

    void setBinary(const QString &binary);
    void setProfile(const QString &profile);
    void setWorkingDirectory(const QString &directory);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void restart();

    // Exposed for tests and for the doctor command.
    static QString extractUrl(const QString &line);

signals:
    void stateChanged();
    void urlChanged();
    void detailChanged();
    void profileChanged();
    void logChanged();

private:
    void setState(State state, const QString &detail = {});
    void consume(const QByteArray &chunk);
    void fail(const QString &detail);

    QProcess m_process;
    QTimer m_timeout;
    QTimer m_killTimer;
    State m_state = Stopped;
    QString m_url;
    QString m_detail;
    QString m_profile = "web";
    QString m_binary = "dsh";
    QString m_directory;
    QStringList m_log;
    bool m_stopping = false;
};

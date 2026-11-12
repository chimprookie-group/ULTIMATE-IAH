#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QVariantList>

class Backend;

// GitHub ecosystem backed by the authenticated gh CLI. Tokens stay in gh's
// keyring; Ultimate IAH only parses command output and never reads a token.
class GitHubService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY changed)
    Q_PROPERTY(QString login READ login NOTIFY changed)
    Q_PROPERTY(QString accountName READ accountName NOTIFY changed)
    Q_PROPERTY(QString scopes READ scopes NOTIFY changed)
    Q_PROPERTY(QVariantList repositories READ repositories NOTIFY changed)
    Q_PROPERTY(QVariantList issues READ issues NOTIFY changed)
    Q_PROPERTY(QVariantList pulls READ pulls NOTIFY changed)
    Q_PROPERTY(QString selectedRepo READ selectedRepo NOTIFY selectedChanged)
    Q_PROPERTY(QVariantMap selectedRepository READ selectedRepository NOTIFY selectedChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString workspace READ workspace NOTIFY changed)

public:
    explicit GitHubService(Backend *backend, QObject *parent = nullptr);
    ~GitHubService() override;

    bool authenticated() const { return m_authenticated; }
    QString login() const { return m_login; }
    QString accountName() const { return m_accountName; }
    QString scopes() const { return m_scopes; }
    QVariantList repositories() const { return m_repositories; }
    QVariantList issues() const { return m_issues; }
    QVariantList pulls() const { return m_pulls; }
    QString selectedRepo() const { return m_selectedRepo; }
    QVariantMap selectedRepository() const;
    bool loading() const { return m_loading; }
    QString error() const { return m_error; }
    QString workspace() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void selectRepository(const QString &fullName);
    Q_INVOKABLE void loadIssues(const QString &fullName);
    Q_INVOKABLE void loadPulls(const QString &fullName);
    Q_INVOKABLE QVariantMap createRepository(const QString &name, const QString &description,
                                             bool privateRepo, bool clone);
    Q_INVOKABLE bool cloneRepository(const QString &fullName);
    Q_INVOKABLE bool createIssue(const QString &title, const QString &body);
    Q_INVOKABLE void openRepository(const QString &url);
    Q_INVOKABLE void openWorkspace();
    Q_INVOKABLE void clearError();

    QJsonObject status() const;
    QJsonObject createRepositoryJson(const QString &name, const QString &description,
                                     bool privateRepo, bool clone);
    QJsonObject issuesJson(const QString &fullName);
    QJsonObject pullsJson(const QString &fullName);
    QJsonObject createIssueJson(const QString &title, const QString &body);
    QJsonObject cloneJson(const QString &fullName);

    static QVariantList parseRepositories(const QJsonArray &array);
    static QVariantList parseIssues(const QJsonArray &array);
    static QVariantList parsePulls(const QJsonArray &array);

signals:
    void changed();
    void selectedChanged();
    void loadingChanged();
    void errorChanged();

private:
    QJsonValue runGh(const QStringList &arguments, int timeoutMs = 30000, bool *ok = nullptr,
                     QString *output = nullptr) const;
    QString ghBinary() const;
    void setLoading(bool loading);
    void setError(const QString &error);
    QString repoName(const QString &fullName) const;
    QString clonePath(const QString &fullName) const;

    Backend *m_backend = nullptr;
    bool m_authenticated = false;
    QString m_login;
    QString m_accountName;
    QString m_scopes;
    QVariantList m_repositories;
    QVariantList m_issues;
    QVariantList m_pulls;
    QString m_selectedRepo;
    bool m_loading = false;
    QString m_error;
};

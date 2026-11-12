#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QVariantList>
#include <QString>

class QSqlDatabase;

// Persistent index for Ultimate Agent conversations. Each conversation stores
// the UI transcript and the exact OpenAI-compatible message array in SQLite, so
// switching, searching and deleting a discussion never relies on process state.
class AgentConversationStore : public QObject {
    Q_OBJECT

public:
    explicit AgentConversationStore(const QString &path = {}, QObject *parent = nullptr);
    ~AgentConversationStore() override;

    QVariantList conversations(const QString &query = {}, bool includeArchived = false) const;
    QJsonObject conversation(const QString &id) const;
    QString lastConversationId() const;
    QString error() const { return m_error; }

    QString create(const QString &title, const QString &model, const QString &endpoint);
    bool save(const QString &id, const QString &title, const QJsonArray &api,
              const QJsonArray &ui, const QString &model, const QString &endpoint);
    bool remove(const QString &id);
    bool rename(const QString &id, const QString &title);
    bool setPinned(const QString &id, bool pinned);
    bool setArchived(const QString &id, bool archived);
    bool importLegacy(const QString &path);
    void clearError();

    static QString defaultPath();

signals:
    void changed();
    void errorChanged();

private:
    bool ensureSchema();
    void setError(const QString &error);
    QString meta(const QString &key) const;
    bool setMeta(const QString &key, const QString &value);
    QString previewFor(const QJsonArray &ui) const;
    QString titleFor(const QString &title, const QJsonArray &ui) const;

    QString m_path;
    QString m_connectionName;
    QSqlDatabase *m_db = nullptr;
    QString m_error;
};

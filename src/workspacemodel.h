#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Read-only view of the workspace: files tracked by Git (or a bounded walk),
// their status, a safe text preview and the Git diff of a file.
class WorkspaceModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList files READ files NOTIFY filesChanged)
    Q_PROPERTY(int count READ count NOTIFY filesChanged)
    Q_PROPERTY(QString workspace READ workspace NOTIFY workspaceChanged)

public:
    explicit WorkspaceModel(QObject *parent = nullptr);

    QVariantList files() const { return m_files; }
    int count() const { return m_files.size(); }
    QString workspace() const { return m_workspace; }

    Q_INVOKABLE void refresh(const QString &workspace);
    Q_INVOKABLE void setFilter(const QString &filter);
    Q_INVOKABLE QString preview(const QString &path) const;
    Q_INVOKABLE QString diff(const QString &path) const;
    Q_INVOKABLE QString absolutePath(const QString &path) const;
    Q_INVOKABLE void copyToClipboard(const QString &text) const;

signals:
    void filesChanged();
    void workspaceChanged();

private:
    QString resolve(const QString &path) const;
    QVariantMap entry(const QString &relative, const QHash<QString, QString> &statuses) const;

    QString m_workspace;
    QString m_filter;
    QVariantList m_all;
    QVariantList m_files;
};

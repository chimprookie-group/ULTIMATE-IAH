#pragma once

#include <QObject>

// Persistent settings for DeepCrasy H, stored as one JSON object under the
// user's config directory. Every setter writes the file atomically so a crash
// never leaves a half-written configuration behind.
class Config : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString workspace READ workspace WRITE setWorkspace NOTIFY workspaceChanged)
    Q_PROPERTY(QString harnessProfile READ harnessProfile WRITE setHarnessProfile NOTIFY harnessProfileChanged)
    Q_PROPERTY(QString dshBinary READ dshBinary WRITE setDshBinary NOTIFY dshBinaryChanged)
    Q_PROPERTY(QString opencodeBinary READ opencodeBinary WRITE setOpencodeBinary NOTIFY opencodeBinaryChanged)
    Q_PROPERTY(QString view READ view WRITE setView NOTIFY viewChanged)

public:
    explicit Config(const QString &path = {}, QObject *parent = nullptr);

    QString workspace() const { return m_workspace; }
    void setWorkspace(const QString &workspace);

    QString harnessProfile() const { return m_harnessProfile; }
    void setHarnessProfile(const QString &profile);

    QString dshBinary() const { return m_dshBinary; }
    void setDshBinary(const QString &binary);

    QString opencodeBinary() const { return m_opencodeBinary; }
    void setOpencodeBinary(const QString &binary);

    QString view() const { return m_view; }
    void setView(const QString &view);

    QString path() const { return m_path; }

    static QString defaultPath();
    static QString defaultWorkspace();
    static QString detectBinary(const QString &name);

signals:
    void workspaceChanged();
    void harnessProfileChanged();
    void dshBinaryChanged();
    void opencodeBinaryChanged();
    void viewChanged();

private:
    void load();
    void save();

    QString m_path;
    QString m_workspace;
    QString m_harnessProfile;
    QString m_dshBinary;
    QString m_opencodeBinary;
    QString m_view;
};

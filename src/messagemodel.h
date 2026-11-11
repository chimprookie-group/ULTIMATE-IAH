#pragma once

#include <QAbstractListModel>
#include <QVariantList>
#include <QVariantMap>

// The transcript as a real list model: a streamed token only updates one row
// (dataChanged), so ListView keeps its delegates and its scroll position.
class MessageListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        RoleRole,
        TextRole,
        StreamingRole,
        MessageIdRole,
        ToolIdRole,
        TitleRole,
        KindRole,
        StatusRole,
        OutputRole,
        InputRole,
        LocationsRole,
        EntriesRole,
        ExitCodeRole
    };

    explicit MessageListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_items.size(); }
    QVariantMap at(int row) const;
    QVariantList items() const { return m_items; }

    void append(const QVariantMap &item);
    void setAt(int row, const QVariantMap &item);
    void clear();

private:
    QVariantList m_items;
    QVector<int> m_roles;
};

#pragma once

#include <QAbstractListModel>
#include <QVariantList>
#include <QVariantMap>

// Stable list model for the agent transcript. Appending a message emits
// rowsInserted instead of replacing the whole model, so the QML ListView keeps
// its scroll position and does not jump while the agent works.
class AgentMessageModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit AgentMessageModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void reset(const QVariantList &rows);
    void append(const QVariantMap &row);
    void update(int row, const QVariantMap &rowData);

private:
    QVariantList m_rows;
};

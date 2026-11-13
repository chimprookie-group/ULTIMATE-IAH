#include "agentmessagemodel.h"

AgentMessageModel::AgentMessageModel(QObject *parent)
    : QAbstractListModel(parent) {}

int AgentMessageModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant AgentMessageModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    if (role == Qt::DisplayRole)
        return m_rows.at(index.row());
    return {};
}

QHash<int, QByteArray> AgentMessageModel::roleNames() const {
    return {{Qt::DisplayRole, QByteArrayLiteral("modelData")}};
}

void AgentMessageModel::reset(const QVariantList &rows) {
    beginResetModel();
    m_rows = rows;
    endResetModel();
}

void AgentMessageModel::append(const QVariantMap &row) {
    const int position = m_rows.size();
    beginInsertRows(QModelIndex(), position, position);
    m_rows.append(row);
    endInsertRows();
}

void AgentMessageModel::update(int row, const QVariantMap &rowData) {
    if (row < 0 || row >= m_rows.size())
        return;
    m_rows[row] = rowData;
    const QModelIndex changed = index(row, 0);
    emit dataChanged(changed, changed);
}

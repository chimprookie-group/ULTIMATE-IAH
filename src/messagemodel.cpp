#include "messagemodel.h"

MessageListModel::MessageListModel(QObject *parent)
    : QAbstractListModel(parent) {
    m_roles = {IdRole, RoleRole, TextRole, StreamingRole, MessageIdRole, ToolIdRole,
               TitleRole, KindRole, StatusRole, OutputRole, InputRole, LocationsRole,
               EntriesRole, ExitCodeRole};
}

int MessageListModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_items.size();
}

QVariant MessageListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};
    const QVariantMap item = m_items.at(index.row()).toMap();
    switch (role) {
    case IdRole: return item.value("id");
    case RoleRole: return item.value("role");
    case TextRole: return item.value("text");
    case StreamingRole: return item.value("streaming");
    case MessageIdRole: return item.value("messageId");
    case ToolIdRole: return item.value("toolId");
    case TitleRole: return item.value("title");
    case KindRole: return item.value("kind");
    case StatusRole: return item.value("status");
    case OutputRole: return item.value("output");
    case InputRole: return item.value("input");
    case LocationsRole: return item.value("locations");
    case EntriesRole: return item.value("entries");
    case ExitCodeRole: return item.value("exitCode");
    default: return {};
    }
}

QHash<int, QByteArray> MessageListModel::roleNames() const {
    return {
        {IdRole, "id"},
        {RoleRole, "role"},
        {TextRole, "text"},
        {StreamingRole, "streaming"},
        {MessageIdRole, "messageId"},
        {ToolIdRole, "toolId"},
        {TitleRole, "title"},
        {KindRole, "kind"},
        {StatusRole, "status"},
        {OutputRole, "output"},
        {InputRole, "input"},
        {LocationsRole, "locations"},
        {EntriesRole, "entries"},
        {ExitCodeRole, "exitCode"},
    };
}

QVariantMap MessageListModel::at(int row) const {
    if (row < 0 || row >= m_items.size())
        return {};
    return m_items.at(row).toMap();
}

void MessageListModel::append(const QVariantMap &item) {
    const int row = m_items.size();
    beginInsertRows(QModelIndex(), row, row);
    m_items.append(item);
    endInsertRows();
}

void MessageListModel::setAt(int row, const QVariantMap &item) {
    if (row < 0 || row >= m_items.size())
        return;
    m_items[row] = item;
    emit dataChanged(index(row), index(row), m_roles);
}

void MessageListModel::clear() {
    if (m_items.isEmpty())
        return;
    beginResetModel();
    m_items.clear();
    endResetModel();
}

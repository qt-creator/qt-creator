#include "%ClassName:l%.%CppHeaderSuffix%"

%ClassName%::%ClassName%(QObject *parent) :
    QAbstractListModel(parent)
{
}

void %ClassName%::addItems(const QList<%Datatype%> &newItems)
{
    beginInsertRows(QModelIndex(), items.size(), items.size() + newItems.size());
    items.append(newItems);
    endInsertRows();
}

int %ClassName%::rowCount(const QModelIndex &) const
{
    return items.size();
}

QVariant %ClassName%::data(const QModelIndex &index, int)
{
    return items.at(index.row());
}

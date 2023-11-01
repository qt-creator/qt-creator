%{JS: Cpp.licenseTemplate()}\
#include "%{JS: Util.relativeFilePath('%{Path}/%{HdrFileName}', '%{Path}' + '/' + Util.path('%{SrcFileName}'))}"
%{JS: Cpp.openNamespaces('%{Class}')}\

%{CN}::%{CN}(QObject *parent)
    : %{Base}(parent)
{
}
@if %{CustomHeader}

QVariant %{CN}::headerData(int section, Qt::Orientation orientation, int role) const
{
    // FIXME: Implement me!
}
@if %{Editable}

bool %{CN}::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (value != headerData(section, orientation, role)) {
        // FIXME: Implement me!
        emit headerDataChanged(orientation, section, section);
        return true;
    }
    return false;
}
@endif
@endif

int %{CN}::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
}
@if %{DynamicFetch}

bool %{CN}::hasChildren(const QModelIndex &parent) const
{
    // FIXME: Implement me!
}

bool %{CN}::canFetchMore(const QModelIndex &parent) const
{
    // FIXME: Implement me!
    return false;
}

void %{CN}::fetchMore(const QModelIndex &parent)
{
    // FIXME: Implement me!
}
@endif

QVariant %{CN}::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!
    return QVariant();
}
@if %{Editable}

bool %{CN}::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        // FIXME: Implement me!
        emit dataChanged(index, index, {role});
        return true;
    }
    return false;
}

Qt::ItemFlags %{CN}::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable; // FIXME: Implement me!
}
@endif
@if %{AddData}

bool %{CN}::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endInsertRows();
    return true;
}
@endif
@if %{RemoveData}

bool %{CN}::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endRemoveRows();
    return true;
}
@endif
%{JS: Cpp.closeNamespaces('%{Class}')}\

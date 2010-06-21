#include "filesystemmodel.h"

FileSystemModel::FileSystemModel(QObject* parent)
    :QFileSystemModel(parent)
{

}

FileSystemModel::~FileSystemModel()
{

}

int FileSystemModel::columnCount(const QModelIndex &parent ) const
{
    Q_UNUSED(parent)
    return 1;
}



#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QFileSystemModel>

/**
  This FileSystemModel is subcalss of QFileSystemModel which just returns only 1 column.
*/
class FileSystemModel : public QFileSystemModel
{
public:
    FileSystemModel(QObject* parent=0);
    ~FileSystemModel();
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
};

#endif // FILESYSTEMMODEL_H

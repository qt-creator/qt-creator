/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

//![code]
#include "qdeclarativefolderlistmodel.h"
#include <QDirModel>
#include <QDebug>
#include <qdeclarativecontext.h>

#ifndef QT_NO_DIRMODEL

QT_BEGIN_NAMESPACE

class QDeclarativeFolderListModelPrivate
{
public:
    QDeclarativeFolderListModelPrivate()
        : sortField(QDeclarativeFolderListModel::Name), sortReversed(false), count(0) {
        nameFilters << QLatin1String("*");
    }

    void updateSorting() {
        QDir::SortFlags flags = 0;
        switch (sortField) {
        case QDeclarativeFolderListModel::Unsorted:
            flags |= QDir::Unsorted;
            break;
        case QDeclarativeFolderListModel::Name:
            flags |= QDir::Name;
            break;
        case QDeclarativeFolderListModel::Time:
            flags |= QDir::Time;
            break;
        case QDeclarativeFolderListModel::Size:
            flags |= QDir::Size;
            break;
        case QDeclarativeFolderListModel::Type:
            flags |= QDir::Type;
            break;
        }

        if (sortReversed)
            flags |= QDir::Reversed;

        model.setSorting(flags);
    }

    QDirModel model;
    QUrl folder;
    QStringList nameFilters;
    QModelIndex folderIndex;
    QDeclarativeFolderListModel::SortField sortField;
    bool sortReversed;
    int count;
};

/*!
    \qmlclass FolderListModel QDeclarativeFolderListModel
    \ingroup qml-working-with-data
    \brief The FolderListModel provides a model of the contents of a file system folder.

    FolderListModel provides access to information about the contents of a folder
    in the local file system, exposing a list of files to views and other data components.

    \note This type is made available by importing the \c Qt.labs.folderlistmodel module.
    \e{Elements in the Qt.labs module are not guaranteed to remain compatible
    in future versions.}

    \bold{import Qt.labs.folderlistmodel 1.0}

    The \l folder property specifies the folder to access. Information about the
    files and directories in the folder is supplied via the model's interface.
    Components access names and paths via the following roles:

    \list
    \o fileName
    \o filePath
    \endlist

    Additionally a file entry can be differentiated from a folder entry via the
    isFolder() method.

    \section1 Filtering

    Various properties can be set to filter the number of files and directories
    exposed by the model.

    The \l nameFilters property can be set to contain a list of wildcard filters
    that are applied to names of files and directories, causing only those that
    match the filters to be exposed.

    Directories can be included or excluded using the \l showDirs property, and
    navigation directories can also be excluded by setting the \l showDotAndDotDot
    property to false.

    It is sometimes useful to limit the files and directories exposed to those
    that the user can access. The \l showOnlyReadable property can be set to
    enable this feature.

    \section1 Example Usage

    The following example shows a FolderListModel being used to provide a list
    of QML files in a \l ListView:

    \snippet doc/src/snippets/declarative/folderlistmodel.qml 0

    \section1 Path Separators

    Qt uses "/" as a universal directory separator in the same way that "/" is
    used as a path separator in URLs. If you always use "/" as a directory
    separator, Qt will translate your paths to conform to the underlying
    operating system.

    \sa {QML Data Models}
*/

QDeclarativeFolderListModel::QDeclarativeFolderListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles[FileNameRole] = "fileName";
    roles[FilePathRole] = "filePath";
    roles[FileSizeRole] = "fileSize";
    setRoleNames(roles);

    d = new QDeclarativeFolderListModelPrivate;
    d->model.setFilter(QDir::AllDirs | QDir::Files | QDir::Drives | QDir::NoDotAndDotDot);
    connect(&d->model, SIGNAL(rowsInserted(QModelIndex,int,int))
            , this, SLOT(inserted(QModelIndex,int,int)));
    connect(&d->model, SIGNAL(rowsRemoved(QModelIndex,int,int))
            , this, SLOT(removed(QModelIndex,int,int)));
    connect(&d->model, SIGNAL(dataChanged(QModelIndex,QModelIndex))
            , this, SLOT(handleDataChanged(QModelIndex,QModelIndex)));
    connect(&d->model, SIGNAL(modelReset()), this, SLOT(refresh()));
    connect(&d->model, SIGNAL(layoutChanged()), this, SLOT(refresh()));
}

QDeclarativeFolderListModel::~QDeclarativeFolderListModel()
{
    delete d;
}

QVariant QDeclarativeFolderListModel::data(const QModelIndex &index, int role) const
{
    QVariant rv;
    QModelIndex modelIndex = d->model.index(index.row(), 0, d->folderIndex);
    if (modelIndex.isValid()) {
        if (role == FileNameRole)
            rv = d->model.data(modelIndex, QDirModel::FileNameRole).toString();
        else if (role == FilePathRole)
            rv = QUrl::fromLocalFile(d->model.data(modelIndex, QDirModel::FilePathRole).toString());
        else if (role == FileSizeRole)
            rv = d->model.data(d->model.index(index.row(), 1, d->folderIndex), Qt::DisplayRole).toString();
    }
    return rv;
}

/*!
    \qmlproperty int FolderListModel::count

    Returns the number of items in the current folder that match the
    filter criteria.
*/
int QDeclarativeFolderListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return d->count;
}

/*!
    \qmlproperty string FolderListModel::folder

    The \a folder property holds a URL for the folder that the model is
    currently providing.

    The value is a URL expressed as a string, and must be a \c file: or \c qrc:
    URL, or a relative URL.

    By default, the value is an invalid URL.
*/
QUrl QDeclarativeFolderListModel::folder() const
{
    return d->folder;
}

void QDeclarativeFolderListModel::setFolder(const QUrl &folder)
{
    if (folder == d->folder)
        return;
    QModelIndex index = d->model.index(folder.toLocalFile());
    if ((index.isValid() && d->model.isDir(index)) || folder.toLocalFile().isEmpty()) {

        d->folder = folder;
        QMetaObject::invokeMethod(this, "refresh", Qt::QueuedConnection);
        emit folderChanged();
    }
}

/*!
    \qmlproperty url FolderListModel::parentFolder

    Returns the URL of the parent of of the current \l folder.
*/
QUrl QDeclarativeFolderListModel::parentFolder() const
{
    QString localFile = d->folder.toLocalFile();
    if (!localFile.isEmpty()) {
        QDir dir(localFile);
        dir.cdUp();
        localFile = dir.path();
    } else {
        int pos = d->folder.path().lastIndexOf(QLatin1Char('/'));
        if (pos == -1)
            return QUrl();
        localFile = d->folder.path().left(pos);
    }
    return QUrl::fromLocalFile(localFile);
}

/*!
    \qmlproperty list<string> FolderListModel::nameFilters

    The \a nameFilters property contains a list of file name filters.
    The filters may include the ? and * wildcards.

    The example below filters on PNG and JPEG files:

    \qml
    FolderListModel {
        nameFilters: [ "*.png", "*.jpg" ]
    }
    \endqml

    \note Directories are not excluded by filters.
*/
QStringList QDeclarativeFolderListModel::nameFilters() const
{
    return d->nameFilters;
}

void QDeclarativeFolderListModel::setNameFilters(const QStringList &filters)
{
    d->nameFilters = filters;
    d->model.setNameFilters(d->nameFilters);
}

void QDeclarativeFolderListModel::classBegin()
{
}

void QDeclarativeFolderListModel::componentComplete()
{
    if (!d->folder.isValid() || d->folder.toLocalFile().isEmpty() || !QDir().exists(d->folder.toLocalFile()))
        setFolder(QUrl(QLatin1String("file://")+QDir::currentPath()));

    if (!d->folderIndex.isValid())
        QMetaObject::invokeMethod(this, "refresh", Qt::QueuedConnection);
}

/*!
    \qmlproperty enumeration FolderListModel::sortField

    The \a sortField property contains field to use for sorting.  sortField
    may be one of:
    \list
    \o Unsorted - no sorting is applied.  The order is system default.
    \o Name - sort by filename
    \o Time - sort by time modified
    \o Size - sort by file size
    \o Type - sort by file type (extension)
    \endlist

    \sa sortReversed
*/
QDeclarativeFolderListModel::SortField QDeclarativeFolderListModel::sortField() const
{
    return d->sortField;
}

void QDeclarativeFolderListModel::setSortField(SortField field)
{
    if (field != d->sortField) {
        d->sortField = field;
        d->updateSorting();
    }
}

/*!
    \qmlproperty bool FolderListModel::sortReversed

    If set to true, reverses the sort order.  The default is false.

    \sa sortField
*/
bool QDeclarativeFolderListModel::sortReversed() const
{
    return d->sortReversed;
}

void QDeclarativeFolderListModel::setSortReversed(bool rev)
{
    if (rev != d->sortReversed) {
        d->sortReversed = rev;
        d->updateSorting();
    }
}

/*!
    \qmlmethod bool FolderListModel::isFolder(int index)

    Returns true if the entry \a index is a folder; otherwise
    returns false.
*/
bool QDeclarativeFolderListModel::isFolder(int index) const
{
    if (index != -1) {
        QModelIndex idx = d->model.index(index, 0, d->folderIndex);
        if (idx.isValid())
            return d->model.isDir(idx);
    }
    return false;
}

void QDeclarativeFolderListModel::refresh()
{
    d->folderIndex = QModelIndex();
    if (d->count) {
        emit beginRemoveRows(QModelIndex(), 0, d->count-1);
        d->count = 0;
        emit endRemoveRows();
    }
    d->folderIndex = d->model.index(d->folder.toLocalFile());
    int newcount = d->model.rowCount(d->folderIndex);
    if (newcount) {
        emit beginInsertRows(QModelIndex(), 0, newcount-1);
        d->count = newcount;
        emit endInsertRows();
    }
}

void QDeclarativeFolderListModel::inserted(const QModelIndex &index, int start, int end)
{
    if (index == d->folderIndex) {
        emit beginInsertRows(QModelIndex(), start, end);
        d->count = d->model.rowCount(d->folderIndex);
        emit endInsertRows();
    }
}

void QDeclarativeFolderListModel::removed(const QModelIndex &index, int start, int end)
{
    if (index == d->folderIndex) {
        emit beginRemoveRows(QModelIndex(), start, end);
        d->count = d->model.rowCount(d->folderIndex);
        emit endRemoveRows();
    }
}

void QDeclarativeFolderListModel::handleDataChanged(const QModelIndex &start, const QModelIndex &end)
{
    if (start.parent() == d->folderIndex)
        emit dataChanged(index(start.row(),0), index(end.row(),0));
}

/*!
    \qmlproperty bool FolderListModel::showDirs

    If true, directories are included in the model; otherwise only files
    are included.

    By default, this property is true.

    Note that the nameFilters are not applied to directories.

    \sa showDotAndDotDot
*/
bool QDeclarativeFolderListModel::showDirs() const
{
    return d->model.filter() & QDir::AllDirs;
}

void  QDeclarativeFolderListModel::setShowDirs(bool on)
{
    if (!(d->model.filter() & QDir::AllDirs) == !on)
        return;
    if (on)
        d->model.setFilter(d->model.filter() | QDir::AllDirs | QDir::Drives);
    else
        d->model.setFilter(d->model.filter() & ~(QDir::AllDirs | QDir::Drives));
}

/*!
    \qmlproperty bool FolderListModel::showDotAndDotDot

    If true, the "." and ".." directories are included in the model; otherwise
    they are excluded.

    By default, this property is false.

    \sa showDirs
*/
bool QDeclarativeFolderListModel::showDotAndDotDot() const
{
    return !(d->model.filter() & QDir::NoDotAndDotDot);
}

void  QDeclarativeFolderListModel::setShowDotAndDotDot(bool on)
{
    if (!(d->model.filter() & QDir::NoDotAndDotDot) == on)
        return;
    if (on)
        d->model.setFilter(d->model.filter() & ~QDir::NoDotAndDotDot);
    else
        d->model.setFilter(d->model.filter() | QDir::NoDotAndDotDot);
}

/*!
    \qmlproperty bool FolderListModel::showOnlyReadable

    If true, only readable files and directories are shown; otherwise all files
    and directories are shown.

    By default, this property is false.

    \sa showDirs
*/
bool QDeclarativeFolderListModel::showOnlyReadable() const
{
    return d->model.filter() & QDir::Readable;
}

void QDeclarativeFolderListModel::setShowOnlyReadable(bool on)
{
    if (!(d->model.filter() & QDir::Readable) == !on)
        return;
    if (on)
        d->model.setFilter(d->model.filter() | QDir::Readable);
    else
        d->model.setFilter(d->model.filter() & ~QDir::Readable);
}

//![code]
QT_END_NAMESPACE

#endif // QT_NO_DIRMODEL

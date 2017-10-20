/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "customfilesystemmodel.h"

#include <utils/filesystemwatcher.h>

#include <QDir>
#include <QDirIterator>
#include <QFileIconProvider>
#include <QFileSystemModel>
#include <QFont>
#include <QImageReader>

namespace QmlDesigner {

class ItemLibraryFileIconProvider : public QFileIconProvider
{
public:
    ItemLibraryFileIconProvider()
    {
    }

    QIcon icon( const QFileInfo & info ) const
    {
        QIcon icon;

        for (auto iconSize : iconSizes) {

            QPixmap pixmap(info.absoluteFilePath());

            if (pixmap.isNull())
                return QFileIconProvider::icon(info);

            if (pixmap.width() == iconSize.width()
                    && pixmap.height() == iconSize.height())
                return pixmap;

            if ((pixmap.width() > iconSize.width())
                    || (pixmap.height() > iconSize.height()))
                pixmap = pixmap.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            icon.addPixmap(pixmap);
        }

        return icon;
    }

    QList<QSize> iconSizes  = {{256, 196},{128, 96},{64, 64},{32, 32}};

};

CustomFileSystemModel::CustomFileSystemModel(QObject *parent) : QAbstractListModel(parent)
  , m_fileSystemModel(new QFileSystemModel(this))
  , m_fileSystemWatcher(new Utils::FileSystemWatcher(this))
{
    m_fileSystemModel->setIconProvider(new ItemLibraryFileIconProvider());

    connect(m_fileSystemWatcher, &Utils::FileSystemWatcher::directoryChanged, [this] {
        setRootPath(m_fileSystemModel->rootPath());
    });
}

void CustomFileSystemModel::setFilter(QDir::Filters)
{

}

QModelIndex CustomFileSystemModel::setRootPath(const QString &newPath)
{
    beginResetModel();
    m_fileSystemModel->setRootPath(newPath);

    m_fileSystemWatcher->removeDirectories(m_fileSystemWatcher->directories());

    m_fileSystemWatcher->addDirectory(newPath, Utils::FileSystemWatcher::WatchAllChanges);

    QStringList nameFilterList;

    const QString searchFilter = m_searchFilter;

    if (searchFilter.contains(QLatin1Char('.'))) {
        nameFilterList.append(QString(QStringLiteral("*%1*")).arg(searchFilter));
    } else {
        foreach (const QByteArray &extension, QImageReader::supportedImageFormats()) {
            nameFilterList.append(QString(QStringLiteral("*%1*.%2")).arg(searchFilter, QString::fromUtf8(extension)));
        }
    }

    m_files.clear();

    QDirIterator fileIterator(newPath, nameFilterList, QDir::Files, QDirIterator::Subdirectories);

    while (fileIterator.hasNext())
        m_files.append(fileIterator.next());

    QDirIterator dirIterator(newPath, {}, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (dirIterator.hasNext())
        m_fileSystemWatcher->addDirectory(dirIterator.next(), Utils::FileSystemWatcher::WatchAllChanges);

    endResetModel();
    return QAbstractListModel::index(0, 0);
}

QVariant CustomFileSystemModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ToolTipRole)
        return fileInfo(index).filePath();

    if (role == Qt::FontRole) {
        QFont font = m_fileSystemModel->data(fileSystemModelIndex(index), role).value<QFont>();
        font.setPixelSize(9);
        return font;
    }


    return m_fileSystemModel->data(fileSystemModelIndex(index), role);
}

int CustomFileSystemModel::rowCount(const QModelIndex &) const
{
    return m_files.count();
}

int CustomFileSystemModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QModelIndex CustomFileSystemModel::indexForPath(const QString &path, int /*column*/) const
{
    return QAbstractListModel::index(m_files.indexOf(path), 0);
}

QIcon CustomFileSystemModel::fileIcon(const QModelIndex &index) const
{
    return m_fileSystemModel->fileIcon(fileSystemModelIndex(index));
}

QString CustomFileSystemModel::fileName(const QModelIndex &index) const
{
    return m_fileSystemModel->fileName(fileSystemModelIndex(index));
}

QFileInfo CustomFileSystemModel::fileInfo(const QModelIndex &index) const
{
    return m_fileSystemModel->fileInfo(fileSystemModelIndex(index));
}

Qt::ItemFlags CustomFileSystemModel::flags(const QModelIndex &index) const
{
    return m_fileSystemModel->flags (fileSystemModelIndex(index));
}

void CustomFileSystemModel::setSearchFilter(const QString &nameFilterList)
{
    m_searchFilter = nameFilterList;
    setRootPath(m_fileSystemModel->rootPath());
}

QModelIndex CustomFileSystemModel::fileSystemModelIndex(const QModelIndex &index) const
{
    const int row = index.row();
    return m_fileSystemModel->index(m_files.at(row));
}

} //QmlDesigner

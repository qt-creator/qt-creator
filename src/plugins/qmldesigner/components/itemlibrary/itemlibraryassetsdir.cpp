/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "itemlibraryassetsdir.h"
#include "itemlibraryassetsdirsmodel.h"
#include "itemlibraryassetsfilesmodel.h"

namespace QmlDesigner {

ItemLibraryAssetsDir::ItemLibraryAssetsDir(const QString &path, int depth, bool expanded, QObject *parent)
    : QObject(parent)
    , m_dirPath(path)
    , m_dirDepth(depth)
    , m_dirExpanded(expanded)
{

}

QString ItemLibraryAssetsDir::dirName() const { return m_dirPath.split('/').last(); }
QString ItemLibraryAssetsDir::dirPath() const { return m_dirPath; }
int ItemLibraryAssetsDir::dirDepth() const { return m_dirDepth; }
bool ItemLibraryAssetsDir::dirExpanded() const { return m_dirExpanded; }
bool ItemLibraryAssetsDir::dirVisible() const { return m_dirVisible; }

void ItemLibraryAssetsDir::setDirExpanded(bool expand)
{
    if (m_dirExpanded != expand) {
        m_dirExpanded = expand;
        emit dirExpandedChanged();
    }
}

void ItemLibraryAssetsDir::setDirVisible(bool visible)
{
    if (m_dirVisible != visible) {
        m_dirVisible = visible;
        emit dirVisibleChanged();
    }
}

QObject *ItemLibraryAssetsDir::filesModel() const
{
    return m_filesModel;
}

QObject *ItemLibraryAssetsDir::dirsModel() const
{
    return m_dirsModel;
}

QList<ItemLibraryAssetsDir *> ItemLibraryAssetsDir::childAssetsDirs() const
{
    if (m_dirsModel)
        return m_dirsModel->assetsDirs();

    return {};
}

void ItemLibraryAssetsDir::addDir(ItemLibraryAssetsDir *assetsDir)
{
    if (!m_dirsModel)
        m_dirsModel = new ItemLibraryAssetsDirsModel(this);

    m_dirsModel->addDir(assetsDir);
}

void ItemLibraryAssetsDir::addFile(const QString &filePath)
{
    if (!m_filesModel)
        m_filesModel = new ItemLibraryAssetsFilesModel(this);

    m_filesModel->addFile(filePath);
}

} // namespace QmlDesigner

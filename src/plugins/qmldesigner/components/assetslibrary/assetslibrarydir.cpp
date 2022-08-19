// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "assetslibrarydir.h"
#include "assetslibrarydirsmodel.h"
#include "assetslibraryfilesmodel.h"

namespace QmlDesigner {

AssetsLibraryDir::AssetsLibraryDir(const QString &path, int depth, bool expanded, QObject *parent)
    : QObject(parent)
    , m_dirPath(path)
    , m_dirDepth(depth)
    , m_dirExpanded(expanded)
{

}

QString AssetsLibraryDir::dirName() const { return m_dirPath.split('/').last(); }
QString AssetsLibraryDir::dirPath() const { return m_dirPath; }
int AssetsLibraryDir::dirDepth() const { return m_dirDepth; }
bool AssetsLibraryDir::dirExpanded() const { return m_dirExpanded; }
bool AssetsLibraryDir::dirVisible() const { return m_dirVisible; }

void AssetsLibraryDir::setDirExpanded(bool expand)
{
    if (m_dirExpanded != expand) {
        m_dirExpanded = expand;
        emit dirExpandedChanged();
    }
}

void AssetsLibraryDir::setDirVisible(bool visible)
{
    if (m_dirVisible != visible) {
        m_dirVisible = visible;
        emit dirVisibleChanged();
    }
}

QObject *AssetsLibraryDir::filesModel() const
{
    return m_filesModel;
}

QObject *AssetsLibraryDir::dirsModel() const
{
    return m_dirsModel;
}

QList<AssetsLibraryDir *> AssetsLibraryDir::childAssetsDirs() const
{
    if (m_dirsModel)
        return m_dirsModel->assetsDirs();

    return {};
}

void AssetsLibraryDir::addDir(AssetsLibraryDir *assetsDir)
{
    if (!m_dirsModel)
        m_dirsModel = new AssetsLibraryDirsModel(this);

    m_dirsModel->addDir(assetsDir);
}

void AssetsLibraryDir::addFile(const QString &filePath)
{
    if (!m_filesModel)
        m_filesModel = new AssetsLibraryFilesModel(this);

    m_filesModel->addFile(filePath);
}

} // namespace QmlDesigner

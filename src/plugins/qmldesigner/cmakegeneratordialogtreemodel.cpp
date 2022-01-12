/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include "cmakegeneratordialogtreemodel.h"
#include "generatecmakelistsconstants.h"
#include "checkablefiletreeitem.h"

#include <utils/utilsicons.h>

using namespace Utils;

namespace QmlDesigner {
namespace GenerateCmake {

CMakeGeneratorDialogTreeModel::CMakeGeneratorDialogTreeModel(const FilePath &rootDir,
                                                             const FilePaths &files, QObject *parent)
    :QStandardItemModel(parent),
      rootDir(rootDir),
      m_icons(new QFileIconProvider())
{
    createNodes(files, invisibleRootItem());
}

CMakeGeneratorDialogTreeModel::~CMakeGeneratorDialogTreeModel()
{
    delete m_icons;
}

QVariant CMakeGeneratorDialogTreeModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        const CheckableFileTreeItem *node = constNodeForIndex(index);
        if (role == Qt::CheckStateRole) {
            if (!node->isDir())
                return node->isChecked() ? Qt::Checked : Qt::Unchecked;
            return {};
        }
        else if (role == Qt::DisplayRole) {
            FilePath fullPath = node->toFilePath();
            return QVariant(fullPath.fileName());
        }
        else if (role == Qt::DecorationRole) {
            if (node->isFile())
                return Utils::Icons::WARNING.icon();
            if (node->isDir())
                return m_icons->icon(QFileIconProvider::Folder);
            else
                return Utils::Icons::NEWFILE.icon();
        }
        else if (role == Qt::ToolTipRole) {
            if (node->isFile())
                return QCoreApplication::translate("QmlDesigner::GenerateCmake",
                                "This file already exists and will be overwritten.");
            if (!node->toFilePath().exists())
                return QCoreApplication::translate("QmlDesigner::GenerateCmake",
                                "This file or folder will be created.");
        }
    }

    return QStandardItemModel::data(index, role);
}

bool CMakeGeneratorDialogTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid()) {
        CheckableFileTreeItem *node = nodeForIndex(index);
        if (role == Qt::CheckStateRole) {
            node->setChecked(value.value<bool>());
            emit checkedStateChanged(node);
            return true;
        }
    }

    return QStandardItemModel::setData(index, value, role);;
}

const QList<CheckableFileTreeItem*> CMakeGeneratorDialogTreeModel::items() const
{
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QList<QStandardItem*> standardItems = findItems("*", Qt::MatchWildcard | Qt::MatchRecursive);
#else
    QList<QStandardItem*> standardItems = findItems(".*", Qt::MatchRegularExpression | Qt::MatchRecursive);
#endif
    QList<CheckableFileTreeItem*> checkableItems;
    for (QStandardItem *item : standardItems)
        checkableItems.append(static_cast<CheckableFileTreeItem*>(item));

    return checkableItems;
}

const QList<CheckableFileTreeItem*> CMakeGeneratorDialogTreeModel::checkedItems() const
{
    QList<CheckableFileTreeItem*> allItems = items();

    QList<CheckableFileTreeItem*> checkedItems;
    for (CheckableFileTreeItem *item : allItems) {
        if (item->isChecked())
            checkedItems.append(item);
    }

    return checkedItems;
}

bool CMakeGeneratorDialogTreeModel::checkedByDefault(const Utils::FilePath &file) const
{
    if (file.exists()) {
        QString relativePath = file.relativeChildPath(rootDir).toString();
        if (relativePath.compare(QmlDesigner::GenerateCmake::Constants::FILENAME_CMAKELISTS) == 0)
            return false;
        if (relativePath.endsWith(QmlDesigner::GenerateCmake::Constants::FILENAME_CMAKELISTS)
            && relativePath.length() > QString(QmlDesigner::GenerateCmake::Constants::FILENAME_CMAKELISTS).length())
            return true;
        if (relativePath.compare(QmlDesigner::GenerateCmake::Constants::FILENAME_MODULES) == 0)
            return true;
        if (relativePath.compare(
                FilePath::fromString(QmlDesigner::GenerateCmake::Constants::DIRNAME_CPP)
                .pathAppended(QmlDesigner::GenerateCmake::Constants::FILENAME_MAINCPP_HEADER)
                .toString())
                == 0)
            return true;
    }

    return !file.exists();
}

void CMakeGeneratorDialogTreeModel::createNodes(const FilePaths &candidates, QStandardItem *parent)
{
    if (!parent)
        return;

    CheckableFileTreeItem *checkParent = dynamic_cast<CheckableFileTreeItem*>(parent);
    FilePath thisDir = (parent == invisibleRootItem()) ? rootDir : checkParent->toFilePath();

    for (const FilePath &file : candidates) {
        if (file.parentDir() == thisDir) {
            CheckableFileTreeItem *fileNode = new CheckableFileTreeItem(file);
            fileNode->setChecked(checkedByDefault(file));
            if (!file.exists())
                fileNode->setChecked(true);
            parent->appendRow(fileNode);
        }
    }

    FilePaths directSubDirs;
    for (const FilePath &file : candidates) {
        FilePath dir = file.parentDir();
        if (dir.parentDir() == thisDir && !directSubDirs.contains(dir))
            directSubDirs.append(dir);
    }

    for (const FilePath &subDir : directSubDirs) {
        CheckableFileTreeItem *dirNode = new CheckableFileTreeItem(subDir);
        parent->appendRow(dirNode);

        FilePaths subDirCandidates;
        for (const FilePath &file : candidates)
            if (file.isChildOf(subDir))
                subDirCandidates.append(file);

        createNodes(subDirCandidates, dirNode);
    }
}

const CheckableFileTreeItem* CMakeGeneratorDialogTreeModel::constNodeForIndex(const QModelIndex &index) const
{
    const QStandardItem *parent = static_cast<const QStandardItem*>(index.constInternalPointer());
    const QStandardItem *item = parent->child(index.row(), index.column());
    return static_cast<const CheckableFileTreeItem*>(item);
}

CheckableFileTreeItem* CMakeGeneratorDialogTreeModel::nodeForIndex(const QModelIndex &index)
{
    QStandardItem *parent = static_cast<QStandardItem*>(index.internalPointer());
    QStandardItem *item = parent->child(index.row(), index.column());
    return static_cast<CheckableFileTreeItem*>(item);
}

} //GenerateCmake
} //QmlDesigner

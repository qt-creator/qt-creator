/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "clangselectablefilesdialog.h"
#include "ui_clangselectablefilesdialog.h"

#include "clangtoolsutils.h"

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectinfo.h>
#include <cpptools/projectpart.h>

#include <projectexplorer/selectablefilesmodel.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QPushButton>

using namespace CppTools;
using namespace Utils;
using namespace ProjectExplorer;

namespace ClangTools {
namespace Internal {

class TreeWithFileInfo : public Tree
{
public:
    FileInfo info;
};

static void linkDirNode(Tree *parentNode, Tree *childNode)
{
    parentNode->childDirectories.append(childNode);
    childNode->parent = parentNode;
}

static void linkFileNode(Tree *parentNode, Tree *childNode)
{
    childNode->parent = parentNode;
    parentNode->files.append(childNode);
    parentNode->visibleFiles.append(childNode);
}

static Tree *createDirNode(const QString &name, const FileName &filePath = FileName())
{
    auto node = new Tree;
    node->name = name;
    node->fullPath = filePath;
    node->isDir = true;

    return node;
}

static Tree *createFileNode(const FileInfo &fileInfo, bool displayFullPath = false)
{
    auto node = new TreeWithFileInfo;
    node->name = displayFullPath ? fileInfo.file.toString() : fileInfo.file.fileName();
    node->fullPath = fileInfo.file;
    node->info = fileInfo;

    return node;
}

class SelectableFilesModel : public ProjectExplorer::SelectableFilesModel
{
    Q_OBJECT

public:
    SelectableFilesModel(const CppTools::ProjectInfo &projectInfo, const FileInfos &allFileInfos)
        : ProjectExplorer::SelectableFilesModel(nullptr)
    {
        buildTree(projectInfo.project(), allFileInfos);
    }

    void buildTree(ProjectExplorer::Project *project, const FileInfos &fileInfos)
    {
        m_root->fullPath = project->projectFilePath();
        m_root->name = project->projectFilePath().fileName();
        m_root->isDir = true;

        FileInfos outOfBaseDirFiles;
        Tree *projectDirTree = buildProjectDirTree(project->projectDirectory(),
                                                   fileInfos,
                                                   outOfBaseDirFiles);
        if (outOfBaseDirFiles.isEmpty()) {
            // Showing the project file and beneath the project dir is pointless in this case,
            // so get rid of the root node and modify the project dir node as the new root node.
            projectDirTree->name = m_root->name;
            projectDirTree->fullPath = m_root->fullPath;
            projectDirTree->parent = m_root->parent;

            delete m_root; // OK, it has no files / child dirs.

            m_root = projectDirTree;
        } else {
            // Set up project dir node as sub node of the project file node
            linkDirNode(m_root, projectDirTree);

            // Add files outside of the base directory to a separate node
            Tree *externalFilesNode = createDirNode(SelectableFilesDialog::tr(
                                                        "Files outside of the base directory"),
                                                    FileName::fromString("/"));
            linkDirNode(m_root, externalFilesNode);
            for (const FileInfo &fileInfo : outOfBaseDirFiles)
                linkFileNode(externalFilesNode, createFileNode(fileInfo, true));
        }
    }

    static Tree *buildProjectDirTree(const FileName &projectDir,
                                     const FileInfos &fileInfos,
                                     FileInfos &outOfBaseDirFiles)
    {
        Tree *projectDirNode = createDirNode(projectDir.fileName(), projectDir);

        QHash<FileName, Tree *> dirsToNode;
        dirsToNode.insert(projectDirNode->fullPath, projectDirNode);

        for (const FileInfo &fileInfo : fileInfos) {
            if (!fileInfo.file.isChildOf(projectDirNode->fullPath)) {
                outOfBaseDirFiles += fileInfo;
                continue; // Handle these separately.
            }

            // Find or create parent nodes
            FileName parentDir = fileInfo.file.parentDir();
            Tree *parentNode = dirsToNode[parentDir];
            if (!parentNode) {
                // Find nearest existing node
                QStringList dirsToCreate;
                while (!parentNode) {
                    dirsToCreate.prepend(parentDir.fileName());
                    parentDir = parentDir.parentDir();
                    parentNode = dirsToNode[parentDir];
                }

                // Create needed extra dir nodes
                FileName currentDirPath = parentDir;
                for (const QString &dirName : dirsToCreate) {
                    currentDirPath.appendPath(dirName);

                    Tree *newDirNode = createDirNode(dirName, currentDirPath);
                    linkDirNode(parentNode, newDirNode);

                    dirsToNode.insert(currentDirPath, newDirNode);
                    parentNode = newDirNode;
                }
            }

            // Create and link file node to dir node
            linkFileNode(parentNode, createFileNode(fileInfo));
        }

        return projectDirNode;
    }

    FileInfos selectedFileInfos() const
    {
        FileInfos result;
        collectFileInfos(m_root, &result);
        return result;
    }

private:
    void collectFileInfos(ProjectExplorer::Tree *root, FileInfos *result) const
    {
        if (root->checked == Qt::Unchecked)
            return;
        for (Tree *t : root->childDirectories)
            collectFileInfos(t, result);
        for (Tree *t : root->visibleFiles) {
            if (t->checked == Qt::Checked)
                result->append(static_cast<TreeWithFileInfo *>(t)->info);
        }
    }
};

SelectableFilesDialog::SelectableFilesDialog(const ProjectInfo &projectInfo,
                                             const FileInfos &allFileInfos)
    : QDialog(nullptr)
    , m_ui(new Ui::SelectableFilesDialog)
    , m_filesModel(new SelectableFilesModel(projectInfo, allFileInfos))
    , m_analyzeButton(new QPushButton(tr("Analyze"), this))
{
    m_ui->setupUi(this);

    m_ui->filesView->setModel(m_filesModel.get());
    m_ui->filesView->expandToDepth(2);

    m_ui->buttons->setStandardButtons(QDialogButtonBox::Cancel);
    m_ui->buttons->addButton(m_analyzeButton, QDialogButtonBox::AcceptRole);

    m_analyzeButton->setEnabled(m_filesModel->hasCheckedFiles());
    connect(m_filesModel.get(), &QAbstractItemModel::dataChanged, [this]() {
        m_analyzeButton->setEnabled(m_filesModel->hasCheckedFiles());
    });
}

SelectableFilesDialog::~SelectableFilesDialog() {}

FileInfos SelectableFilesDialog::filteredFileInfos() const
{
    return m_filesModel->selectedFileInfos();
}

} // namespace Internal
} // namespace ClangTools

#include "clangselectablefilesdialog.moc"

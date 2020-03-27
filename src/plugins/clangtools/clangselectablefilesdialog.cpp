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

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/find/itemviewfind.h>
#include <cpptools/projectinfo.h>
#include <projectexplorer/selectablefilesmodel.h>
#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItem>

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

static Tree *createDirNode(const QString &name, const FilePath &filePath = FilePath())
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
    SelectableFilesModel()
        : ProjectExplorer::SelectableFilesModel(nullptr)
    {}

    void buildTree(ProjectExplorer::Project *project, const FileInfos &fileInfos)
    {
        beginResetModel();
        m_root->fullPath = project->projectFilePath();
        m_root->name = project->projectFilePath().fileName();
        m_root->isDir = true;

        FileInfos outOfBaseDirFiles;
        Tree *projectDirTree = buildProjectDirTree(project->projectDirectory(),
                                                   fileInfos,
                                                   outOfBaseDirFiles);
        if (outOfBaseDirFiles.empty()) {
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
                                                    FilePath::fromString("/"));
            linkDirNode(m_root, externalFilesNode);
            for (const FileInfo &fileInfo : outOfBaseDirFiles)
                linkFileNode(externalFilesNode, createFileNode(fileInfo, true));
        }
        endResetModel();
    }

    // Returns the minimal selection that can restore all selected files.
    //
    // For example, if a directory node if fully checked, there is no need to
    // save all the children of that node.
    void minimalSelection(FileInfoSelection &selection) const
    {
        selection.dirs.clear();
        selection.files.clear();
        traverse(index(0, 0, QModelIndex()), [&](const QModelIndex &index){
            auto node = static_cast<Tree *>(index.internalPointer());

            if (node->checked == Qt::Checked) {
                if (node->isDir) {
                    selection.dirs += node->fullPath;
                    return false; // Do not descend further.
                }

                selection.files += node->fullPath;
            }

            return true;
        });
    }

    void restoreMinimalSelection(const FileInfoSelection &selection)
    {
        if (selection.dirs.isEmpty() && selection.files.isEmpty())
            return;

        traverse(index(0, 0, QModelIndex()), [&](const QModelIndex &index){
            auto node = static_cast<Tree *>(index.internalPointer());

            if (node->isDir && selection.dirs.contains(node->fullPath)) {
                setData(index, Qt::Checked, Qt::CheckStateRole);
                return false; // Do not descend further.
            }

            if (!node->isDir && selection.files.contains(node->fullPath))
                setData(index, Qt::Checked, Qt::CheckStateRole);

            return true;
        });
    }

    FileInfos selectedFileInfos() const
    {
        FileInfos result;

        traverse(index(0, 0, QModelIndex()), [&](const QModelIndex &index){
            auto node = static_cast<Tree *>(index.internalPointer());

            if (node->checked == Qt::Unchecked)
                return false;

            if (!node->isDir)
                result.push_back(static_cast<TreeWithFileInfo *>(node)->info);

            return true;
        });

        return result;
    }

private:
    void traverse(const QModelIndex &index,
                  const std::function<bool(const QModelIndex &)> &visit) const
    {
        if (!index.isValid())
            return;

        if (!visit(index))
            return;

        if (!hasChildren(index))
            return;

        const int rows = rowCount(index);
        const int cols = columnCount(index);
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j)
                traverse(this->index(i, j, index), visit);
        }
    }

    Tree *buildProjectDirTree(const FilePath &projectDir,
                              const FileInfos &fileInfos,
                              FileInfos &outOfBaseDirFiles) const
    {
        Tree *projectDirNode = createDirNode(projectDir.fileName(), projectDir);

        QHash<FilePath, Tree *> dirsToNode;
        dirsToNode.insert(projectDirNode->fullPath, projectDirNode);

        for (const FileInfo &fileInfo : fileInfos) {
            if (!fileInfo.file.isChildOf(projectDirNode->fullPath)) {
                outOfBaseDirFiles.push_back(fileInfo);
                continue; // Handle these separately.
            }

            // Find or create parent nodes
            FilePath parentDir = fileInfo.file.parentDir();
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
                FilePath currentDirPath = parentDir;
                for (const QString &dirName : dirsToCreate) {
                    currentDirPath = currentDirPath.pathAppended(dirName);

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
};

SelectableFilesDialog::SelectableFilesDialog(const ProjectInfo &projectInfo,
                                             const FileInfoProviders &fileInfoProviders,
                                             int initialProviderIndex)
    : QDialog(nullptr)
    , m_ui(new Ui::SelectableFilesDialog)
    , m_filesModel(new SelectableFilesModel)
    , m_fileInfoProviders(fileInfoProviders)
    , m_project(projectInfo.project())
    , m_analyzeButton(new QPushButton(tr("Analyze"), this))
{
    m_ui->setupUi(this);

    // Files View
    // Make find actions available in this dialog, e.g. Strg+F for the view.
    addAction(Core::ActionManager::command(Core::Constants::FIND_IN_DOCUMENT)->action());
    addAction(Core::ActionManager::command(Core::Constants::FIND_NEXT)->action());
    addAction(Core::ActionManager::command(Core::Constants::FIND_PREVIOUS)->action());
    m_fileView = new QTreeView;
    m_fileView->setHeaderHidden(true);
    m_fileView->setModel(m_filesModel.get());
    m_ui->verticalLayout->addWidget(
        Core::ItemViewFind::createSearchableWrapper(m_fileView, Core::ItemViewFind::LightColored));

    // Filter combo box
    for (const FileInfoProvider &provider : m_fileInfoProviders) {
        m_ui->fileFilterComboBox->addItem(provider.displayName);

        // Disable item if it has no file infos
        auto *model = qobject_cast<QStandardItemModel *>(m_ui->fileFilterComboBox->model());
        QStandardItem *item = model->item(m_ui->fileFilterComboBox->count() - 1);
        item->setFlags(provider.fileInfos.empty() ? item->flags() & ~Qt::ItemIsEnabled
                                                  : item->flags() | Qt::ItemIsEnabled);
    }
    int providerIndex = initialProviderIndex;
    if (m_fileInfoProviders[providerIndex].fileInfos.empty())
        providerIndex = 0;
    m_ui->fileFilterComboBox->setCurrentIndex(providerIndex);
    onFileFilterChanged(providerIndex);
    connect(m_ui->fileFilterComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &SelectableFilesDialog::onFileFilterChanged);

    // Buttons
    m_buttons = new QDialogButtonBox;
    m_buttons->setStandardButtons(QDialogButtonBox::Cancel);
    m_buttons->addButton(m_analyzeButton, QDialogButtonBox::AcceptRole);\
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    m_analyzeButton->setEnabled(m_filesModel->hasCheckedFiles());
    connect(m_filesModel.get(), &QAbstractItemModel::dataChanged, [this]() {
        m_analyzeButton->setEnabled(m_filesModel->hasCheckedFiles());
    });
    m_ui->verticalLayout->addWidget(m_buttons);
}

SelectableFilesDialog::~SelectableFilesDialog() = default;

FileInfos SelectableFilesDialog::fileInfos() const
{
    return m_filesModel->selectedFileInfos();
}

int SelectableFilesDialog::currentProviderIndex() const
{
    return m_ui->fileFilterComboBox->currentIndex();
}

void SelectableFilesDialog::onFileFilterChanged(int index)
{
    // Remember previous filter/selection
    if (m_previousProviderIndex != -1)
        m_filesModel->minimalSelection(m_fileInfoProviders[m_previousProviderIndex].selection);
    m_previousProviderIndex = index;

    // Reset model
    const FileInfoProvider &provider = m_fileInfoProviders[index];
    m_filesModel->buildTree(m_project, provider.fileInfos);

    // Expand
    if (provider.expandPolicy == FileInfoProvider::All)
        m_fileView->expandAll();
    else
        m_fileView->expandToDepth(2);

    // Handle selection
    if (provider.selection.dirs.isEmpty() && provider.selection.files.isEmpty())
        m_filesModel->selectAllFiles(); // Initially, all files are selected
    else
        m_filesModel->restoreMinimalSelection(provider.selection);
}

void SelectableFilesDialog::accept()
{
    FileInfoSelection selection;
    m_filesModel->minimalSelection(selection);
    FileInfoProvider &provider = m_fileInfoProviders[m_ui->fileFilterComboBox->currentIndex()];
    provider.onSelectionAccepted(selection);

    QDialog::accept();
}

} // namespace Internal
} // namespace ClangTools

#include "clangselectablefilesdialog.moc"

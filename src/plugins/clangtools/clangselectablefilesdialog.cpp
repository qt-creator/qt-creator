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
#include "ui_clangtoolsbasicsettings.h"

#include "clangtoolsprojectsettings.h"
#include "clangtoolssettings.h"
#include "clangtoolsutils.h"

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsreuse.h>
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

    // Returns the minimal selection that can restore all selected files.
    //
    // For example, if a directory node if fully checked, there is no need to
    // save all the children of that node.
    void minimalSelection(QSet<FileName> &checkedDirs, QSet<FileName> &checkedFiles) const
    {
        traverse(index(0, 0, QModelIndex()), [&](const QModelIndex &index){
            auto node = static_cast<Tree *>(index.internalPointer());

            if (node->checked == Qt::Checked) {
                if (node->isDir) {
                    checkedDirs += node->fullPath;
                    return false; // Do not descend further.
                }

                checkedFiles += node->fullPath;
            }

            return true;
        });
    }

    void restoreMinimalSelection(const QSet<FileName> &dirs, const QSet<FileName> &files)
    {
        if (dirs.isEmpty() && files.isEmpty())
            return;

        traverse(index(0, 0, QModelIndex()), [&](const QModelIndex &index){
            auto node = static_cast<Tree *>(index.internalPointer());

            if (node->isDir && dirs.contains(node->fullPath)) {
                setData(index, Qt::Checked, Qt::CheckStateRole);
                return false; // Do not descend further.
            }

            if (!node->isDir && files.contains(node->fullPath))
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

    void buildTree(ProjectExplorer::Project *project, const FileInfos &fileInfos)
    {
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
                                                    FileName::fromString("/"));
            linkDirNode(m_root, externalFilesNode);
            for (const FileInfo &fileInfo : outOfBaseDirFiles)
                linkFileNode(externalFilesNode, createFileNode(fileInfo, true));
        }
    }

    Tree *buildProjectDirTree(const FileName &projectDir,
                              const FileInfos &fileInfos,
                              FileInfos &outOfBaseDirFiles) const
    {
        Tree *projectDirNode = createDirNode(projectDir.fileName(), projectDir);

        QHash<FileName, Tree *> dirsToNode;
        dirsToNode.insert(projectDirNode->fullPath, projectDirNode);

        for (const FileInfo &fileInfo : fileInfos) {
            if (!fileInfo.file.isChildOf(projectDirNode->fullPath)) {
                outOfBaseDirFiles.push_back(fileInfo);
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
};

enum { GlobalSettings , CustomSettings };

static Core::Id diagnosticConfiguration(ClangToolsProjectSettings *settings)
{
    Core::Id id = settings->diagnosticConfig();
    if (id.isValid())
        return id;
    return ClangToolsSettings::instance()->savedDiagnosticConfigId();
}

SelectableFilesDialog::SelectableFilesDialog(const ProjectInfo &projectInfo,
                                             const FileInfos &allFileInfos)
    : QDialog(nullptr)
    , m_ui(new Ui::SelectableFilesDialog)
    , m_filesModel(new SelectableFilesModel(projectInfo, allFileInfos))
    , m_project(projectInfo.project())
    , m_analyzeButton(new QPushButton(tr("Analyze"), this))
{
    m_ui->setupUi(this);

    m_ui->filesView->setModel(m_filesModel.get());
    m_ui->filesView->expandToDepth(2);

    m_ui->buttons->setStandardButtons(QDialogButtonBox::Cancel);
    m_ui->buttons->addButton(m_analyzeButton, QDialogButtonBox::AcceptRole);

    CppTools::ClangDiagnosticConfigsSelectionWidget *diagnosticConfigsSelectionWidget
            = m_ui->clangToolsBasicSettings->ui()->clangDiagnosticConfigsSelectionWidget;
    QCheckBox *buildBeforeAnalysis = m_ui->clangToolsBasicSettings->ui()->buildBeforeAnalysis;

    ClangToolsProjectSettings *settings = ClangToolsProjectSettingsManager::getSettings(m_project);
    m_customDiagnosticConfig = diagnosticConfiguration(settings);
    m_buildBeforeAnalysis = settings->buildBeforeAnalysis();

    if (settings->useGlobalSettings()) {
        m_ui->globalOrCustom->setCurrentIndex(GlobalSettings);
        m_ui->clangToolsBasicSettings->setEnabled(false);
        diagnosticConfigsSelectionWidget->refresh(
                    ClangToolsSettings::instance()->savedDiagnosticConfigId());
        buildBeforeAnalysis->setCheckState(
                    ClangToolsSettings::instance()->savedBuildBeforeAnalysis()
                    ? Qt::Checked : Qt::Unchecked);
    } else {
        m_ui->globalOrCustom->setCurrentIndex(CustomSettings);
        m_ui->clangToolsBasicSettings->setEnabled(true);
        diagnosticConfigsSelectionWidget->refresh(m_customDiagnosticConfig);
        buildBeforeAnalysis->setCheckState(m_buildBeforeAnalysis ? Qt::Checked : Qt::Unchecked);
    }

    connect(m_ui->globalOrCustom,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [=](int index){
        m_ui->clangToolsBasicSettings->setEnabled(index == CustomSettings);
        if (index == CustomSettings) {
            diagnosticConfigsSelectionWidget->refresh(m_customDiagnosticConfig);
            buildBeforeAnalysis->setCheckState(m_buildBeforeAnalysis ? Qt::Checked : Qt::Unchecked);
        } else {
            diagnosticConfigsSelectionWidget->refresh(
                ClangToolsSettings::instance()->savedDiagnosticConfigId());
            buildBeforeAnalysis->setCheckState(
                        ClangToolsSettings::instance()->savedBuildBeforeAnalysis()
                        ? Qt::Checked : Qt::Unchecked);
        }
    });
    connect(diagnosticConfigsSelectionWidget,
            &ClangDiagnosticConfigsSelectionWidget::currentConfigChanged,
            [this](const Core::Id &currentConfigId) {
        if (m_ui->globalOrCustom->currentIndex() == CustomSettings)
            m_customDiagnosticConfig = currentConfigId;
    });
    connect(buildBeforeAnalysis, &QCheckBox::toggled, [this](bool checked) {
        if (m_ui->globalOrCustom->currentIndex() == CustomSettings)
            m_buildBeforeAnalysis = checked;
    });

    // Restore selection
    if (settings->selectedDirs().isEmpty() && settings->selectedFiles().isEmpty())
        m_filesModel->selectAllFiles(); // Initially, all files are selected
    else // Restore selection
        m_filesModel->restoreMinimalSelection(settings->selectedDirs(), settings->selectedFiles());

    m_analyzeButton->setEnabled(m_filesModel->hasCheckedFiles());
    connect(m_filesModel.get(), &QAbstractItemModel::dataChanged, [this]() {
        m_analyzeButton->setEnabled(m_filesModel->hasCheckedFiles());
    });

    connect(CppTools::codeModelSettings().data(), &CppTools::CppCodeModelSettings::changed,
            this, [=]() {
        if (m_ui->globalOrCustom->currentIndex() == CustomSettings) {
            diagnosticConfigsSelectionWidget->refresh(m_customDiagnosticConfig);
        } else {
            diagnosticConfigsSelectionWidget->refresh(
                ClangToolsSettings::instance()->savedDiagnosticConfigId());
        }
    });
}

SelectableFilesDialog::~SelectableFilesDialog() = default;

FileInfos SelectableFilesDialog::filteredFileInfos() const
{
    return m_filesModel->selectedFileInfos();
}

void SelectableFilesDialog::accept()
{
    ClangToolsProjectSettings *settings = ClangToolsProjectSettingsManager::getSettings(m_project);

    // Save diagnostic configuration and flag to build before analysis
    settings->setUseGlobalSettings(m_ui->globalOrCustom->currentIndex() == GlobalSettings);
    settings->setDiagnosticConfig(m_customDiagnosticConfig);
    settings->setBuildBeforeAnalysis(m_buildBeforeAnalysis);

    // Save selection
    QSet<FileName> checkedDirs;
    QSet<FileName> checkedFiles;
    m_filesModel->minimalSelection(checkedDirs, checkedFiles);
    settings->setSelectedDirs(checkedDirs);
    settings->setSelectedFiles(checkedFiles);

    QDialog::accept();
}

} // namespace Internal
} // namespace ClangTools

#include "clangselectablefilesdialog.moc"

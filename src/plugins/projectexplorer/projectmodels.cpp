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

#include "projectmodels.h"

#include "buildsystem.h"
#include "project.h"
#include "projectnodes.h"
#include "projectexplorer.h"
#include "projecttree.h"
#include "session.h"
#include "target.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <utils/utilsicons.h>
#include <utils/algorithm.h>
#include <utils/dropsupport.h>
#include <utils/pathchooser.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <QButtonGroup>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QLoggingCategory>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include <functional>
#include <tuple>
#include <vector>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

bool compareNodes(const Node *n1, const Node *n2)
{
    if (n1->priority() > n2->priority())
        return true;
    if (n1->priority() < n2->priority())
        return false;

    const int displayNameResult = caseFriendlyCompare(n1->displayName(), n2->displayName());
    if (displayNameResult != 0)
        return displayNameResult < 0;

    const int filePathResult = caseFriendlyCompare(n1->filePath().toString(),
                                 n2->filePath().toString());
    if (filePathResult != 0)
        return filePathResult < 0;
    return n1 < n2; // sort by pointer value
}

static bool sortWrapperNodes(const WrapperNode *w1, const WrapperNode *w2)
{
    return compareNodes(w1->m_node, w2->m_node);
}

FlatModel::FlatModel(QObject *parent)
    : TreeModel<WrapperNode, WrapperNode>(new WrapperNode(nullptr), parent)
{
    ProjectTree *tree = ProjectTree::instance();
    connect(tree, &ProjectTree::subtreeChanged, this, &FlatModel::updateSubtree);

    SessionManager *sm = SessionManager::instance();
    connect(sm, &SessionManager::projectRemoved, this, &FlatModel::handleProjectRemoved);
    connect(sm, &SessionManager::aboutToLoadSession, this, &FlatModel::loadExpandData);
    connect(sm, &SessionManager::aboutToSaveSession, this, &FlatModel::saveExpandData);
    connect(sm, &SessionManager::projectAdded, this, &FlatModel::handleProjectAdded);
    connect(sm, &SessionManager::startupProjectChanged, this, [this] { emit layoutChanged(); });

    for (Project *project : SessionManager::projects())
        handleProjectAdded(project);

    m_disabledTextColor = Utils::creatorTheme()->color(Utils::Theme::TextColorDisabled);
    m_enabledTextColor = Utils::creatorTheme()->color(Utils::Theme::TextColorNormal);
}

QVariant FlatModel::data(const QModelIndex &index, int role) const
{
    const Node * const node = nodeForIndex(index);
    if (!node)
        return QVariant();

    const FolderNode * const folderNode = node->asFolderNode();
    const ContainerNode * const containerNode = node->asContainerNode();
    const Project * const project = containerNode ? containerNode->project() : nullptr;
    const Target * const target = project ? project->activeTarget() : nullptr;
    const BuildSystem * const bs = target ? target->buildSystem() : nullptr;

    switch (role) {
    case Qt::DisplayRole:
        return node->displayName();
    case Qt::EditRole:
        return node->filePath().fileName();
    case Qt::ToolTipRole: {
        QString tooltip = node->tooltip();
        if (project) {
            if (target) {
                QString projectIssues = toHtml(project->projectIssues(project->activeTarget()->kit()));
                if (!projectIssues.isEmpty())
                    tooltip += "<p>" + projectIssues;
            } else {
                tooltip += "<p>" + tr("No kits are enabled for this project. "
                                      "Enable kits in the \"Projects\" mode.");
            }
        }
        return tooltip;
    }
    case Qt::DecorationRole: {
        if (!folderNode)
            return Core::FileIconProvider::icon(node->filePath().toString());
        if (!project)
            return folderNode->icon();
        static QIcon warnIcon = Utils::Icons::WARNING.icon();
        static QIcon emptyIcon = Utils::Icons::EMPTY16.icon();
        if (project->needsConfiguration())
            return warnIcon;
        if (bs && bs->isParsing())
            return emptyIcon;
        if (!target || !project->projectIssues(target->kit()).isEmpty())
            return warnIcon;
        return containerNode->rootProjectNode() ? containerNode->rootProjectNode()->icon()
                                                : folderNode->icon();
    }
    case Qt::FontRole: {
        QFont font;
        if (project == SessionManager::startupProject())
            font.setBold(true);
        return font;
    }
    case Qt::ForegroundRole:
        return node->isEnabled() ? m_enabledTextColor : m_disabledTextColor;
    case Project::FilePathRole:
        return node->filePath().toString();
    case Project::isParsingRole:
        return project && bs ? bs->isParsing() && !project->needsConfiguration() : false;
    }

    return QVariant();
}

Qt::ItemFlags FlatModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};
    // We claim that everything is editable
    // That's slightly wrong
    // We control the only view, and that one does the checks
    Qt::ItemFlags f = Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsDragEnabled;
    if (Node *node = nodeForIndex(index)) {
        if (!node->asProjectNode()) {
            // either folder or file node
            if (node->supportsAction(Rename, node))
                f = f | Qt::ItemIsEditable;
        } else if (node->supportsAction(ProjectAction::AddExistingFile, node)) {
            f |= Qt::ItemIsDropEnabled;
        }
    }
    return f;
}

bool FlatModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    if (role != Qt::EditRole)
        return false;

    Node *node = nodeForIndex(index);
    QTC_ASSERT(node, return false);

    std::vector<std::tuple<Node *, FilePath, FilePath>> toRename;
    const Utils::FilePath orgFilePath = node->filePath();
    const Utils::FilePath newFilePath = orgFilePath.parentDir().pathAppended(value.toString());
    const QFileInfo orgFileInfo = orgFilePath.toFileInfo();
    toRename.emplace_back(std::make_tuple(node, orgFilePath, newFilePath));

    // The base name of the file was changed. Go look for other files with the same base name
    // and offer to rename them as well.
    if (orgFilePath != newFilePath && orgFileInfo.suffix() == newFilePath.toFileInfo().suffix()) {
        const QList<Node *> candidateNodes = ProjectTree::siblingsWithSameBaseName(node);
        if (!candidateNodes.isEmpty()) {
            const QMessageBox::StandardButton reply = QMessageBox::question(
                        Core::ICore::dialogParent(), tr("Rename More Files?"),
                        tr("Would you like to rename these files as well?\n    %1")
                        .arg(transform<QStringList>(candidateNodes, [](const Node *n) {
                                 return n->filePath().toFileInfo().fileName();
                             }).join("\n    ")));
            if (reply == QMessageBox::Yes) {
                for (Node * const n : candidateNodes) {
                    QString targetFilePath = orgFileInfo.absolutePath() + '/'
                            + newFilePath.toFileInfo().completeBaseName();
                    const QString suffix = n->filePath().toFileInfo().suffix();
                    if (!suffix.isEmpty())
                        targetFilePath.append('.').append(suffix);
                    toRename.emplace_back(std::make_tuple(n, n->filePath(),
                                                          FilePath::fromString(targetFilePath)));
                }
            }
        }
    }

    for (const auto &f : toRename) {
        ProjectExplorerPlugin::renameFile(std::get<0>(f), std::get<2>(f).toString());
        emit renamed(std::get<1>(f), std::get<2>(f));
    }
    return true;
}

static bool compareProjectNames(const WrapperNode *lhs, const WrapperNode *rhs)
{
    Node *p1 = lhs->m_node;
    Node *p2 = rhs->m_node;
    const int displayNameResult = caseFriendlyCompare(p1->displayName(), p2->displayName());
    if (displayNameResult != 0)
        return displayNameResult < 0;
    return p1 < p2; // sort by pointer value
}

void FlatModel::addOrRebuildProjectModel(Project *project)
{
    WrapperNode *container = nodeForProject(project);
    if (container) {
        container->removeChildren();
        project->containerNode()->removeAllChildren();
    } else {
        container = new WrapperNode(project->containerNode());
        rootItem()->insertOrderedChild(container, &compareProjectNames);
    }

    QSet<Node *> seen;

    if (ProjectNode *projectNode = project->rootProjectNode()) {
        addFolderNode(container, projectNode, &seen);
        if (m_trimEmptyDirectories)
            trimEmptyDirectories(container);
    }

    if (project->needsInitialExpansion())
        m_toExpand.insert(expandDataForNode(container->m_node));

    if (container->childCount() == 0) {
        auto projectFileNode = std::make_unique<FileNode>(project->projectFilePath(),
                                                          FileType::Project);
        seen.insert(projectFileNode.get());
        container->appendChild(new WrapperNode(projectFileNode.get()));
        project->containerNode()->addNestedNode(std::move(projectFileNode));
    }

    container->sortChildren(&sortWrapperNodes);

    container->forAllChildren([this](WrapperNode *node) {
        if (node->m_node) {
            const QString path = node->m_node->filePath().toString();
            const QString displayName = node->m_node->displayName();
            ExpandData ed(path, displayName);
            if (m_toExpand.contains(ed))
                emit requestExpansion(node->index());
        } else {
            emit requestExpansion(node->index());
        }
    });

    const QString path = container->m_node->filePath().toString();
    const QString displayName = container->m_node->displayName();
    ExpandData ed(path, displayName);
    if (m_toExpand.contains(ed))
        emit requestExpansion(container->index());
}

void FlatModel::parsingStateChanged(Project *project)
{
    const WrapperNode *const node = nodeForProject(project);
    const QModelIndex nodeIdx = indexForNode(node->m_node);
    emit dataChanged(nodeIdx, nodeIdx);
}

void FlatModel::updateSubtree(FolderNode *node)
{
    // FIXME: This is still excessive, should be limited to the affected subtree.
    while (FolderNode *parent = node->parentFolderNode())
        node = parent;
    if (ContainerNode *container = node->asContainerNode())
        addOrRebuildProjectModel(container->project());
}

void FlatModel::rebuildModel()
{
    const QList<Project *> projects = SessionManager::projects();
    for (Project *project : projects)
        addOrRebuildProjectModel(project);
}

void FlatModel::onCollapsed(const QModelIndex &idx)
{
    m_toExpand.remove(expandDataForNode(nodeForIndex(idx)));
}

void FlatModel::onExpanded(const QModelIndex &idx)
{
    m_toExpand.insert(expandDataForNode(nodeForIndex(idx)));
}

ExpandData FlatModel::expandDataForNode(const Node *node) const
{
    QTC_ASSERT(node, return ExpandData());
    const QString path = node->filePath().toString();
    const QString displayName = node->displayName();
    return ExpandData(path, displayName);
}

void FlatModel::handleProjectAdded(Project *project)
{
    QTC_ASSERT(project, return);

    connect(project, &Project::anyParsingStarted,
            this, [this, project]() {
        if (nodeForProject(project))
            parsingStateChanged(project);
    });
    connect(project, &Project::anyParsingFinished,
            this, [this, project]() {
        if (nodeForProject(project))
            parsingStateChanged(project);
        emit ProjectTree::instance()->nodeActionsChanged();
    });
    addOrRebuildProjectModel(project);
}

void FlatModel::handleProjectRemoved(Project *project)
{
    destroyItem(nodeForProject(project));
}

WrapperNode *FlatModel::nodeForProject(const Project *project) const
{
    QTC_ASSERT(project, return nullptr);
    ContainerNode *containerNode = project->containerNode();
    QTC_ASSERT(containerNode, return nullptr);
    return rootItem()->findFirstLevelChild([containerNode](WrapperNode *node) {
        return node->m_node == containerNode;
    });
}

void FlatModel::loadExpandData()
{
    const QList<QVariant> data = SessionManager::value("ProjectTree.ExpandData").value<QList<QVariant>>();
    m_toExpand = Utils::transform<QSet>(data, &ExpandData::fromSettings);
    m_toExpand.remove(ExpandData());
}

void FlatModel::saveExpandData()
{
    // TODO if there are multiple ProjectTreeWidgets, the last one saves the data
    QList<QVariant> data = Utils::transform<QList>(m_toExpand, &ExpandData::toSettings);
    SessionManager::setValue(QLatin1String("ProjectTree.ExpandData"), data);
}

void FlatModel::addFolderNode(WrapperNode *parent, FolderNode *folderNode, QSet<Node *> *seen)
{
    for (Node *node : folderNode->nodes()) {
        if (m_filterGeneratedFiles && node->isGenerated())
            continue;
        if (m_filterDisabledFiles && !node->isEnabled())
            continue;
        if (FolderNode *subFolderNode = node->asFolderNode()) {
            const bool isHidden = m_filterProjects && !subFolderNode->showInSimpleTree();
            if (!isHidden && !seen->contains(subFolderNode)) {
                seen->insert(subFolderNode);
                auto node = new WrapperNode(subFolderNode);
                parent->appendChild(node);
                addFolderNode(node, subFolderNode, seen);
                node->sortChildren(&sortWrapperNodes);
            } else {
                addFolderNode(parent, subFolderNode, seen);
            }
        } else if (FileNode *fileNode = node->asFileNode()) {
            if (!seen->contains(fileNode)) {
                seen->insert(fileNode);
                parent->appendChild(new WrapperNode(fileNode));
            }
        }
    }
}

bool FlatModel::trimEmptyDirectories(WrapperNode *parent)
{
    const FolderNode *fn = parent->m_node->asFolderNode();
    if (!fn)
        return false;

    for (int i = parent->childCount() - 1; i >= 0; --i) {
        if (trimEmptyDirectories(parent->childAt(i)))
            parent->removeChildAt(i);
    }
    return parent->childCount() == 0 && !fn->showWhenEmpty();
}

Qt::DropActions FlatModel::supportedDragActions() const
{
    return Qt::CopyAction;
}

QStringList FlatModel::mimeTypes() const
{
    return Utils::DropSupport::mimeTypesForFilePaths();
}

QMimeData *FlatModel::mimeData(const QModelIndexList &indexes) const
{
    auto data = new DropMimeData;
    foreach (const QModelIndex &index, indexes) {
        if (Node *node = nodeForIndex(index)) {
            if (node->asFileNode())
                data->addFile(node->filePath().toString());
            data->addValue(QVariant::fromValue(node));
        }
    }
    return data;
}

bool FlatModel::canDropMimeData(const QMimeData *data, Qt::DropAction, int, int,
                                const QModelIndex &) const
{
    // For now, we support only drops of Qt Creator file nodes.
    const auto * const dropData = dynamic_cast<const DropMimeData *>(data);
    if (!dropData)
        return false;
    QTC_ASSERT(!dropData->values().empty(), return false);
    return dropData->files().size() == dropData->values().size();
}

enum class DropAction { Copy, CopyWithFiles, Move, MoveWithFiles };

class DropFileDialog : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::Internal::FlatModel)
public:
    DropFileDialog(const FilePath &defaultTargetDir)
        : m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel)),
          m_buttonGroup(new QButtonGroup(this))
    {
        setWindowTitle(tr("Choose Drop Action"));
        const bool offerFileIo = !defaultTargetDir.isEmpty();
        auto * const layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel(tr("You just dragged some files from one project node to "
                                        "another.\nWhat should Qt Creator do now?"), this));
        auto * const copyButton = new QRadioButton(this);
        m_buttonGroup->addButton(copyButton, int(DropAction::Copy));
        layout->addWidget(copyButton);
        auto * const moveButton = new QRadioButton(this);
        m_buttonGroup->addButton(moveButton, int(DropAction::Move));
        layout->addWidget(moveButton);
        if (offerFileIo) {
            copyButton->setText(tr("Copy Only File References"));
            moveButton->setText(tr("Move Only File References"));
            auto * const copyWithFilesButton
                    = new QRadioButton(tr("Copy file references and files"), this);
            m_buttonGroup->addButton(copyWithFilesButton, int(DropAction::CopyWithFiles));
            layout->addWidget(copyWithFilesButton);
            auto * const moveWithFilesButton
                    = new QRadioButton(tr("Move file references and files"), this);
            m_buttonGroup->addButton(moveWithFilesButton, int(DropAction::MoveWithFiles));
            layout->addWidget(moveWithFilesButton);
            moveWithFilesButton->setChecked(true);
            auto * const targetDirLayout = new QHBoxLayout;
            layout->addLayout(targetDirLayout);
            targetDirLayout->addWidget(new QLabel(tr("Target directory:"), this));
            m_targetDirChooser = new PathChooser(this);
            m_targetDirChooser->setExpectedKind(PathChooser::ExistingDirectory);
            m_targetDirChooser->setFilePath(defaultTargetDir);
            connect(m_targetDirChooser, &PathChooser::validChanged, this, [this](bool valid) {
                m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
            });
            targetDirLayout->addWidget(m_targetDirChooser);
            connect(m_buttonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
                    this, [this] {
                switch (dropAction()) {
                case DropAction::CopyWithFiles:
                case DropAction::MoveWithFiles:
                    m_targetDirChooser->setEnabled(true);
                    m_buttonBox->button(QDialogButtonBox::Ok)
                            ->setEnabled(m_targetDirChooser->isValid());
                    break;
                case DropAction::Copy:
                case DropAction::Move:
                    m_targetDirChooser->setEnabled(false);
                    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
                    break;
                }
            });
        } else {
            copyButton->setText(tr("Copy File References"));
            moveButton->setText(tr("Move File References"));
            moveButton->setChecked(true);
        }
        connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(m_buttonBox);
    }

    DropAction dropAction() const { return static_cast<DropAction>(m_buttonGroup->checkedId()); }
    FilePath targetDir() const
    {
        return m_targetDirChooser ? m_targetDirChooser->filePath() : FilePath();
    }

private:
    PathChooser *m_targetDirChooser = nullptr;
    QDialogButtonBox * const m_buttonBox;
    QButtonGroup * const m_buttonGroup;
};


bool FlatModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                             const QModelIndex &parent)
{
    Q_UNUSED(action)

    const auto * const dropData = dynamic_cast<const DropMimeData *>(data);
    QTC_ASSERT(dropData, return false);

    auto fileNodes = transform<QList<const Node *>>(dropData->values(),
        [](const QVariant &v) { return v.value<Node *>(); });
    QTC_ASSERT(!fileNodes.empty(), return true);

    // The drag operation does not block event handling, so it's possible that the project
    // was reparsed and the nodes in the drop data are now invalid. If that happens for any node,
    // we chicken out and abort the entire operation.
    // Note: In theory, it might be possible that the memory was reused in such an unlucky
    //       way that the pointers refer to different project nodes now, but...
    if (!allOf(fileNodes, [](const Node *n) { return ProjectTree::hasNode(n); }))
        return true;

    // We handle only proper file nodes, i.e. no project or folder nodes and no "pseudo"
    // file nodes that represent the project file.
    fileNodes = filtered(fileNodes, [](const Node *n) {
        return n->asFileNode() && n->asFileNode()->fileType() != FileType::Project;
    });
    if (fileNodes.empty())
        return true;

    // We can handle more than one file being dropped, as long as they have the same parent node.
    ProjectNode * const sourceProjectNode = fileNodes.first()->parentProjectNode();
    QTC_ASSERT(sourceProjectNode, return true);
    if (anyOf(fileNodes, [sourceProjectNode](const Node *n) {
              return n->parentProjectNode() != sourceProjectNode; })) {
        return true;
    }
    Node *targetNode = nodeForIndex(index(row, column, parent));
    if (!targetNode)
        targetNode = nodeForIndex(parent);
    QTC_ASSERT(targetNode, return true);
    ProjectNode *targetProjectNode = targetNode->asProjectNode();
    if (!targetProjectNode)
        targetProjectNode = targetNode->parentProjectNode();
    QTC_ASSERT(targetProjectNode, return true);
    if (sourceProjectNode == targetProjectNode)
        return true;

    // Node weirdness: Sometimes the "file path" is a directory, sometimes it's a file...
    const auto dirForProjectNode = [](const ProjectNode *pNode) {
        const FilePath dir = pNode->filePath();
        if (dir.isDir())
            return dir;
        return FilePath::fromString(dir.toFileInfo().path());
    };
    FilePath targetDir = dirForProjectNode(targetProjectNode);

    // Ask the user what to do now: Copy or add? With or without file transfer?
    DropFileDialog dlg(targetDir == dirForProjectNode(sourceProjectNode) ? FilePath() : targetDir);
    if (dlg.exec() != QDialog::Accepted)
        return true;
    if (!dlg.targetDir().isEmpty())
        targetDir = dlg.targetDir();

    // Check the nodes again.
    if (!allOf(fileNodes, [](const Node *n) { return ProjectTree::hasNode(n); }))
        return true;

    // Some helper functions for the file operations.
    const auto targetFilePath = [&targetDir](const QString &sourceFilePath) {
        return targetDir.pathAppended(QFileInfo(sourceFilePath).fileName()).toString();
    };

    struct VcsInfo {
        Core::IVersionControl *vcs = nullptr;
        QString repoDir;
        bool operator==(const VcsInfo &other) const {
            return vcs == other.vcs && repoDir == other.repoDir;
        }
    };
    QHash<QString, VcsInfo> vcsHash;
    const auto vcsInfoForFile = [&vcsHash](const QString &filePath) {
        const QString dir = QFileInfo(filePath).path();
        const auto it = vcsHash.constFind(dir);
        if (it != vcsHash.constEnd())
            return it.value();
        VcsInfo vcsInfo;
        vcsInfo.vcs = Core::VcsManager::findVersionControlForDirectory(dir, &vcsInfo.repoDir);
        vcsHash.insert(dir, vcsInfo);
        return vcsInfo;
    };

    // Now do the actual work.
    const QStringList sourceFiles = transform(fileNodes, [](const Node *n) {
        return n->filePath().toString();
    });
    QStringList failedRemoveFromProject;
    QStringList failedAddToProject;
    QStringList failedCopyOrMove;
    QStringList failedDelete;
    QStringList failedVcsOp;
    switch (dlg.dropAction()) {
    case DropAction::CopyWithFiles: {
        QStringList filesToAdd;
        Core::IVersionControl * const vcs = Core::VcsManager::findVersionControlForDirectory(
                    targetDir.toString());
        const bool addToVcs = vcs && vcs->supportsOperation(Core::IVersionControl::AddOperation);
        for (const QString &sourceFile : sourceFiles) {
            const QString targetFile = targetFilePath(sourceFile);
            if (QFile::copy(sourceFile, targetFile)) {
                filesToAdd << targetFile;
                if (addToVcs && !vcs->vcsAdd(targetFile))
                    failedVcsOp << targetFile;
            } else {
                failedCopyOrMove << sourceFile;
            }
        }
        targetProjectNode->addFiles(filesToAdd, &failedAddToProject);
        break;
    }
    case DropAction::Copy:
        targetProjectNode->addFiles(sourceFiles, &failedAddToProject);
        break;
    case DropAction::MoveWithFiles: {
        QStringList filesToAdd;
        QStringList filesToRemove;
        const VcsInfo targetVcs = vcsInfoForFile(targetDir.toString());
        const bool vcsAddPossible = targetVcs.vcs
                && targetVcs.vcs->supportsOperation(Core::IVersionControl::AddOperation);
        for (const QString &sourceFile : sourceFiles) {
            const QString targetFile = targetFilePath(sourceFile);
            const VcsInfo sourceVcs = vcsInfoForFile(sourceFile);
            if (sourceVcs.vcs && targetVcs.vcs && sourceVcs == targetVcs
                    && sourceVcs.vcs->supportsOperation(Core::IVersionControl::MoveOperation)) {
                if (sourceVcs.vcs->vcsMove(sourceFile, targetFile)) {
                    filesToAdd << targetFile;
                    filesToRemove << sourceFile;
                } else {
                    failedCopyOrMove << sourceFile;
                }
                continue;
            }
            if (!QFile::copy(sourceFile, targetFile)) {
                failedCopyOrMove << sourceFile;
                continue;
            }
            filesToAdd << targetFile;
            filesToRemove << sourceFile;
            Core::FileChangeBlocker changeGuard(sourceFile);
            if (sourceVcs.vcs && sourceVcs.vcs->supportsOperation(
                        Core::IVersionControl::DeleteOperation)
                    && !sourceVcs.vcs->vcsDelete(sourceFile)) {
                failedVcsOp << sourceFile;
            }
            if (QFile::exists(sourceFile) && !QFile::remove(sourceFile))
                failedDelete << sourceFile;
            if (vcsAddPossible && !targetVcs.vcs->vcsAdd(targetFile))
                failedVcsOp << targetFile;
        }
        const RemovedFilesFromProject result
                = sourceProjectNode->removeFiles(filesToRemove, &failedRemoveFromProject);
        if (result == RemovedFilesFromProject::Wildcard)
            failedRemoveFromProject.clear();
        targetProjectNode->addFiles(filesToAdd, &failedAddToProject);
        break;
    }
    case DropAction::Move:
        sourceProjectNode->removeFiles(sourceFiles, &failedRemoveFromProject);
        targetProjectNode->addFiles(sourceFiles, &failedAddToProject);
        break;
    }

    // Summary for the user in case anything went wrong.
    const auto makeUserFileList = [](const QStringList &files) {
        return transform(files, [](const QString &f) { return QDir::toNativeSeparators(f); })
                .join("\n  ");
    };
    if (!failedAddToProject.empty() || !failedRemoveFromProject.empty()
            || !failedCopyOrMove.empty() || !failedDelete.empty() || !failedVcsOp.empty()) {
        QString message = tr("Not all operations finished successfully.");
        if (!failedCopyOrMove.empty()) {
            message.append('\n').append(tr("The following files could not be copied or moved:"))
                    .append("\n  ").append(makeUserFileList(failedCopyOrMove));
        }
        if (!failedRemoveFromProject.empty()) {
            message.append('\n').append(tr("The following files could not be removed from the "
                                           "project file:"))
                    .append("\n  ").append(makeUserFileList(failedRemoveFromProject));
        }
        if (!failedAddToProject.empty()) {
            message.append('\n').append(tr("The following files could not be added to the "
                                           "project file:"))
                    .append("\n  ").append(makeUserFileList(failedAddToProject));
        }
        if (!failedDelete.empty()) {
            message.append('\n').append(tr("The following files could not be deleted:"))
                    .append("\n  ").append(makeUserFileList(failedDelete));
        }
        if (!failedVcsOp.empty()) {
            message.append('\n').append(tr("A version control operation failed for the following "
                                           "files. Please check your repository."))
                    .append("\n  ").append(makeUserFileList(failedVcsOp));
        }
        QMessageBox::warning(Core::ICore::dialogParent(), tr("Failure Updating Project"), message);
    }

    return true;
}

WrapperNode *FlatModel::wrapperForNode(const Node *node) const
{
    return findNonRootItem([node](WrapperNode *item) {
        return item->m_node == node;
    });
}

QModelIndex FlatModel::indexForNode(const Node *node) const
{
    WrapperNode *wrapper = wrapperForNode(node);
    return wrapper ? indexForItem(wrapper) : QModelIndex();
}

void FlatModel::setProjectFilterEnabled(bool filter)
{
    if (filter == m_filterProjects)
        return;
    m_filterProjects = filter;
    rebuildModel();
}

void FlatModel::setGeneratedFilesFilterEnabled(bool filter)
{
    if (filter == m_filterGeneratedFiles)
        return;
    m_filterGeneratedFiles = filter;
    rebuildModel();
}

void FlatModel::setDisabledFilesFilterEnabled(bool filter)
{
    if (filter == m_filterDisabledFiles)
        return;
    m_filterDisabledFiles = filter;
    rebuildModel();
}

void FlatModel::setTrimEmptyDirectories(bool filter)
{
    if (filter == m_trimEmptyDirectories)
        return;
    m_trimEmptyDirectories = filter;
    rebuildModel();
}

bool FlatModel::projectFilterEnabled()
{
    return m_filterProjects;
}

bool FlatModel::generatedFilesFilterEnabled()
{
    return m_filterGeneratedFiles;
}

bool FlatModel::trimEmptyDirectoriesEnabled()
{
    return m_trimEmptyDirectories;
}

Node *FlatModel::nodeForIndex(const QModelIndex &index) const
{
    WrapperNode *flatNode = itemForIndex(index);
    return flatNode ? flatNode->m_node : nullptr;
}

const QLoggingCategory &FlatModel::logger()
{
    static QLoggingCategory logger("qtc.projectexplorer.flatmodel", QtWarningMsg);
    return logger;
}

} // namespace Internal
} // namespace ProjectExplorer


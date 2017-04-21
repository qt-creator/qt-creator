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

#include "projectwizardpage.h"
#include "ui_projectwizardpage.h"

#include "project.h"
#include "projectexplorer.h"
#include "session.h"

#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/iwizardfactory.h>
#include <coreplugin/vcsmanager.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/treemodel.h>
#include <utils/wizard.h>
#include <vcsbase/vcsbaseconstants.h>

#include <QDir>
#include <QTextStream>
#include <QTreeView>

/*!
    \class ProjectExplorer::Internal::ProjectWizardPage

    \brief The ProjectWizardPage class provides a wizard page showing projects
    and version control to add new files to.

    \sa ProjectExplorer::Internal::ProjectFileWizardExtension
*/

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class AddNewTree : public TreeItem
{
public:
    AddNewTree(const QString &displayName);
    AddNewTree(FolderNode *node, QList<AddNewTree *> children, const QString &displayName);
    AddNewTree(FolderNode *node, QList<AddNewTree *> children, const FolderNode::AddNewInformation &info);

    QVariant data(int column, int role) const;
    Qt::ItemFlags flags(int column) const;

    QString displayName() const { return m_displayName; }
    FolderNode *node() const { return m_node; }
    int priority() const { return m_priority; }

private:
    QString m_displayName;
    QString m_toolTip;
    FolderNode *m_node = nullptr;
    bool m_canAdd = true;
    int m_priority = -1;
};

AddNewTree::AddNewTree(const QString &displayName) :
    m_displayName(displayName)
{ }

// FIXME: potentially merge the following two functions.
// Note the different handling of 'node' and m_canAdd.
AddNewTree::AddNewTree(FolderNode *node, QList<AddNewTree *> children, const QString &displayName) :
    m_displayName(displayName),
    m_node(node),
    m_canAdd(false)
{
    if (node)
        m_toolTip = ProjectExplorerPlugin::directoryFor(node);
    foreach (AddNewTree *child, children)
        appendChild(child);
}

AddNewTree::AddNewTree(FolderNode *node, QList<AddNewTree *> children,
                       const FolderNode::AddNewInformation &info) :
    m_displayName(info.displayName),
    m_node(node),
    m_priority(info.priority)
{
    if (node)
        m_toolTip = ProjectExplorerPlugin::directoryFor(node);
    foreach (AddNewTree *child, children)
        appendChild(child);
}


QVariant AddNewTree::data(int, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        return m_displayName;
    case Qt::ToolTipRole:
        return m_toolTip;
    case Qt::UserRole:
        return QVariant::fromValue(static_cast<void*>(node()));
    default:
        return QVariant();
    }
}

Qt::ItemFlags AddNewTree::flags(int) const
{
    if (m_canAdd)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return Qt::NoItemFlags;
}

// --------------------------------------------------------------------
// BestNodeSelector:
// --------------------------------------------------------------------

class BestNodeSelector
{
public:
    BestNodeSelector(const QString &commonDirectory, const QStringList &files);
    void inspect(AddNewTree *tree, bool isContextNode);
    AddNewTree *bestChoice() const;
    bool deploys();
    QString deployingProjects() const;

private:
    QString m_commonDirectory;
    QStringList m_files;
    bool m_deploys = false;
    QString m_deployText;
    AddNewTree *m_bestChoice = nullptr;
    int m_bestMatchLength = -1;
    int m_bestMatchPriority = -1;
};

BestNodeSelector::BestNodeSelector(const QString &commonDirectory, const QStringList &files) :
    m_commonDirectory(commonDirectory),
    m_files(files),
    m_deployText(QCoreApplication::translate("ProjectWizard", "The files are implicitly added to the projects:") + QLatin1Char('\n'))
{ }

// Find the project the new files should be added
// If any node deploys the files, then we don't want to add the files.
// Otherwise consider their common path. Either a direct match on the directory
// or the directory with the longest matching path (list containing"/project/subproject1"
// matching common path "/project/subproject1/newuserpath").
void BestNodeSelector::inspect(AddNewTree *tree, bool isContextNode)
{
    FolderNode *node = tree->node();
    if (node->nodeType() == NodeType::Project) {
        if (static_cast<ProjectNode *>(node)->deploysFolder(m_commonDirectory)) {
            m_deploys = true;
            m_deployText += tree->displayName() + QLatin1Char('\n');
        }
    }
    if (m_deploys)
        return;

    const QString projectDirectory = ProjectExplorerPlugin::directoryFor(node);
    const int projectDirectorySize = projectDirectory.size();
    if (m_commonDirectory != projectDirectory
            && !m_commonDirectory.startsWith(projectDirectory + QLatin1Char('/'))
            && !isContextNode)
        return;

    bool betterMatch = isContextNode
            || (tree->priority() > 0
                && (projectDirectorySize > m_bestMatchLength
                    || (projectDirectorySize == m_bestMatchLength && tree->priority() > m_bestMatchPriority)));

    if (betterMatch) {
        m_bestMatchPriority = tree->priority();
        m_bestMatchLength = isContextNode ? std::numeric_limits<int>::max() : projectDirectorySize;
        m_bestChoice = tree;
    }
}

AddNewTree *BestNodeSelector::bestChoice() const
{
    if (m_deploys)
        return 0;
    return m_bestChoice;
}

bool BestNodeSelector::deploys()
{
    return m_deploys;
}

QString BestNodeSelector::deployingProjects() const
{
    if (m_deploys)
        return m_deployText;
    return QString();
}

// --------------------------------------------------------------------
// Helper:
// --------------------------------------------------------------------

static inline AddNewTree *createNoneNode(BestNodeSelector *selector)
{
    QString displayName = QCoreApplication::translate("ProjectWizard", "<None>");
    if (selector->deploys())
        displayName = QCoreApplication::translate("ProjectWizard", "<Implicitly Add>");
    return new AddNewTree(displayName);
}

static inline AddNewTree *buildAddProjectTree(ProjectNode *root, const QString &projectPath, Node *contextNode, BestNodeSelector *selector)
{
    QList<AddNewTree *> children;
    for (Node *node : root->nodes()) {
        if (ProjectNode *pn = node->asProjectNode()) {
            if (AddNewTree *child = buildAddProjectTree(pn, projectPath, contextNode, selector))
                children.append(child);
        }
    }

    if (root->supportsAction(AddSubProject, root) && !root->supportsAction(InheritedFromParent, root)) {
        if (projectPath.isEmpty() || root->canAddSubProject(projectPath)) {
            FolderNode::AddNewInformation info = root->addNewInformation(QStringList() << projectPath, contextNode);
            auto item = new AddNewTree(root, children, info);
            selector->inspect(item, root == contextNode);
            return item;
        }
    }

    if (children.isEmpty())
        return nullptr;
    return new AddNewTree(root, children, root->displayName());
}

static inline AddNewTree *buildAddFilesTree(FolderNode *root, const QStringList &files,
                                            Node *contextNode, BestNodeSelector *selector)
{
    QList<AddNewTree *> children;
    foreach (FolderNode *fn, root->folderNodes()) {
        AddNewTree *child = buildAddFilesTree(fn, files, contextNode, selector);
        if (child)
            children.append(child);
    }

    if (root->supportsAction(AddNewFile, root) && !root->supportsAction(InheritedFromParent, root)) {
        FolderNode::AddNewInformation info = root->addNewInformation(files, contextNode);
        auto item = new AddNewTree(root, children, info);
        selector->inspect(item, root == contextNode);
        return item;
    }
    if (children.isEmpty())
        return nullptr;
    return new AddNewTree(root, children, root->displayName());
}

// --------------------------------------------------------------------
// ProjectWizardPage:
// --------------------------------------------------------------------

ProjectWizardPage::ProjectWizardPage(QWidget *parent) : WizardPage(parent),
    m_ui(new Ui::WizardPage)
{
    m_ui->setupUi(this);
    m_ui->vcsManageButton->setText(ICore::msgShowOptionsDialog());
    connect(m_ui->projectComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ProjectWizardPage::projectChanged);
    connect(m_ui->addToVersionControlComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ProjectWizardPage::versionControlChanged);
    connect(m_ui->vcsManageButton, &QAbstractButton::clicked, this, &ProjectWizardPage::manageVcs);
    setProperty(SHORT_TITLE_PROPERTY, tr("Summary"));

    connect(VcsManager::instance(), &VcsManager::configurationChanged,
            this, &ProjectExplorer::Internal::ProjectWizardPage::initializeVersionControls);

    m_ui->projectComboBox->setModel(&m_model);
}

ProjectWizardPage::~ProjectWizardPage()
{
    disconnect(m_ui->projectComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
               this, &ProjectWizardPage::projectChanged);
    delete m_ui;
}

bool ProjectWizardPage::expandTree(const QModelIndex &root)
{
    bool expand = false;
    if (!root.isValid()) // always expand root
        expand = true;

    // Check children
    int count = m_model.rowCount(root);
    for (int i = 0; i < count; ++i) {
        if (expandTree(m_model.index(i, 0, root)))
            expand = true;
    }

    // Apply to self
    if (expand)
        m_ui->projectComboBox->view()->expand(root);
    else
        m_ui->projectComboBox->view()->collapse(root);

    // if we are a high priority node, our *parent* needs to be expanded
    auto tree = static_cast<AddNewTree *>(root.internalPointer());
    if (tree && tree->priority() >= 100)
        expand = true;

    return expand;
}

void ProjectWizardPage::setBestNode(AddNewTree *tree)
{
    QModelIndex index = tree ? m_model.indexForItem(tree) : QModelIndex();
    m_ui->projectComboBox->setCurrentIndex(index);

    while (index.isValid()) {
        m_ui->projectComboBox->view()->expand(index);
        index = index.parent();
    }
}

FolderNode *ProjectWizardPage::currentNode() const
{
    QVariant v = m_ui->projectComboBox->currentData(Qt::UserRole);
    return v.isNull() ? nullptr : static_cast<FolderNode *>(v.value<void *>());
}

void ProjectWizardPage::setAddingSubProject(bool addingSubProject)
{
    m_ui->projectLabel->setText(addingSubProject ?
                                    tr("Add as a subproject to project:")
                                  : tr("Add to &project:"));
}

void ProjectWizardPage::initializeVersionControls()
{
    // Figure out version control situation:
    // 0) Check that any version control is available
    // 1) Directory is managed and VCS supports "Add" -> List it
    // 2) Directory is managed and VCS does not support "Add" -> None available
    // 3) Directory is not managed -> Offer all VCS that support "CreateRepository"

    QList<IVersionControl *> versionControls = VcsManager::versionControls();
    if (versionControls.isEmpty())
        hideVersionControlUiElements();

    IVersionControl *currentSelection = nullptr;
    int currentIdx = versionControlIndex() - 1;
    if (currentIdx >= 0 && currentIdx <= m_activeVersionControls.size() - 1)
        currentSelection = m_activeVersionControls.at(currentIdx);

    m_activeVersionControls.clear();

    QStringList versionControlChoices = QStringList(tr("<None>"));
    if (!m_commonDirectory.isEmpty()) {
        IVersionControl *managingControl = VcsManager::findVersionControlForDirectory(m_commonDirectory);
        if (managingControl) {
            // Under VCS
            if (managingControl->supportsOperation(IVersionControl::AddOperation)) {
                versionControlChoices.append(managingControl->displayName());
                m_activeVersionControls.push_back(managingControl);
                m_repositoryExists = true;
            }
        } else {
            // Create
            foreach (IVersionControl *vc, VcsManager::versionControls()) {
                if (vc->supportsOperation(IVersionControl::CreateRepositoryOperation)) {
                    versionControlChoices.append(vc->displayName());
                    m_activeVersionControls.append(vc);
                }
            }
            m_repositoryExists = false;
        }
    } // has a common root.

    setVersionControls(versionControlChoices);
    // Enable adding to version control by default.
    if (m_repositoryExists && versionControlChoices.size() >= 2)
        setVersionControlIndex(1);
    if (!m_repositoryExists) {
        int newIdx = m_activeVersionControls.indexOf(currentSelection) + 1;
        setVersionControlIndex(newIdx);
    }
}

bool ProjectWizardPage::runVersionControl(const QList<GeneratedFile> &files, QString *errorMessage)
{
    // Add files to  version control (Entry at 0 is 'None').
    const int vcsIndex = versionControlIndex() - 1;
    if (vcsIndex < 0 || vcsIndex >= m_activeVersionControls.size())
        return true;
    QTC_ASSERT(!m_commonDirectory.isEmpty(), return false);

    IVersionControl *versionControl = m_activeVersionControls.at(vcsIndex);
    // Create repository?
    if (!m_repositoryExists) {
        QTC_ASSERT(versionControl->supportsOperation(IVersionControl::CreateRepositoryOperation), return false);
        if (!versionControl->vcsCreateRepository(m_commonDirectory)) {
            *errorMessage = tr("A version control system repository could not be created in \"%1\".").arg(m_commonDirectory);
            return false;
        }
    }
    // Add files if supported.
    if (versionControl->supportsOperation(IVersionControl::AddOperation)) {
        foreach (const GeneratedFile &generatedFile, files) {
            if (!versionControl->vcsAdd(generatedFile.path())) {
                *errorMessage = tr("Failed to add \"%1\" to the version control system.").arg(generatedFile.path());
                return false;
            }
        }
    }
    return true;
}

void ProjectWizardPage::initializeProjectTree(Node *context, const QStringList &paths,
                                              IWizardFactory::WizardKind kind,
                                              ProjectAction action)
{
    BestNodeSelector selector(m_commonDirectory, paths);

    TreeItem *root = m_model.rootItem();
    root->removeChildren();
    for (Project *project : SessionManager::projects()) {
        if (ProjectNode *pn = project->rootProjectNode()) {
            if (kind == IWizardFactory::ProjectWizard) {
                if (AddNewTree *child = buildAddProjectTree(pn, paths.first(), context, &selector))
                    root->appendChild(child);
            } else {
                if (AddNewTree *child = buildAddFilesTree(pn, paths, context, &selector))
                    root->appendChild(child);
            }
        }
    }
    root->prependChild(createNoneNode(&selector));

    setAdditionalInfo(selector.deployingProjects());
    setBestNode(selector.bestChoice());
    setAddingSubProject(action == AddSubProject);

    m_ui->projectComboBox->setEnabled(m_model.rowCount(QModelIndex()) > 1);
}

void ProjectWizardPage::setNoneLabel(const QString &label)
{
    m_ui->projectComboBox->setItemText(0, label);
}

void ProjectWizardPage::setAdditionalInfo(const QString &text)
{
    m_ui->additionalInfo->setText(text);
    m_ui->additionalInfo->setVisible(!text.isEmpty());
}

void ProjectWizardPage::setVersionControls(const QStringList &vcs)
{
    m_ui->addToVersionControlComboBox->clear();
    m_ui->addToVersionControlComboBox->addItems(vcs);
}

int ProjectWizardPage::versionControlIndex() const
{
    return m_ui->addToVersionControlComboBox->currentIndex();
}

void ProjectWizardPage::setVersionControlIndex(int idx)
{
    m_ui->addToVersionControlComboBox->setCurrentIndex(idx);
}

IVersionControl *ProjectWizardPage::currentVersionControl()
{
    int index = m_ui->addToVersionControlComboBox->currentIndex() - 1; // Subtract "<None>"
    if (index < 0 || index > m_activeVersionControls.count())
        return nullptr; // <None>
    return m_activeVersionControls.at(index);
}

void ProjectWizardPage::setFiles(const QStringList &fileNames)
{
    if (fileNames.count() == 1)
        m_commonDirectory = QFileInfo(fileNames.first()).absolutePath();
    else
        m_commonDirectory = Utils::commonPath(fileNames);
    QString fileMessage;
    {
        QTextStream str(&fileMessage);
        str << "<qt>"
            << (m_commonDirectory.isEmpty() ? tr("Files to be added:") : tr("Files to be added in"))
            << "<pre>";

        QStringList formattedFiles;
        if (m_commonDirectory.isEmpty()) {
            formattedFiles = fileNames;
        } else {
            str << QDir::toNativeSeparators(m_commonDirectory) << ":\n\n";
            const int prefixSize = m_commonDirectory.size() + 1;
            formattedFiles = Utils::transform(fileNames, [prefixSize](const QString &f)
                                                         { return f.mid(prefixSize); });
        }
        // Alphabetically, and files in sub-directories first
        Utils::sort(formattedFiles, [](const QString &filePath1, const QString &filePath2) -> bool {
            const bool filePath1HasDir = filePath1.contains(QLatin1Char('/'));
            const bool filePath2HasDir = filePath2.contains(QLatin1Char('/'));

            if (filePath1HasDir == filePath2HasDir)
                return FileName::fromString(filePath1) < FileName::fromString(filePath2);
            return filePath1HasDir;
        }
);

        foreach (const QString &f, formattedFiles)
            str << QDir::toNativeSeparators(f) << '\n';

        str << "</pre>";
    }
    m_ui->filesLabel->setText(fileMessage);
}

void ProjectWizardPage::setProjectToolTip(const QString &tt)
{
    m_ui->projectComboBox->setToolTip(tt);
    m_ui->projectLabel->setToolTip(tt);
}

void ProjectWizardPage::projectChanged(int index)
{
    setProjectToolTip(index >= 0 && index < m_projectToolTips.size() ?
                      m_projectToolTips.at(index) : QString());
    emit projectNodeChanged();
}

void ProjectWizardPage::manageVcs()
{
    ICore::showOptionsDialog(VcsBase::Constants::VCS_COMMON_SETTINGS_ID, this);
}

void ProjectWizardPage::hideVersionControlUiElements()
{
    m_ui->addToVersionControlLabel->hide();
    m_ui->vcsManageButton->hide();
    m_ui->addToVersionControlComboBox->hide();
}

void ProjectWizardPage::setProjectUiVisible(bool visible)
{
    m_ui->projectLabel->setVisible(visible);
    m_ui->projectComboBox->setVisible(visible);
}

} // namespace Internal
} // namespace ProjectExplorer

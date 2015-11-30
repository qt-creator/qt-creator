/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "projectwizardpage.h"
#include "ui_projectwizardpage.h"

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
    FolderNode *m_node;
    bool m_canAdd;
    int m_priority;
};

AddNewTree::AddNewTree(const QString &displayName)
    : m_displayName(displayName),
      m_node(0),
      m_canAdd(true),
      m_priority(-1)
{
}

// FIXME: potentially merge the following two functions.
// Note the different handling of 'node' and m_canAdd.
AddNewTree::AddNewTree(FolderNode *node, QList<AddNewTree *> children, const QString &displayName)
    : m_displayName(displayName),
      m_node(0),
      m_canAdd(false),
      m_priority(-1)
{
    if (node)
        m_toolTip = ProjectExplorerPlugin::directoryFor(node);
    foreach (AddNewTree *child, children)
        appendChild(child);
}

AddNewTree::AddNewTree(FolderNode *node, QList<AddNewTree *> children, const FolderNode::AddNewInformation &info)
    : m_displayName(info.displayName),
      m_node(node),
      m_canAdd(true),
      m_priority(info.priority)
{
    if (node)
        m_toolTip = ProjectExplorerPlugin::directoryFor(node);
    foreach (AddNewTree *child, children)
        appendChild(child);
}


QVariant AddNewTree::data(int, int role) const
{
    if (role == Qt::DisplayRole)
        return m_displayName;
    if (role == Qt::ToolTipRole)
        return m_toolTip;
    return QVariant();
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
    bool m_deploys;
    QString m_deployText;
    AddNewTree *m_bestChoice;
    int m_bestMatchLength;
    int m_bestMatchPriority;
};

BestNodeSelector::BestNodeSelector(const QString &commonDirectory, const QStringList &files)
    : m_commonDirectory(commonDirectory),
      m_files(files),
      m_deploys(false),
      m_deployText(QCoreApplication::translate("ProjectWizard", "The files are implicitly added to the projects:") + QLatin1Char('\n')),
      m_bestChoice(0),
      m_bestMatchLength(-1),
      m_bestMatchPriority(-1)
{ }

// Find the project the new files should be added
// If any node deploys the files, then we don't want to add the files.
// Otherwise consider their common path. Either a direct match on the directory
// or the directory with the longest matching path (list containing"/project/subproject1"
// matching common path "/project/subproject1/newuserpath").
void BestNodeSelector::inspect(AddNewTree *tree, bool isContextNode)
{
    FolderNode *node = tree->node();
    if (node->nodeType() == ProjectNodeType) {
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
    foreach (ProjectNode *pn, root->subProjectNodes()) {
        AddNewTree *child = buildAddProjectTree(pn, projectPath, contextNode, selector);
        if (child)
            children.append(child);
    }

    const QList<ProjectAction> &list = root->supportedActions(root);
    if (list.contains(AddSubProject) && !list.contains(InheritedFromParent)) {
        if (projectPath.isEmpty() || root->canAddSubProject(projectPath)) {
            FolderNode::AddNewInformation info = root->addNewInformation(QStringList() << projectPath, contextNode);
            AddNewTree *item = new AddNewTree(root, children, info);
            selector->inspect(item, root == contextNode);
            return item;
        }
    }

    if (children.isEmpty())
        return 0;
    return new AddNewTree(root, children, root->displayName());
}

static inline AddNewTree *buildAddProjectTree(SessionNode *root, const QString &projectPath, Node *contextNode, BestNodeSelector *selector)
{
    QList<AddNewTree *> children;
    foreach (ProjectNode *pn, root->projectNodes()) {
        AddNewTree *child = buildAddProjectTree(pn, projectPath, contextNode, selector);
        if (child)
            children.append(child);
    }
    children.prepend(createNoneNode(selector));
    return new AddNewTree(root, children, root->displayName());
}

static inline AddNewTree *buildAddFilesTree(FolderNode *root, const QStringList &files,
                                            Node *contextNode, BestNodeSelector *selector)
{
    QList<AddNewTree *> children;
    foreach (FolderNode *fn, root->subFolderNodes()) {
        AddNewTree *child = buildAddFilesTree(fn, files, contextNode, selector);
        if (child)
            children.append(child);
    }

    const QList<ProjectAction> &list = root->supportedActions(root);
    if (list.contains(AddNewFile) && !list.contains(InheritedFromParent)) {
        FolderNode::AddNewInformation info = root->addNewInformation(files, contextNode);
        AddNewTree *item = new AddNewTree(root, children, info);
        selector->inspect(item, root == contextNode);
        return item;
    }
    if (children.isEmpty())
        return 0;
    return new AddNewTree(root, children, root->displayName());
}

static inline AddNewTree *buildAddFilesTree(SessionNode *root, const QStringList &files,
                                            Node *contextNode, BestNodeSelector *selector)
{
    QList<AddNewTree *> children;
    foreach (ProjectNode *pn, root->projectNodes()) {
        AddNewTree *child = buildAddFilesTree(pn, files, contextNode, selector);
        if (child)
            children.append(child);
    }
    children.prepend(createNoneNode(selector));
    return new AddNewTree(root, children, root->displayName());
}

static inline AddNewTree *getChoices(const QStringList &generatedFiles,
                                     IWizardFactory::WizardKind wizardKind,
                                     Node *contextNode,
                                     BestNodeSelector *selector)
{
    if (wizardKind == IWizardFactory::ProjectWizard)
        return buildAddProjectTree(SessionManager::sessionNode(), generatedFiles.first(), contextNode, selector);
    else
        return buildAddFilesTree(SessionManager::sessionNode(), generatedFiles, contextNode, selector);
}

// --------------------------------------------------------------------
// ProjectWizardPage:
// --------------------------------------------------------------------

ProjectWizardPage::ProjectWizardPage(QWidget *parent) :
    WizardPage(parent),
    m_ui(new Ui::WizardPage),
    m_model(0),
    m_repositoryExists(false)
{
    m_ui->setupUi(this);
    m_ui->vcsManageButton->setText(ICore::msgShowOptionsDialog());
    connect(m_ui->projectComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(projectChanged(int)));
    connect(m_ui->addToVersionControlComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ProjectWizardPage::versionControlChanged);
    connect(m_ui->vcsManageButton, &QAbstractButton::clicked, this, &ProjectWizardPage::manageVcs);
    setProperty(SHORT_TITLE_PROPERTY, tr("Summary"));

    connect(VcsManager::instance(), SIGNAL(configurationChanged(const IVersionControl*)),
            this, SLOT(initializeVersionControls()));
}

ProjectWizardPage::~ProjectWizardPage()
{
    disconnect(m_ui->projectComboBox, SIGNAL(currentIndexChanged(int)),
               this, SLOT(projectChanged(int)));
    delete m_model;
    delete m_ui;
}

void ProjectWizardPage::setModel(TreeModel *model)
{
    delete m_model;
    m_model = model;

    // TODO see OverViewCombo and OverView for click event filter
    m_ui->projectComboBox->setModel(model);
    bool enabled = m_model->rowCount(QModelIndex()) > 1;
    m_ui->projectComboBox->setEnabled(enabled);

    expandTree(QModelIndex());
}

bool ProjectWizardPage::expandTree(const QModelIndex &root)
{
    bool expand = false;
    if (!root.isValid()) // always expand root
        expand = true;

    // Check children
    int count = m_model->rowCount(root);
    for (int i = 0; i < count; ++i) {
        if (expandTree(m_model->index(i, 0, root)))
            expand = true;
    }

    // Apply to self
    if (expand)
        m_ui->projectComboBox->view()->expand(root);
    else
        m_ui->projectComboBox->view()->collapse(root);

    // if we are a high priority node, our *parent* needs to be expanded
    AddNewTree *tree = static_cast<AddNewTree *>(root.internalPointer());
    if (tree && tree->priority() >= 100)
        expand = true;

    return expand;
}

void ProjectWizardPage::setBestNode(AddNewTree *tree)
{
    QModelIndex index = tree ? m_model->indexForItem(tree) : QModelIndex();
    m_ui->projectComboBox->setCurrentIndex(index);

    while (index.isValid()) {
        m_ui->projectComboBox->view()->expand(index);
        index = index.parent();
    }
}

FolderNode *ProjectWizardPage::currentNode() const
{
    QModelIndex index = m_ui->projectComboBox->view()->currentIndex();
    TreeItem *item = m_model->itemForIndex(index);
    return item ? static_cast<AddNewTree *>(item)->node() : 0;
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

    IVersionControl *currentSelection = 0;
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
    AddNewTree *tree = getChoices(paths, kind, context, &selector);

    setAdditionalInfo(selector.deployingProjects());

    TreeModel *model = new TreeModel(tree);
    setModel(model);
    setBestNode(selector.bestChoice());
    setAddingSubProject(action == AddSubProject);
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
        return 0; // <None>
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

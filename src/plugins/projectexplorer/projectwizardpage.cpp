// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectwizardpage.h"

#include "project.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "projectmodels.h"

#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/iwizardfactory.h>
#include <coreplugin/vcsmanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/treemodel.h>
#include <utils/treeviewcombobox.h>
#include <utils/wizard.h>

#include <vcsbase/vcsbaseconstants.h>

#include <QComboBox>
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTextStream>

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

    QVariant data(int column, int role) const override;
    Qt::ItemFlags flags(int column) const override;

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
        m_toolTip = node->directory().toString();
    for (AddNewTree *child : std::as_const(children))
        appendChild(child);
}

AddNewTree::AddNewTree(FolderNode *node, QList<AddNewTree *> children,
                       const FolderNode::AddNewInformation &info) :
    m_displayName(info.displayName),
    m_node(node),
    m_priority(info.priority)
{
    if (node)
        m_toolTip = node->directory().toString();
    for (AddNewTree *child : std::as_const(children))
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
        return {};
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
    BestNodeSelector(const FilePath &commonDirectory, const FilePaths &files);
    void inspect(AddNewTree *tree, bool isContextNode);
    AddNewTree *bestChoice() const;
    bool deploys();
    QString deployingProjects() const;

private:
    FilePath m_commonDirectory;
    FilePaths m_files;
    bool m_deploys = false;
    QString m_deployText;
    AddNewTree *m_bestChoice = nullptr;
    int m_bestMatchLength = -1;
    int m_bestMatchPriority = -1;
};

BestNodeSelector::BestNodeSelector(const FilePath &commonDirectory, const FilePaths &files) :
    m_commonDirectory(commonDirectory),
    m_files(files),
    m_deployText(Tr::tr("The files are implicitly added to the projects:") + QLatin1Char('\n'))
{ }

// Find the project the new files should be added
// If any node deploys the files, then we don't want to add the files.
// Otherwise consider their common path. Either a direct match on the directory
// or the directory with the longest matching path (list containing"/project/subproject1"
// matching common path "/project/subproject1/newuserpath").
void BestNodeSelector::inspect(AddNewTree *tree, bool isContextNode)
{
    FolderNode *node = tree->node();
    if (node->isProjectNodeType()) {
        if (static_cast<ProjectNode *>(node)->deploysFolder(m_commonDirectory.toString())) {
            m_deploys = true;
            m_deployText += tree->displayName() + QLatin1Char('\n');
        }
    }
    if (m_deploys)
        return;

    const FilePath projectDirectory = node->directory();
    const int projectDirectorySize = projectDirectory.toString().size();
    if (m_commonDirectory != projectDirectory
            && !m_commonDirectory.toString().startsWith(
                projectDirectory.toString() + QLatin1Char('/')) // TODO: still required?
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
        return nullptr;
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
    return {};
}

// --------------------------------------------------------------------
// Helper:
// --------------------------------------------------------------------

static inline AddNewTree *createNoneNode(BestNodeSelector *selector)
{
    QString displayName = Tr::tr("<None>");
    if (selector->deploys())
        displayName = Tr::tr("<Implicitly Add>");
    return new AddNewTree(displayName);
}

static inline AddNewTree *buildAddProjectTree(ProjectNode *root, const FilePath &projectPath,
                                              Node *contextNode, BestNodeSelector *selector)
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
            FolderNode::AddNewInformation info = root->addNewInformation({projectPath}, contextNode);
            auto item = new AddNewTree(root, children, info);
            selector->inspect(item, root == contextNode);
            return item;
        }
    }

    if (children.isEmpty())
        return nullptr;
    return new AddNewTree(root, children, root->displayName());
}

static AddNewTree *buildAddFilesTree(FolderNode *root, const FilePaths &files,
                                     Node *contextNode, BestNodeSelector *selector)
{
    QList<AddNewTree *> children;
    root->forEachFolderNode([&](FolderNode *fn) {
        if (AddNewTree *child = buildAddFilesTree(fn, files, contextNode, selector))
            children.append(child);
    });

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

ProjectWizardPage::ProjectWizardPage(QWidget *parent)
    : WizardPage(parent)
{
    m_projectLabel = new QLabel;
    m_projectLabel->setObjectName("projectLabel");
    m_projectComboBox = new Utils::TreeViewComboBox;
    m_projectComboBox->setObjectName("projectComboBox");
    m_additionalInfo = new QLabel;
    m_addToVersionControlLabel = new QLabel(Tr::tr("Add to &version control:"));
    m_addToVersionControlComboBox = new QComboBox;
    m_addToVersionControlComboBox->setObjectName("addToVersionControlComboBox");
    m_vcsManageButton = new QPushButton(ICore::msgShowOptionsDialog());
    m_vcsManageButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    m_filesLabel = new QLabel;
    m_filesLabel->setObjectName("filesLabel");
    m_filesLabel->setAlignment(Qt::AlignBottom);
    m_filesLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    auto scrollArea = new QScrollArea;
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(m_filesLabel);

    using namespace Layouting;
    Column {
        Form {
            m_projectLabel, m_projectComboBox, br,
            empty, m_additionalInfo, br,
            m_addToVersionControlLabel, m_addToVersionControlComboBox, m_vcsManageButton, br,
        },
        scrollArea,
    }.attachTo(this);

    connect(m_vcsManageButton, &QAbstractButton::clicked, this, &ProjectWizardPage::manageVcs);
    setProperty(SHORT_TITLE_PROPERTY, Tr::tr("Summary"));

    connect(VcsManager::instance(), &VcsManager::configurationChanged,
            this, &ProjectExplorer::Internal::ProjectWizardPage::initializeVersionControls);

    m_projectComboBox->setModel(&m_model);
}

ProjectWizardPage::~ProjectWizardPage()
{
    disconnect(m_projectComboBox, &QComboBox::currentIndexChanged,
               this, &ProjectWizardPage::projectChanged);
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
        m_projectComboBox->view()->expand(root);
    else
        m_projectComboBox->view()->collapse(root);

    // if we are a high priority node, our *parent* needs to be expanded
    auto tree = static_cast<AddNewTree *>(root.internalPointer());
    if (tree && tree->priority() >= 100)
        expand = true;

    return expand;
}

void ProjectWizardPage::setBestNode(AddNewTree *tree)
{
    QModelIndex index = tree ? m_model.indexForItem(tree) : QModelIndex();
    m_projectComboBox->setCurrentIndex(index);

    while (index.isValid()) {
        m_projectComboBox->view()->expand(index);
        index = index.parent();
    }
}

FolderNode *ProjectWizardPage::currentNode() const
{
    QVariant v = m_projectComboBox->currentData(Qt::UserRole);
    return v.isNull() ? nullptr : static_cast<FolderNode *>(v.value<void *>());
}

void ProjectWizardPage::setAddingSubProject(bool addingSubProject)
{
    m_projectLabel->setText(addingSubProject ?
                                Tr::tr("Add as a subproject to project:")
                              : Tr::tr("Add to &project:"));
}

void ProjectWizardPage::initializeVersionControls()
{
    // Figure out version control situation:
    // 0) Check that any version control is available
    // 1) Directory is managed and VCS supports "Add" -> List it
    // 2) Directory is managed and VCS does not support "Add" -> None available
    // 3) Directory is not managed -> Offer all VCS that support "CreateRepository"

    m_addToVersionControlComboBox->disconnect();
    QList<IVersionControl *> versionControls = VcsManager::versionControls();
    if (versionControls.isEmpty())
        hideVersionControlUiElements();

    IVersionControl *currentSelection = nullptr;
    int currentIdx = versionControlIndex() - 1;
    if (currentIdx >= 0 && currentIdx <= m_activeVersionControls.size() - 1)
        currentSelection = m_activeVersionControls.at(currentIdx);

    m_activeVersionControls.clear();

    QStringList versionControlChoices = QStringList(Tr::tr("<None>"));
    if (!m_commonDirectory.isEmpty()) {
        IVersionControl *managingControl =
                VcsManager::findVersionControlForDirectory(m_commonDirectory);
        if (managingControl) {
            // Under VCS
            if (managingControl->supportsOperation(IVersionControl::AddOperation)) {
                versionControlChoices.append(managingControl->displayName());
                m_activeVersionControls.push_back(managingControl);
                m_repositoryExists = true;
            }
        } else {
            // Create
            const QList<IVersionControl *> versionControls = VcsManager::versionControls();
            for (IVersionControl *vc : versionControls) {
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

    connect(m_addToVersionControlComboBox, &QComboBox::currentIndexChanged,
            this, &ProjectWizardPage::versionControlChanged);
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
            *errorMessage =
                    Tr::tr("A version control system repository could not be created in \"%1\".").
                    arg(m_commonDirectory.toUserOutput());
            return false;
        }
    }
    // Add files if supported.
    if (versionControl->supportsOperation(IVersionControl::AddOperation)) {
        for (const GeneratedFile &generatedFile : files) {
            if (!versionControl->vcsAdd(generatedFile.filePath())) {
                *errorMessage = Tr::tr("Failed to add \"%1\" to the version control system.").
                        arg(generatedFile.filePath().toUserOutput());
                return false;
            }
        }
    }
    return true;
}

void ProjectWizardPage::initializeProjectTree(Node *context, const FilePaths &paths,
                                              IWizardFactory::WizardKind kind,
                                              ProjectAction action)
{
    m_projectComboBox->disconnect();
    BestNodeSelector selector(m_commonDirectory, paths);

    TreeItem *root = m_model.rootItem();
    root->removeChildren();
    for (Project *project : ProjectManager::projects()) {
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
    root->sortChildren([](const TreeItem *ti1, const TreeItem *ti2) {
        return compareNodes(static_cast<const AddNewTree *>(ti1)->node(),
                            static_cast<const AddNewTree *>(ti2)->node());
    });
    root->prependChild(createNoneNode(&selector));

    // Set combobox to context node if that appears in the tree:
    auto predicate = [context](TreeItem *ti) { return static_cast<AddNewTree*>(ti)->node() == context; };
    TreeItem *contextItem = root->findAnyChild(predicate);
    if (contextItem)
        m_projectComboBox->setCurrentIndex(m_model.indexForItem(contextItem));

    setAdditionalInfo(selector.deployingProjects());
    setBestNode(selector.bestChoice());
    setAddingSubProject(action == AddSubProject);

    m_projectComboBox->setEnabled(m_model.rowCount(QModelIndex()) > 1);
    connect(m_projectComboBox, &QComboBox::currentIndexChanged,
            this, &ProjectWizardPage::projectChanged);
}

void ProjectWizardPage::setNoneLabel(const QString &label)
{
    m_projectComboBox->setItemText(0, label);
}

void ProjectWizardPage::setAdditionalInfo(const QString &text)
{
    m_additionalInfo->setText(text);
    m_additionalInfo->setVisible(!text.isEmpty());
}

void ProjectWizardPage::setVersionControls(const QStringList &vcs)
{
    m_addToVersionControlComboBox->clear();
    m_addToVersionControlComboBox->addItems(vcs);
}

int ProjectWizardPage::versionControlIndex() const
{
    return m_addToVersionControlComboBox->currentIndex();
}

void ProjectWizardPage::setVersionControlIndex(int idx)
{
    m_addToVersionControlComboBox->setCurrentIndex(idx);
}

IVersionControl *ProjectWizardPage::currentVersionControl()
{
    int index = m_addToVersionControlComboBox->currentIndex() - 1; // Subtract "<None>"
    if (index < 0 || index > m_activeVersionControls.count())
        return nullptr; // <None>
    return m_activeVersionControls.at(index);
}

void ProjectWizardPage::setFiles(const FilePaths &files)
{
    m_commonDirectory = FileUtils::commonPath(files);
    const bool hasNoCommonDirectory = m_commonDirectory.isEmpty() || files.size() < 2;

    QString fileMessage;
    {
        QTextStream str(&fileMessage);
        str << "<qt>"
            << (hasNoCommonDirectory ? Tr::tr("Files to be added:") : Tr::tr("Files to be added in"))
            << "<pre>";

        QStringList formattedFiles;
        if (hasNoCommonDirectory) {
            formattedFiles = Utils::transform(files, &FilePath::toString);
        } else {
            str << m_commonDirectory.toUserOutput() << ":\n\n";
            int prefixSize = m_commonDirectory.toUserOutput().size();
            formattedFiles = Utils::transform(files, [prefixSize] (const FilePath &f) {
                return f.toUserOutput().mid(prefixSize + 1); // +1 skips the initial dir separator
            });
        }
        // Alphabetically, and files in sub-directories first
        Utils::sort(formattedFiles, [](const QString &filePath1, const QString &filePath2) -> bool {
            const bool filePath1HasDir = filePath1.contains(QLatin1Char('/'));
            const bool filePath2HasDir = filePath2.contains(QLatin1Char('/'));

            if (filePath1HasDir == filePath2HasDir)
                return FilePath::fromString(filePath1) < FilePath::fromString(filePath2);
            return filePath1HasDir;
        });

        for (const QString &f : std::as_const(formattedFiles))
            str << QDir::toNativeSeparators(f) << '\n';

        str << "</pre>";
    }
    m_filesLabel->setText(fileMessage);
}

void ProjectWizardPage::setProjectToolTip(const QString &tt)
{
    m_projectComboBox->setToolTip(tt);
    m_projectLabel->setToolTip(tt);
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
    m_addToVersionControlLabel->hide();
    m_vcsManageButton->hide();
    m_addToVersionControlComboBox->hide();
}

void ProjectWizardPage::setProjectUiVisible(bool visible)
{
    m_projectLabel->setVisible(visible);
    m_projectComboBox->setVisible(visible);
}

} // namespace Internal
} // namespace ProjectExplorer

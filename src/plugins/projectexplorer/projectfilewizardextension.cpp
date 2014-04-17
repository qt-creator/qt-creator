/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "projectfilewizardextension.h"
#include "projectexplorer.h"
#include "session.h"
#include "projectnodes.h"
#include "nodesvisitor.h"
#include "projectwizardpage.h"

#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <coreplugin/basefilewizard.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/mimedatabase.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/normalindenter.h>
#include <texteditor/tabsettings.h>
#include <texteditor/storagesettings.h>
#include <projectexplorer/project.h>
#include <projectexplorer/editorconfiguration.h>

#include <QtAlgorithms>
#include <QPointer>
#include <QDebug>
#include <QFileInfo>
#include <QMultiMap>
#include <QDir>
#include <QTextDocument>
#include <QTextCursor>
#include <QMessageBox>

using namespace TextEditor;
using namespace Core;

/*!
    \class ProjectExplorer::Internal::ProjectFileWizardExtension

    \brief The ProjectFileWizardExtension class implements the post-file
    generating steps of a project wizard.

    This class provides the following functions:
    \list
    \li Add to a project file (*.pri/ *.pro)
    \li Initialize a version control system repository (unless the path is already
        managed) and do 'add' if the VCS supports it.
    \endlist

    \sa ProjectExplorer::Internal::ProjectWizardPage
*/

enum { debugExtension = 0 };

namespace ProjectExplorer {

typedef QList<FolderNode *> FolderNodeList;
typedef QList<ProjectNode *> ProjectNodeList;

namespace Internal {

// AddNewFileNodesVisitor: Retrieve all folders which support AddNew
class AddNewFileNodesVisitor : public NodesVisitor
{
public:
    AddNewFileNodesVisitor();

    static FolderNodeList allFolders();

    virtual void visitProjectNode(ProjectNode *node);
    virtual void visitFolderNode(FolderNode *node);

private:
    FolderNodeList m_folderNodes;
};

AddNewFileNodesVisitor::AddNewFileNodesVisitor()
{}

FolderNodeList AddNewFileNodesVisitor::allFolders()
{
    AddNewFileNodesVisitor visitor;
    SessionManager::sessionNode()->accept(&visitor);
    return visitor.m_folderNodes;
}

void AddNewFileNodesVisitor::visitProjectNode(ProjectNode *node)
{
    visitFolderNode(node);
}

void AddNewFileNodesVisitor::visitFolderNode(FolderNode *node)
{
    const QList<ProjectExplorer::ProjectAction> &list = node->supportedActions(node);
    if (list.contains(ProjectExplorer::AddNewFile) && !list.contains(ProjectExplorer::InheritedFromParent))
        m_folderNodes.push_back(node);
}

// AddNewProjectNodesVisitor: Retrieve all folders which support AddNewProject
// also checks the path of the added subproject
class AddNewProjectNodesVisitor : public NodesVisitor
{
public:
    AddNewProjectNodesVisitor(const QString &subProjectPath);

    static ProjectNodeList projectNodes(const QString &subProjectPath);

    virtual void visitProjectNode(ProjectNode *node);

private:
    ProjectNodeList m_projectNodes;
    QString m_subProjectPath;
};

AddNewProjectNodesVisitor::AddNewProjectNodesVisitor(const QString &subProjectPath)
    : m_subProjectPath(subProjectPath)
{}

ProjectNodeList AddNewProjectNodesVisitor::projectNodes(const QString &subProjectPath)
{
    AddNewProjectNodesVisitor visitor(subProjectPath);
    SessionManager::sessionNode()->accept(&visitor);
    return visitor.m_projectNodes;
}

void AddNewProjectNodesVisitor::visitProjectNode(ProjectNode *node)
{
    const QList<ProjectExplorer::ProjectAction> &list = node->supportedActions(node);
    if (list.contains(ProjectExplorer::AddSubProject) && !list.contains(ProjectExplorer::InheritedFromParent))
        if (m_subProjectPath.isEmpty() || node->canAddSubProject(m_subProjectPath))
            m_projectNodes.push_back(node);
}

// ProjectEntry: Context entry for a *.pri/*.pro file. Stores name and path
// for quick sort and path search, provides operator<() for maps.
struct FolderEntry {
    FolderEntry() : node(0), priority(-1) {}
    explicit FolderEntry(FolderNode *node, const QStringList &generatedFiles, Node *contextNode);

    int compare(const FolderEntry &rhs) const;

    FolderNode *node;
    QString directory; // For matching against wizards' files, which are native.
    QString displayName;
    QString baseName;
    int priority;
};

FolderEntry::FolderEntry(FolderNode *n, const QStringList &generatedFiles, Node *contextNode) :
    node(n)
{
    FolderNode::AddNewInformation info = node->addNewInformation(generatedFiles, contextNode);
    displayName = info.displayName;
    const QFileInfo fi(ProjectExplorerPlugin::pathFor(node));
    baseName = fi.baseName();
    priority = info.priority;
    directory = ProjectExplorerPlugin::directoryFor(node);
}

// Sort helper that sorts by base name and puts '*.pro' before '*.pri'
int FolderEntry::compare(const FolderEntry &rhs) const
{
    if (const int drc = displayName.compare(rhs.displayName))
        return drc;
    if (const int drc = directory.compare(rhs.directory))
        return drc;
    if (const int brc = baseName.compare(rhs.baseName))
        return brc;
    if (priority > rhs.priority)
        return -1;
    if (priority < rhs.priority)
        return 1;
    return 0;
}

inline bool operator<(const FolderEntry &pe1, const FolderEntry &pe2)
{
    return pe1.compare(pe2) < 0;
}

QDebug operator<<(QDebug d, const FolderEntry &e)
{
    d.nospace() << e.directory << ' ' << e.displayName << ' ' << e.priority;
    return d;
}

// --------- ProjectWizardContext
struct ProjectWizardContext
{
    ProjectWizardContext();
    void clear();

    QList<IVersionControl*> versionControls;
    QList<IVersionControl*> activeVersionControls;
    QList<FolderEntry> projects;
    QPointer<ProjectWizardPage> page; // this is managed by the wizard!
    bool repositoryExists; // Is VCS 'add' sufficient, or should a repository be created?
    QString commonDirectory;
    const IWizard *wizard;
};

ProjectWizardContext::ProjectWizardContext() :
    page(0),
    repositoryExists(false),
    wizard(0)
{
}

void ProjectWizardContext::clear()
{
    activeVersionControls.clear();
    projects.clear();
    commonDirectory.clear();
    page = 0;
    repositoryExists = false;
    wizard = 0;
}

// ---- ProjectFileWizardExtension
ProjectFileWizardExtension::ProjectFileWizardExtension()
  : m_context(0)
{
}

ProjectFileWizardExtension::~ProjectFileWizardExtension()
{
    delete m_context;
}

static QList<FolderEntry> findDeployProject(const QList<FolderEntry> &folders,
                                 QString &commonPath)
{
    QList<FolderEntry> filtered;
    foreach (const FolderEntry &folder, folders)
        if (folder.node->nodeType() == ProjectNodeType)
            if (static_cast<ProjectNode *>(folder.node)->deploysFolder(commonPath))
                filtered << folder;
    return filtered;
}

// Find the project the new files should be added to given their common
// path. Either a direct match on the directory or the directory with
// the longest matching path (list containing"/project/subproject1" matching
// common path "/project/subproject1/newuserpath").
static int findMatchingProject(const QList<FolderEntry> &projects,
                               const QString &commonPath)
{
    if (projects.isEmpty() || commonPath.isEmpty())
        return -1;

    const int count = projects.size();
    int bestMatch = -1;
    int bestMatchLength = 0;
    int bestMatchPriority = -1;
    for (int p = 0; p < count; p++) {
        // Direct match or better match? (note that the wizards' files are native).
        const FolderEntry &entry = projects.at(p);
        const QString &projectDirectory = entry.directory;
        const int projectDirectorySize = projectDirectory.size();
        if (entry.priority > bestMatchPriority) {
            if (commonPath.startsWith(projectDirectory)) {
                bestMatchPriority = entry.priority;
                bestMatchLength = projectDirectory.size();
                bestMatch = p;
            }
        } else if (entry.priority == bestMatchPriority) {
            if (projectDirectorySize > bestMatchLength
                    && commonPath.startsWith(projectDirectory)) {
                bestMatchPriority = entry.priority;
                bestMatchLength = projectDirectory.size();
                bestMatch = p;
            }
        }
    }
    return bestMatch;
}

static QString generatedProjectFilePath(const QList<GeneratedFile> &files)
{
    foreach (const GeneratedFile &file, files)
        if (file.attributes() & GeneratedFile::OpenProjectAttribute)
            return file.path();
    return QString();
}

void ProjectFileWizardExtension::firstExtensionPageShown(
        const QList<GeneratedFile> &files,
        const QVariantMap &extraValues)
{
    initProjectChoices(files, extraValues);

    if (debugExtension)
        qDebug() << Q_FUNC_INFO << files.size();

    // Parametrize wizard page: find best project to add to, set up files display and
    // version control depending on path
    QStringList fileNames;
    foreach (const GeneratedFile &f, files)
        fileNames.push_back(f.path());
    m_context->commonDirectory = Utils::commonPath(fileNames);
    m_context->page->setFilesDisplay(m_context->commonDirectory, fileNames);
    // Find best project (Entry at 0 is 'None').

    int bestProjectIndex = -1;

    QList<FolderEntry> deployingProjects = findDeployProject(m_context->projects, m_context->commonDirectory);
    if (!deployingProjects.isEmpty()) {
        // Oh we do have someone that deploys it
        // then the best match is NONE
        // We display a label explaining that and rename <None> to
        // <Implicitly Add>
        m_context->page->setNoneLabel(tr("<Implicitly Add>"));

        QString text = tr("The files are implicitly added to the projects:");
        text += QLatin1Char('\n');
        foreach (const FolderEntry &project, deployingProjects) {
            text += project.displayName;
            text += QLatin1Char('\n');
        }

        m_context->page->setAdditionalInfo(text);
        bestProjectIndex = -1;
    } else {
        bestProjectIndex = findMatchingProject(m_context->projects, m_context->commonDirectory);
        m_context->page->setNoneLabel(tr("<None>"));
    }

    if (bestProjectIndex == -1)
        m_context->page->setCurrentProjectIndex(0);
    else
        m_context->page->setCurrentProjectIndex(bestProjectIndex + 1);

    // Store all version controls for later use:
    if (m_context->versionControls.isEmpty()) {
        foreach (IVersionControl *vc, ExtensionSystem::PluginManager::getObjects<IVersionControl>()) {
            m_context->versionControls.append(vc);
            connect(vc, SIGNAL(configurationChanged()), this, SLOT(initializeVersionControlChoices()));
        }
    }

    initializeVersionControlChoices();
}

void ProjectFileWizardExtension::initializeVersionControlChoices()
{
    if (m_context->page.isNull())
        return;

    // Figure out version control situation:
    // 1) Directory is managed and VCS supports "Add" -> List it
    // 2) Directory is managed and VCS does not support "Add" -> None available
    // 3) Directory is not managed -> Offer all VCS that support "CreateRepository"

    IVersionControl *currentSelection = 0;
    int currentIdx = m_context->page->versionControlIndex() - 1;
    if (currentIdx >= 0 && currentIdx <= m_context->activeVersionControls.size() - 1)
        currentSelection = m_context->activeVersionControls.at(currentIdx);

    m_context->activeVersionControls.clear();

    QStringList versionControlChoices = QStringList(tr("<None>"));
    if (!m_context->commonDirectory.isEmpty()) {
        IVersionControl *managingControl = VcsManager::findVersionControlForDirectory(m_context->commonDirectory);
        if (managingControl) {
            // Under VCS
            if (managingControl->supportsOperation(IVersionControl::AddOperation)) {
                versionControlChoices.append(managingControl->displayName());
                m_context->activeVersionControls.push_back(managingControl);
                m_context->repositoryExists = true;
            }
        } else {
            // Create
            foreach (IVersionControl *vc, m_context->versionControls)
                if (vc->supportsOperation(IVersionControl::CreateRepositoryOperation)) {
                    versionControlChoices.append(vc->displayName());
                    m_context->activeVersionControls.append(vc);
                }
            m_context->repositoryExists = false;
        }
    } // has a common root.

    m_context->page->setVersionControls(versionControlChoices);
    // Enable adding to version control by default.
    if (m_context->repositoryExists && versionControlChoices.size() >= 2)
        m_context->page->setVersionControlIndex(1);
    if (!m_context->repositoryExists) {
        int newIdx = m_context->activeVersionControls.indexOf(currentSelection) + 1;
        m_context->page->setVersionControlIndex(newIdx);
    }
}

QList<QWizardPage *> ProjectFileWizardExtension::extensionPages(const IWizard *wizard)
{
    if (!m_context)
        m_context = new ProjectWizardContext;
    else
        m_context->clear();
    // Init context with page and projects
    m_context->page = new ProjectWizardPage;
    m_context->wizard = wizard;
    return QList<QWizardPage *>() << m_context->page;
}


static inline void getProjectChoicesAndToolTips(QStringList *projectChoicesParam,
                                                QStringList *projectToolTipsParam,
                                                ProjectExplorer::ProjectAction *projectActionParam,
                                                const QList<GeneratedFile> &generatedFiles,
                                                ProjectWizardContext *context,
                                                Node *contextNode)
{
    // Set up project list which remains the same over duration of wizard execution
    // As tooltip, set the directory for disambiguation (should someone have
    // duplicate base names in differing directories).
    //: No project selected
    QStringList projectChoices(ProjectFileWizardExtension::tr("<None>"));
    QStringList projectToolTips((QString()));

    typedef QMap<FolderEntry, bool> FolderEntryMap;
    // Sort by base name and purge duplicated entries (resulting from dependencies)
    // via Map.
    FolderEntryMap entryMap;
    ProjectExplorer::ProjectAction projectAction;
    if (context->wizard->kind()== IWizard::ProjectWizard) {
        const QString projectFilePath = generatedProjectFilePath(generatedFiles);
        projectAction = ProjectExplorer::AddSubProject;
        foreach (ProjectNode *pn, AddNewProjectNodesVisitor::projectNodes(projectFilePath))
            entryMap.insert(FolderEntry(pn, QStringList() << projectFilePath, contextNode), true);

    } else {
        QStringList filePaths;
        foreach (const GeneratedFile &gf, generatedFiles)
            filePaths << gf.path();

        projectAction = ProjectExplorer::AddNewFile;
        foreach (FolderNode *fn, AddNewFileNodesVisitor::allFolders())
            entryMap.insert(FolderEntry(fn, filePaths, contextNode), true);
    }

    context->projects.clear();

    // Collect names
    const FolderEntryMap::const_iterator cend = entryMap.constEnd();
    for (FolderEntryMap::const_iterator it = entryMap.constBegin(); it != cend; ++it) {
        context->projects.push_back(it.key());
        projectChoices.push_back(it.key().displayName);
        projectToolTips.push_back(QDir::toNativeSeparators(it.key().directory));
    }

    *projectChoicesParam  = projectChoices;
    *projectToolTipsParam = projectToolTips;
    *projectActionParam = projectAction;
}

void ProjectFileWizardExtension::initProjectChoices(const QList<GeneratedFile> generatedFiles, const QVariantMap &extraValues)
{
    QStringList projectChoices;
    QStringList projectToolTips;
    ProjectExplorer::ProjectAction projectAction;

    getProjectChoicesAndToolTips(&projectChoices, &projectToolTips, &projectAction,
                                 generatedFiles, m_context,
                                 extraValues.value(QLatin1String(Constants::PREFERRED_PROJECT_NODE)).value<Node *>());

    m_context->page->setProjects(projectChoices);
    m_context->page->setProjectToolTips(projectToolTips);
    m_context->page->setAddingSubProject(projectAction == ProjectExplorer::AddSubProject);
}

bool ProjectFileWizardExtension::processFiles(
        const QList<GeneratedFile> &files,
        bool *removeOpenProjectAttribute, QString *errorMessage)
{
    if (!processProject(files, removeOpenProjectAttribute, errorMessage))
        return false;
    if (!processVersionControl(files, errorMessage)) {
        QString message;
        if (errorMessage) {
            message = *errorMessage;
            message.append(QLatin1String("\n\n"));
            errorMessage->clear();
        }
        message.append(tr("Open project anyway?"));
        if (QMessageBox::question(ICore::mainWindow(), tr("Version Control Failure"), message,
                                  QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
            return false;
    }
    return true;
}

// Add files to project && version control
bool ProjectFileWizardExtension::processProject(
        const QList<GeneratedFile> &files,
        bool *removeOpenProjectAttribute, QString *errorMessage)
{
    *removeOpenProjectAttribute = false;

    QString generatedProject = generatedProjectFilePath(files);

    // Add files to folder (Entry at 0 is 'None').
    const int folderIndex = m_context->page->currentProjectIndex() - 1;
    if (folderIndex < 0 || folderIndex >= m_context->projects.size())
        return true;
    FolderNode *folder = m_context->projects.at(folderIndex).node;
    if (m_context->wizard->kind() == IWizard::ProjectWizard) {
        if (!static_cast<ProjectNode *>(folder)->addSubProjects(QStringList(generatedProject))) {
            *errorMessage = tr("Failed to add subproject \"%1\"\nto project \"%2\".")
                            .arg(generatedProject).arg(folder->path());
            return false;
        }
        *removeOpenProjectAttribute = true;
    } else {
        QStringList filePaths;
        foreach (const GeneratedFile &generatedFile, files)
            filePaths << generatedFile.path();
        if (!folder->addFiles(filePaths)) {
            *errorMessage = tr("Failed to add one or more files to project\n\"%1\" (%2).").
                    arg(folder->path(), filePaths.join(QString(QLatin1Char(','))));
            return false;
        }
    }
    return true;
}

bool ProjectFileWizardExtension::processVersionControl(const QList<GeneratedFile> &files, QString *errorMessage)
{
    // Add files to  version control (Entry at 0 is 'None').
    const int vcsIndex = m_context->page->versionControlIndex() - 1;
    if (vcsIndex < 0 || vcsIndex >= m_context->activeVersionControls.size())
        return true;
    QTC_ASSERT(!m_context->commonDirectory.isEmpty(), return false);
    IVersionControl *versionControl = m_context->activeVersionControls.at(vcsIndex);
    // Create repository?
    if (!m_context->repositoryExists) {
        QTC_ASSERT(versionControl->supportsOperation(IVersionControl::CreateRepositoryOperation), return false);
        if (!versionControl->vcsCreateRepository(m_context->commonDirectory)) {
            *errorMessage = tr("A version control system repository could not be created in \"%1\".").arg(m_context->commonDirectory);
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

static ICodeStylePreferences *codeStylePreferences(Project *project, Id languageId)
{
    if (!languageId.isValid())
        return 0;

    if (project)
        return project->editorConfiguration()->codeStyle(languageId);

    return TextEditorSettings::codeStyle(languageId);
}

void ProjectFileWizardExtension::applyCodeStyle(GeneratedFile *file) const
{
    if (file->isBinary() || file->contents().isEmpty())
        return; // nothing to do

    MimeType mt = MimeDatabase::findByFile(QFileInfo(file->path()));
    Id languageId = TextEditorSettings::languageId(mt.type());

    if (!languageId.isValid())
        return; // don't modify files like *.ui *.pro

    FolderNode *folder = 0;
    const int projectIndex = m_context->page->currentProjectIndex() - 1;
    if (projectIndex >= 0 && projectIndex < m_context->projects.size())
        folder = m_context->projects.at(projectIndex).node;

    Project *baseProject = SessionManager::projectForNode(folder);

    ICodeStylePreferencesFactory *factory = TextEditorSettings::codeStyleFactory(languageId);

    Indenter *indenter = 0;
    if (factory)
        indenter = factory->createIndenter();
    if (!indenter)
        indenter = new NormalIndenter();

    ICodeStylePreferences *codeStylePrefs = codeStylePreferences(baseProject, languageId);
    indenter->setCodeStylePreferences(codeStylePrefs);
    QTextDocument doc(file->contents());
    QTextCursor cursor(&doc);
    cursor.select(QTextCursor::Document);
    indenter->indent(&doc, cursor, QChar::Null, codeStylePrefs->currentTabSettings());
    delete indenter;
    if (TextEditorSettings::storageSettings().m_cleanWhitespace) {
        QTextBlock block = doc.firstBlock();
        while (block.isValid()) {
            codeStylePrefs->currentTabSettings().removeTrailingWhitespace(cursor, block);
            block = block.next();
        }
    }
    file->setContents(doc.toPlainText());
}

void ProjectFileWizardExtension::hideProjectComboBox()
{
    m_context->page->setProjectComoBoxVisible(false);
}

void ProjectFileWizardExtension::setProjectIndex(int i)
{
    m_context->page->setCurrentProjectIndex(i);
}

} // namespace Internal
} // namespace ProjectExplorer

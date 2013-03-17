/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include <projectexplorer/project.h>
#include <projectexplorer/editorconfiguration.h>

#include <QtAlgorithms>
#include <QDebug>
#include <QFileInfo>
#include <QMultiMap>
#include <QDir>
#include <QTextDocument>
#include <QTextCursor>
#include <QMessageBox>

/*!
    \class ProjectExplorer::Internal::ProjectFileWizardExtension

    \brief Post-file generating steps of a project wizard.

    Offers:
    \list
    \li Add to a project file (*.pri/ *.pro)
    \li Initialize a version control repository (unless the path is already
        managed) and do 'add' if the VCS supports it.
    \endlist

    \sa ProjectExplorer::Internal::ProjectWizardPage
*/

enum { debugExtension = 0 };

namespace ProjectExplorer {

typedef QList<ProjectNode *> ProjectNodeList;

namespace Internal {

// AllProjectNodesVisitor: Retrieve all projects (*.pri/*.pro)
// which support adding files
class AllProjectNodesVisitor : public NodesVisitor
{
public:
    AllProjectNodesVisitor(ProjectNode::ProjectAction action)
        : m_action(action)
        {}

    static ProjectNodeList allProjects(ProjectNode::ProjectAction action);

    virtual void visitProjectNode(ProjectNode *node);

private:
    ProjectNodeList m_projectNodes;
    ProjectNode::ProjectAction m_action;
};

ProjectNodeList AllProjectNodesVisitor::allProjects(ProjectNode::ProjectAction action)
{
    AllProjectNodesVisitor visitor(action);
    ProjectExplorerPlugin::instance()->session()->sessionNode()->accept(&visitor);
    return visitor.m_projectNodes;
}

void AllProjectNodesVisitor::visitProjectNode(ProjectNode *node)
{
    if (node->supportedActions(node).contains(m_action))
        m_projectNodes.push_back(node);
}

// ProjectEntry: Context entry for a *.pri/*.pro file. Stores name and path
// for quick sort and path search, provides operator<() for maps.
struct ProjectEntry {
    enum Type { ProFile, PriFile }; // Sort order: 'pro' before 'pri'

    ProjectEntry() : node(0), type(ProFile) {}
    explicit ProjectEntry(ProjectNode *node);

    int compare(const ProjectEntry &rhs) const;

    ProjectNode *node;
    QString directory; // For matching against wizards' files, which are native.
    QString fileName;
    QString baseName;
    Type type;
};

ProjectEntry::ProjectEntry(ProjectNode *n) :
    node(n),
    type(ProFile)
{
    const QFileInfo fi(node->path());
    fileName = fi.fileName();
    baseName = fi.baseName();
    if (fi.suffix() != QLatin1String("pro"))
        type = PriFile;
    directory = fi.absolutePath();
}

// Sort helper that sorts by base name and puts '*.pro' before '*.pri'
int ProjectEntry::compare(const ProjectEntry &rhs) const
{
    if (const int drc = directory.compare(rhs.directory))
        return drc;
    if (const int brc = baseName.compare(rhs.baseName))
        return brc;
    if (type < rhs.type)
        return -1;
    if (type > rhs.type)
        return 1;
    return 0;
}

inline bool operator<(const ProjectEntry &pe1, const ProjectEntry &pe2)
{
    return pe1.compare(pe2) < 0;
}

QDebug operator<<(QDebug d, const ProjectEntry &e)
{
    d.nospace() << e.directory << ' ' << e.fileName << ' ' << e.type;
    return d;
}

// --------- ProjectWizardContext
struct ProjectWizardContext
{
    ProjectWizardContext();
    void clear();

    QList<Core::IVersionControl*> versionControls;
    QList<Core::IVersionControl*> activeVersionControls;
    QList<ProjectEntry> projects;
    ProjectWizardPage *page;
    bool repositoryExists; // Is VCS 'add' sufficient, or should a repository be created?
    QString commonDirectory;
    const Core::IWizard *wizard;
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

static QList<ProjectEntry> findDeployProject(const QList<ProjectEntry> &projects,
                                 QString &commonPath)
{
    QList<ProjectEntry> filtered;
    foreach (const ProjectEntry &project, projects)
        if (project.node->deploysFolder(commonPath))
            filtered << project;
    return filtered;
}

// Find the project the new files should be added to given their common
// path. Either a direct match on the directory or the directory with
// the longest matching path (list containing"/project/subproject1" matching
// common path "/project/subproject1/newuserpath").
static int findMatchingProject(const QList<ProjectEntry> &projects,
                               const QString &commonPath,
                               const QString &preferedProjectNode)
{
    if (projects.isEmpty() || commonPath.isEmpty())
        return -1;

    const int count = projects.size();
    if (!preferedProjectNode.isEmpty()) {
        for (int p = 0; p < count; ++p) {
            if (projects.at(p).node->path() == preferedProjectNode)
                return p;
        }
    }

    int bestMatch = -1;
    int bestMatchLength = 0;
    bool bestMatchIsProFile = false;
    for (int p = 0; p < count; p++) {
        // Direct match or better match? (note that the wizards' files are native).
        const ProjectEntry &entry = projects.at(p);
        const QString &projectDirectory = entry.directory;
        const int projectDirectorySize = projectDirectory.size();
        if (projectDirectorySize == bestMatchLength && bestMatchIsProFile)
            continue; // prefer first pro file over all other files with same bestMatchLength
        if (projectDirectorySize == bestMatchLength && entry.type == ProjectEntry::PriFile)
            continue; // we already have a match with same bestMatchLength that is at least a pri file
        if (projectDirectorySize >= bestMatchLength
                && commonPath.startsWith(projectDirectory)) {
            bestMatchIsProFile = (entry.type == ProjectEntry::ProFile);
            bestMatchLength = projectDirectory.size();
            bestMatch = p;
        }
    }
    return bestMatch;
}

static QString generatedProjectFilePath(const QList<Core::GeneratedFile> &files)
{
    foreach (const Core::GeneratedFile &file, files)
        if (file.attributes() & Core::GeneratedFile::OpenProjectAttribute)
            return file.path();
    return QString();
}

void ProjectFileWizardExtension::firstExtensionPageShown(
        const QList<Core::GeneratedFile> &files,
        const QVariantMap &extraValues)
{
    initProjectChoices(generatedProjectFilePath(files));

    if (debugExtension)
        qDebug() << Q_FUNC_INFO << files.size();
    // Parametrize wizard page: find best project to add to, set up files display and
    // version control depending on path
    QStringList fileNames;
    foreach (const Core::GeneratedFile &f, files)
        fileNames.push_back(f.path());
    m_context->commonDirectory = Utils::commonPath(fileNames);
    m_context->page->setFilesDisplay(m_context->commonDirectory, fileNames);
    // Find best project (Entry at 0 is 'None').

    int bestProjectIndex = -1;

    QList<ProjectEntry> deployingProjects = findDeployProject(m_context->projects, m_context->commonDirectory);
    if (!deployingProjects.isEmpty()) {
        // Oh we do have someone that deploys it
        // then the best match is NONE
        // We display a label explaining that and rename <None> to
        // <Implicitly Add>
        m_context->page->setNoneLabel(tr("<Implicitly Add>"));

        QString text = tr("The files are implicitly added to the projects:\n");
        foreach (const ProjectEntry &project, deployingProjects) {
            text += project.fileName;
            text += QLatin1Char('\n');
        }

        m_context->page->setAdditionalInfo(text);
        bestProjectIndex = -1;
    } else {
        bestProjectIndex = findMatchingProject(m_context->projects, m_context->commonDirectory,
                                               extraValues.value(QLatin1String(Constants::PREFERED_PROJECT_NODE)).toString());
        m_context->page->setNoneLabel(tr("<None>"));
    }

    if (bestProjectIndex == -1)
        m_context->page->setCurrentProjectIndex(0);
    else
        m_context->page->setCurrentProjectIndex(bestProjectIndex + 1);

    // Store all version controls for later use:
    if (m_context->versionControls.isEmpty()) {
        foreach (Core::IVersionControl *vc, ExtensionSystem::PluginManager::getObjects<Core::IVersionControl>()) {
            m_context->versionControls.append(vc);
            connect(vc, SIGNAL(configurationChanged()), this, SLOT(initializeVersionControlChoices()));
        }
    }

    initializeVersionControlChoices();
}

void ProjectFileWizardExtension::initializeVersionControlChoices()
{
    // Figure out version control situation:
    // 1) Directory is managed and VCS supports "Add" -> List it
    // 2) Directory is managed and VCS does not support "Add" -> None available
    // 3) Directory is not managed -> Offer all VCS that support "CreateRepository"

    Core::IVersionControl *currentSelection = 0;
    int currentIdx = m_context->page->versionControlIndex() - 1;
    if (currentIdx >= 0 && currentIdx <= m_context->activeVersionControls.size() - 1)
        currentSelection = m_context->activeVersionControls.at(currentIdx);

    m_context->activeVersionControls.clear();

    QStringList versionControlChoices = QStringList(tr("<None>"));
    if (!m_context->commonDirectory.isEmpty()) {
        Core::IVersionControl *managingControl = Core::ICore::vcsManager()->findVersionControlForDirectory(m_context->commonDirectory);
        if (managingControl) {
            // Under VCS
            if (managingControl->supportsOperation(Core::IVersionControl::AddOperation)) {
                versionControlChoices.append(managingControl->displayName());
                m_context->activeVersionControls.push_back(managingControl);
                m_context->repositoryExists = true;
            }
        } else {
            // Create
            foreach (Core::IVersionControl *vc, m_context->versionControls)
                if (vc->supportsOperation(Core::IVersionControl::CreateRepositoryOperation)) {
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

QList<QWizardPage *> ProjectFileWizardExtension::extensionPages(const Core::IWizard *wizard)
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
                                                ProjectNode::ProjectAction *projectActionParam,
                                                const QString &generatedProjectFilePath,
                                                ProjectWizardContext *context)
{
    // Set up project list which remains the same over duration of wizard execution
    // As tooltip, set the directory for disambiguation (should someone have
    // duplicate base names in differing directories).
    //: No project selected
    QStringList projectChoices(ProjectFileWizardExtension::tr("<None>"));
    QStringList projectToolTips((QString()));

    typedef QMap<ProjectEntry, bool> ProjectEntryMap;
    // Sort by base name and purge duplicated entries (resulting from dependencies)
    // via Map.
    ProjectEntryMap entryMap;

    ProjectNode::ProjectAction projectAction =
           context->wizard->kind() == Core::IWizard::ProjectWizard
            ? ProjectNode::AddSubProject : ProjectNode::AddNewFile;
    foreach (ProjectNode *n, AllProjectNodesVisitor::allProjects(projectAction)) {
        if (projectAction == ProjectNode::AddNewFile
                || (projectAction == ProjectNode::AddSubProject
                && (generatedProjectFilePath.isEmpty() ? true : n->canAddSubProject(generatedProjectFilePath))))
            entryMap.insert(ProjectEntry(n), true);
    }

    context->projects.clear();

    // Collect names
    const ProjectEntryMap::const_iterator cend = entryMap.constEnd();
    for (ProjectEntryMap::const_iterator it = entryMap.constBegin(); it != cend; ++it) {
        context->projects.push_back(it.key());
        projectChoices.push_back(it.key().fileName);
        projectToolTips.push_back(QDir::toNativeSeparators(it.key().directory));
    }
    *projectChoicesParam  = projectChoices;
    *projectToolTipsParam = projectToolTips;
    *projectActionParam = projectAction;
}

void ProjectFileWizardExtension::initProjectChoices(const QString &generatedProjectFilePath)
{
    QStringList projectChoices;
    QStringList projectToolTips;
    ProjectNode::ProjectAction projectAction;

    getProjectChoicesAndToolTips(&projectChoices, &projectToolTips, &projectAction,
                                 generatedProjectFilePath, m_context);

    m_context->page->setProjects(projectChoices);
    m_context->page->setProjectToolTips(projectToolTips);
    m_context->page->setAddingSubProject(projectAction == ProjectNode::AddSubProject);
}

bool ProjectFileWizardExtension::processFiles(
        const QList<Core::GeneratedFile> &files,
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
        if (QMessageBox::question(Core::ICore::mainWindow(), tr("Version Control Failure"), message,
                                  QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
            return false;
    }
    return true;
}

// Add files to project && version control
bool ProjectFileWizardExtension::processProject(
        const QList<Core::GeneratedFile> &files,
        bool *removeOpenProjectAttribute, QString *errorMessage)
{
    typedef QMultiMap<FileType, QString> TypeFileMap;

    *removeOpenProjectAttribute = false;

    QString generatedProject = generatedProjectFilePath(files);

    // Add files to  project (Entry at 0 is 'None').
    const int projectIndex = m_context->page->currentProjectIndex() - 1;
    if (projectIndex < 0 || projectIndex >= m_context->projects.size())
        return true;
    ProjectNode *project = m_context->projects.at(projectIndex).node;
    if (m_context->wizard->kind() == Core::IWizard::ProjectWizard) {
        if (!project->addSubProjects(QStringList(generatedProject))) {
            *errorMessage = tr("Failed to add subproject '%1'\nto project '%2'.")
                            .arg(generatedProject).arg(project->path());
            return false;
        }
        *removeOpenProjectAttribute = true;
    } else {
        // Split into lists by file type and bulk-add them.
        TypeFileMap typeFileMap;
        const Core::MimeDatabase *mdb = Core::ICore::mimeDatabase();
        foreach (const Core::GeneratedFile &generatedFile, files) {
            const QString path = generatedFile.path();
            typeFileMap.insert(typeForFileName(mdb, path), path);
        }
        foreach (FileType type, typeFileMap.uniqueKeys()) {
            const QStringList typeFiles = typeFileMap.values(type);
            if (!project->addFiles(type, typeFiles)) {
                *errorMessage = tr("Failed to add one or more files to project\n'%1' (%2).").
                                arg(project->path(), typeFiles.join(QString(QLatin1Char(','))));
                return false;
            }
        }
    }
    return true;
}

bool ProjectFileWizardExtension::processVersionControl(const QList<Core::GeneratedFile> &files, QString *errorMessage)
{
    // Add files to  version control (Entry at 0 is 'None').
    const int vcsIndex = m_context->page->versionControlIndex() - 1;
    if (vcsIndex < 0 || vcsIndex >= m_context->activeVersionControls.size())
        return true;
    QTC_ASSERT(!m_context->commonDirectory.isEmpty(), return false);
    Core::IVersionControl *versionControl = m_context->activeVersionControls.at(vcsIndex);
    // Create repository?
    if (!m_context->repositoryExists) {
        QTC_ASSERT(versionControl->supportsOperation(Core::IVersionControl::CreateRepositoryOperation), return false);
        if (!versionControl->vcsCreateRepository(m_context->commonDirectory)) {
            *errorMessage = tr("A version control system repository could not be created in '%1'.").arg(m_context->commonDirectory);
            return false;
        }
    }
    // Add files if supported.
    if (versionControl->supportsOperation(Core::IVersionControl::AddOperation)) {
        foreach (const Core::GeneratedFile &generatedFile, files) {
            if (!versionControl->vcsAdd(generatedFile.path())) {
                *errorMessage = tr("Failed to add '%1' to the version control system.").arg(generatedFile.path());
                return false;
            }
        }
    }
    return true;
}

static TextEditor::ICodeStylePreferences *codeStylePreferences(ProjectExplorer::Project *project, Core::Id languageId)
{
    if (!languageId.isValid())
        return 0;

    if (project)
        return project->editorConfiguration()->codeStyle(languageId);

    return TextEditor::TextEditorSettings::instance()->codeStyle(languageId);
}

void ProjectFileWizardExtension::applyCodeStyle(Core::GeneratedFile *file) const
{
    if (file->isBinary() || file->contents().isEmpty())
        return; // nothing to do

    const Core::MimeDatabase *mdb = Core::ICore::mimeDatabase();
    Core::MimeType mt = mdb->findByFile(QFileInfo(file->path()));
    Core::Id languageId = TextEditor::TextEditorSettings::instance()->languageId(mt.type());

    if (!languageId.isValid())
        return; // don't modify files like *.ui *.pro

    ProjectNode *project = 0;
    const int projectIndex = m_context->page->currentProjectIndex() - 1;
    if (projectIndex >= 0 && projectIndex < m_context->projects.size())
        project = m_context->projects.at(projectIndex).node;

    ProjectExplorer::Project *baseProject
            = ProjectExplorer::ProjectExplorerPlugin::instance()->session()->projectForNode(project);

    TextEditor::ICodeStylePreferencesFactory *factory
            = TextEditor::TextEditorSettings::instance()->codeStyleFactory(languageId);

    TextEditor::Indenter *indenter = 0;
    if (factory)
        indenter = factory->createIndenter();
    if (!indenter)
        indenter = new TextEditor::NormalIndenter();

    TextEditor::ICodeStylePreferences *codeStylePrefs = codeStylePreferences(baseProject, languageId);
    indenter->setCodeStylePreferences(codeStylePrefs);

    QTextDocument doc(file->contents());
    QTextCursor cursor(&doc);
    cursor.select(QTextCursor::Document);
    indenter->indent(&doc, cursor, QChar::Null, codeStylePrefs->currentTabSettings());
    file->setContents(doc.toPlainText());
    delete indenter;
}

QStringList ProjectFileWizardExtension::getProjectChoices() const
{
    QStringList projectChoices;
    QStringList projectToolTips;
    ProjectNode::ProjectAction projectAction;

    getProjectChoicesAndToolTips(&projectChoices, &projectToolTips, &projectAction,
                                 QString(), m_context);

    return projectChoices;
}

QStringList ProjectFileWizardExtension::getProjectToolTips() const
{
    QStringList projectChoices;
    QStringList projectToolTips;
    ProjectNode::ProjectAction projectAction;

    getProjectChoicesAndToolTips(&projectChoices, &projectToolTips, &projectAction,
                                 QString(), m_context);

    return projectToolTips;
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

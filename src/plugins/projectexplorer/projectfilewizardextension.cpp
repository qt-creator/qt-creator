/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "projectfilewizardextension.h"
#include "projectexplorer.h"
#include "session.h"
#include "projectnodes.h"
#include "nodesvisitor.h"
#include "projectwizardpage.h"

#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <coreplugin/basefilewizard.h>
#include <coreplugin/dialogs/iwizard.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QVariant>
#include <QtCore/QtAlgorithms>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QMultiMap>
#include <QtCore/QDir>

enum { debugExtension = 0 };

namespace ProjectExplorer {

typedef QList<ProjectNode *> ProjectNodeList;

namespace Internal {

// AllProjectNodesVisitor: Retrieve all projects (*.pri/*.pro).
class AllProjectNodesVisitor : public NodesVisitor
{
public:
    static ProjectNodeList allProjects();

    virtual void visitProjectNode(ProjectNode *node);

private:
    ProjectNodeList m_projectNodes;
};

ProjectNodeList AllProjectNodesVisitor::allProjects()
{
    AllProjectNodesVisitor visitor;
    ProjectExplorerPlugin::instance()->session()->sessionNode()->accept(&visitor);
    return visitor.m_projectNodes;
}

void AllProjectNodesVisitor::visitProjectNode(ProjectNode *node)
{
    if (node->supportedActions().contains(ProjectNode::AddFile))
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
    QString nativeDirectory; // For matching against wizards' files, which are native.
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
    nativeDirectory = QDir::toNativeSeparators(fi.absolutePath());
}

// Sort helper that sorts by base name and puts '*.pro' before '*.pri'
int ProjectEntry::compare(const ProjectEntry &rhs) const
{
    if (const int drc = nativeDirectory.compare(rhs.nativeDirectory))
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
    d.nospace() << e.nativeDirectory << ' ' << e.fileName << ' ' << e.type;
    return d;
}

// --------- ProjectWizardContext
struct ProjectWizardContext
{
    ProjectWizardContext();
    void clear();

    QList<Core::IVersionControl*> versionControls;
    QList<ProjectEntry> projects;
    ProjectWizardPage *page;
    bool repositoryExists; // Is VCS 'add' sufficient, or should a repository be created?
    QString commonDirectory;
};

ProjectWizardContext::ProjectWizardContext() :
    page(0),
    repositoryExists(false)
{
}

void ProjectWizardContext::clear()
{
    versionControls.clear();
    projects.clear();
    commonDirectory.clear();
    page = 0;
    repositoryExists = false;
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

// Find the project the new files should be added to given their common
// path. Either a direct match on the directory or the directory with
// the longest matching path (list containing"/project/subproject1" matching
// common path "/project/subproject1/newuserpath").
// This relies on 'pro' occurring before 'pri' in the list.
static int findMatchingProject(const QList<ProjectEntry> &projects,
                               const QString &commonPath)
{
    if (projects.isEmpty() || commonPath.isEmpty())
        return -1;

    int bestMatch = -1;
    int bestMatchLength = 0;
    const int count = projects.size();
    for (int p = 0; p < count; p++) {
        // Direct match or better match? (note that the wizards' files are native).
        const QString &projectDirectory = projects.at(p).nativeDirectory;
        if (projectDirectory == commonPath)
            return p;
        if (projectDirectory.size() > bestMatchLength
            && commonPath.startsWith(projectDirectory)) {
            bestMatchLength = projectDirectory.size();
            bestMatch = p;
        }
    }
    return bestMatch;
}

void ProjectFileWizardExtension::firstExtensionPageShown(const QList<Core::GeneratedFile> &files)
{
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
    const int bestProjectIndex = findMatchingProject(m_context->projects, m_context->commonDirectory);
    if (bestProjectIndex == -1) {
        m_context->page->setCurrentProjectIndex(0);
    } else {
        m_context->page->setCurrentProjectIndex(bestProjectIndex + 1);
    }
    initializeVersionControlChoices();
}

void ProjectFileWizardExtension::initializeVersionControlChoices()
{
    // Figure out version control situation:
    // 1) Directory is managed and VCS supports "Add" -> List it
    // 2) Directory is managed and VCS does not support "Add" -> None available
    // 3) Directory is not managed -> Offer all VCS that support "CreateRepository"
    m_context->versionControls.clear();
    if (!m_context->commonDirectory.isEmpty()) {
        Core::IVersionControl *managingControl = Core::ICore::instance()->vcsManager()->findVersionControlForDirectory(m_context->commonDirectory);
        if (managingControl) {
            // Under VCS
            if (managingControl->supportsOperation(Core::IVersionControl::AddOperation)) {
                m_context->versionControls.push_back(managingControl);
                m_context->repositoryExists = true;
            }
        } else {
            // Create
            foreach (Core::IVersionControl *vc, ExtensionSystem::PluginManager::instance()->getObjects<Core::IVersionControl>())
                if (vc->supportsOperation(Core::IVersionControl::CreateRepositoryOperation))
                    m_context->versionControls.push_back(vc);
            m_context->repositoryExists = false;
        }
    } // has a common root.
    // Compile names
    //: No version control system selected
    QStringList versionControlChoices = QStringList(tr("<None>"));
    foreach(const Core::IVersionControl *c, m_context->versionControls)
        versionControlChoices.push_back(c->displayName());
    m_context->page->setVersionControls(versionControlChoices);
    // Enable adding to version control by default.
    if (m_context->repositoryExists && versionControlChoices.size() >= 2)
        m_context->page->setVersionControlIndex(1);
}

QList<QWizardPage *> ProjectFileWizardExtension::extensionPages(const Core::IWizard *wizard)
{
    if (!m_context) {
        m_context = new ProjectWizardContext;
    } else {
        m_context->clear();
    }
    // Init context with page and projects
    m_context->page = new ProjectWizardPage;
    // Project list remains the same over duration of wizard execution
    // Note that projects cannot be added to projects.
    initProjectChoices(wizard->kind() != Core::IWizard::ProjectWizard);
    return QList<QWizardPage *>() << m_context->page;
}

void ProjectFileWizardExtension::initProjectChoices(bool enabled)
{
    // Set up project list which remains the same over duration of wizard execution
    // As tooltip, set the directory for disambiguation (should someone have
    // duplicate base names in differing directories).
    //: No project selected
    QStringList projectChoices(tr("<None>"));
    QStringList projectToolTips( QString::null ); // Do not use QString() - gcc-bug.
    if (enabled) {
        typedef QMap<ProjectEntry, bool> ProjectEntryMap;
        // Sort by base name and purge duplicated entries (resulting from dependencies)
        // via Map.
        ProjectEntryMap entryMap;
        foreach(ProjectNode *n, AllProjectNodesVisitor::allProjects())
            entryMap.insert(ProjectEntry(n), true);
        // Collect names
        const ProjectEntryMap::const_iterator cend = entryMap.constEnd();
        for (ProjectEntryMap::const_iterator it = entryMap.constBegin(); it != cend; ++it) {
            m_context->projects.push_back(it.key());
            projectChoices.push_back(it.key().fileName);
            projectToolTips.push_back(it.key().nativeDirectory);
        }
    }
    m_context->page->setProjects(projectChoices);
    m_context->page->setProjectToolTips(projectToolTips);
}

bool ProjectFileWizardExtension::process(const QList<Core::GeneratedFile> &files, QString *errorMessage)
{
    return processProject(files, errorMessage) &&
           processVersionControl(files, errorMessage);
}

// Add files to project && version control
bool ProjectFileWizardExtension::processProject(const QList<Core::GeneratedFile> &files, QString *errorMessage)
{
    typedef QMultiMap<FileType, QString> TypeFileMap;

    // Add files to  project (Entry at 0 is 'None').
    const int projectIndex = m_context->page->currentProjectIndex() - 1;
    if (projectIndex < 0 || projectIndex >= m_context->projects.size())
        return true;
    ProjectNode *project = m_context->projects.at(projectIndex).node;
    // Split into lists by file type and bulk-add them.
    TypeFileMap typeFileMap;
    const Core::MimeDatabase *mdb = Core::ICore::instance()->mimeDatabase();
    foreach (const Core::GeneratedFile &generatedFile, files) {
        const QString path = generatedFile.path();
        typeFileMap.insert(typeForFileName(mdb, path), path);
    }
    foreach (FileType type, typeFileMap.uniqueKeys()) {
        const QStringList files = typeFileMap.values(type);
        if (!project->addFiles(type, files)) {
            *errorMessage = tr("Failed to add one or more files to project\n'%1' (%2).").
                            arg(project->path(), files.join(QString(QLatin1Char(','))));
            return false;
        }
    }
    return true;
}

bool ProjectFileWizardExtension::processVersionControl(const QList<Core::GeneratedFile> &files, QString *errorMessage)
{
    // Add files to  version control (Entry at 0 is 'None').
    const int vcsIndex = m_context->page->versionControlIndex() - 1;
    if (vcsIndex < 0 || vcsIndex >= m_context->versionControls.size())
        return true;
    QTC_ASSERT(!m_context->commonDirectory.isEmpty(), return false);
    Core::IVersionControl *versionControl = m_context->versionControls.at(vcsIndex);
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

} // namespace Internal
} // namespace ProjectExplorer

/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#include "projectfilewizardextension.h"
#include "projectexplorer.h"
#include "projectnodes.h"
#include "nodesvisitor.h"
#include "projectwizardpage.h"

#include <coreplugin/basefilewizard.h>
#include <coreplugin/dialogs/iwizard.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QMultiMap>

enum { debugExtension = 0 };

namespace ProjectExplorer {

typedef QList<ProjectNode *> ProjectNodeList;

namespace Internal {

// --------- AllProjectNodesVisitor. Figure out all projects.
// No sooner said then done.
class AllProjectNodesVisitor : public NodesVisitor
{
    AllProjectNodesVisitor(ProjectNodeList &l) : m_projectNodes(l) {}
public:

    static ProjectNodeList allProjects(const ProjectExplorerPlugin *pe);

    virtual void visitProjectNode(ProjectNode *node);

private:
    ProjectNodeList &m_projectNodes;
};

ProjectNodeList AllProjectNodesVisitor::allProjects(const ProjectExplorerPlugin *pe)
{
    ProjectNodeList rc;
    AllProjectNodesVisitor visitor(rc);
    pe->session()->sessionNode()->accept(&visitor);
    return rc;
}

void AllProjectNodesVisitor::visitProjectNode(ProjectNode *node)
{
    if (node->supportedActions().contains(ProjectNode::AddFile))
        m_projectNodes << node;
}

// --------- ProjectWizardContext
struct ProjectWizardContext {
    Core::IVersionControl *versionControl;
    ProjectNodeList projects;
    ProjectWizardPage *page;
};

// ---- ProjectFileWizardExtension
ProjectFileWizardExtension::ProjectFileWizardExtension(Core::ICore *core) :
    m_core(core),
    m_context(0)
{
}

ProjectFileWizardExtension::~ProjectFileWizardExtension()
{
    delete m_context;
}

void ProjectFileWizardExtension::firstExtensionPageShown(const QList<Core::GeneratedFile> &files)
{
    if (debugExtension)
        qDebug() << Q_FUNC_INFO << files.size();
    // Setup files display and version control depending on path
    QStringList fileNames;
    foreach (const Core::GeneratedFile &f, files)
        fileNames.push_back(f.path());

    const QString directory = QFileInfo(fileNames.front()).absolutePath();
    m_context->versionControl = m_core->vcsManager()->findVersionControlForDirectory(directory);

    m_context->page->setFilesDisplay(fileNames);
    m_context->page->setAddToVersionControlEnabled(m_context->versionControl != 0);
}

static ProjectNode *currentProject()
{
    if (Node *currentNode = ProjectExplorerPlugin::instance()->currentNode())
        if (ProjectNode *currentProjectNode = qobject_cast<ProjectNode *>(currentNode))
            return currentProjectNode;
    return 0;
}

QList<QWizardPage *> ProjectFileWizardExtension::extensionPages(const Core::IWizard *wizard)
{
    if (!m_context)
        m_context = new ProjectWizardContext;
    // Init context with page and projects
    m_context->page = new ProjectWizardPage;
    m_context->versionControl = 0;
    m_context->projects = AllProjectNodesVisitor::allProjects(ProjectExplorerPlugin::instance());
    // Set up project list which remains the same over duration of wizard execution
    // Disable "add project to project"
    const bool hasProjects = !m_context->projects.empty();
    if (hasProjects) {
        // Compile list of names and find current project if there is one
        QStringList projectNames;
        ProjectNode *current = currentProject();
        int currentIndex = -1;
        const int count = m_context->projects.size();
        for (int i = 0; i < count; i++) {
            ProjectNode *pn = m_context->projects.at(i);
            projectNames.push_back(pn->name());
            if (current == pn)
                currentIndex = i;
        }
        m_context->page->setProjects(projectNames);
        if (currentIndex != -1)
            m_context->page->setCurrentProjectIndex(currentIndex);
    }
    m_context->page->setAddToProjectEnabled(hasProjects && wizard->kind() != Core::IWizard::ProjectWizard);

    return QList<QWizardPage *>() << m_context->page;
}

bool ProjectFileWizardExtension::process(const QList<Core::GeneratedFile> &files, QString *errorMessage)
{
    typedef QMultiMap<FileType, QString> TypeFileMap;
    // Add files to project && version control
    if (m_context->page->addToProject()) {
        ProjectNode *project = m_context->projects.at(m_context->page->currentProjectIndex());
        // Split into lists by file type and add
        TypeFileMap typeFileMap;
        foreach (const Core::GeneratedFile &generatedFile, files) {
            const QString path = generatedFile.path();
            typeFileMap.insert(typeForFileName(m_core->mimeDatabase(), path), path);
        }
        foreach (FileType type, typeFileMap.uniqueKeys()) {
            const QStringList files = typeFileMap.values(type);
            if (!project->addFiles(type, files)) {
                *errorMessage = tr("Failed to add one or more files to project\n'%1' (%2).").
                    arg(project->path(), files.join(QLatin1String(",")));
                return false;
            }
        }
    }
    // Add files to  version control
    if (m_context->page->addToVersionControl()) {
        foreach (const Core::GeneratedFile &generatedFile, files) {
            if (!m_context->versionControl->vcsAdd(generatedFile.path())) {
                *errorMessage = tr("Failed to add '%1' to the version control system.").arg(generatedFile.path());
                return false;
            }
        }
    }

    return true;
}


}
}

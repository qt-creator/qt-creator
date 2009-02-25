/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

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
struct ProjectWizardContext
{
    Core::IVersionControl *versionControl;
    ProjectNodeList projects;
    ProjectWizardPage *page;
};

// ---- ProjectFileWizardExtension
ProjectFileWizardExtension::ProjectFileWizardExtension()
  : m_context(0)
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
    m_context->versionControl = Core::ICore::instance()->vcsManager()->findVersionControlForDirectory(directory);

    m_context->page->setFilesDisplay(fileNames);

    const bool canAddToVCS = m_context->versionControl && m_context->versionControl->supportsOperation(Core::IVersionControl::AddOperation);
    if (m_context->versionControl)
         m_context->page->setVCSDisplay(m_context->versionControl->name());
    m_context->page->setAddToVersionControlEnabled(canAddToVCS);
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
            projectNames.push_back(QFileInfo(pn->path()).fileName());
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
            typeFileMap.insert(typeForFileName(Core::ICore::instance()->mimeDatabase(), path), path);
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

} // namespace Internal
} // namespace ProjectExplorer

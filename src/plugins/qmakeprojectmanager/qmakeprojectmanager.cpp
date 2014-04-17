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

#include "qmakeprojectmanager.h"

#include "qmakeprojectmanagerconstants.h"
#include "qmakeprojectmanagerplugin.h"
#include "qmakenodes.h"
#include "qmakeproject.h"
#include "profileeditor.h"
#include "qmakestep.h"
#include "qmakebuildconfiguration.h"
#include "addlibrarywizard.h"
#include "wizards/qtquickapp.h"
#include "wizards/html5app.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>
#include <QVariant>
#include <QMessageBox>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;

QmakeManager::QmakeManager(QmakeProjectManagerPlugin *plugin)
  : m_plugin(plugin),
    m_contextNode(0),
    m_contextProject(0),
    m_contextFile(0)
{
}

QmakeManager::~QmakeManager()
{
}

void QmakeManager::registerProject(QmakeProject *project)
{
    m_projects.append(project);
}

void QmakeManager::unregisterProject(QmakeProject *project)
{
    m_projects.removeOne(project);
}

void QmakeManager::notifyChanged(const QString &name)
{
    foreach (QmakeProject *pro, m_projects)
        pro->notifyChanged(name);
}

QString QmakeManager::mimeType() const
{
    return QLatin1String(QmakeProjectManager::Constants::PROFILE_MIMETYPE);
}

ProjectExplorer::Project *QmakeManager::openProject(const QString &fileName, QString *errorString)
{
    if (!QFileInfo(fileName).isFile()) {
        if (errorString)
            *errorString = tr("Failed opening project \"%1\": Project is not a file")
                .arg(fileName);
        return 0;
    }

    return new QmakeProject(this, fileName);
}

ProjectExplorer::Node *QmakeManager::contextNode() const
{
    return m_contextNode;
}

void QmakeManager::setContextNode(ProjectExplorer::Node *node)
{
    m_contextNode = node;
}

ProjectExplorer::Project *QmakeManager::contextProject() const
{
    return m_contextProject;
}

void QmakeManager::setContextProject(ProjectExplorer::Project *project)
{
    m_contextProject = project;
}

ProjectExplorer::FileNode *QmakeManager::contextFile() const
{
    return m_contextFile;
}

void QmakeManager::setContextFile(ProjectExplorer::FileNode *file)
{
    m_contextFile = file;
}

void QmakeManager::addLibrary()
{
    ProFileEditor *editor =
        qobject_cast<ProFileEditor*>(Core::EditorManager::currentEditor());
    if (editor)
        addLibrary(editor->document()->filePath(), editor);
}

void QmakeManager::addLibraryContextMenu()
{
    ProjectExplorer::Node *node = ProjectExplorer::ProjectExplorerPlugin::instance()->currentNode();
    if (qobject_cast<QmakeProFileNode *>(node))
        addLibrary(node->path());
}

void QmakeManager::addLibrary(const QString &fileName, ProFileEditor *editor)
{
    AddLibraryWizard wizard(fileName, Core::EditorManager::instance());
    if (wizard.exec() != QDialog::Accepted)
        return;

    if (!editor)
        editor = qobject_cast<ProFileEditor *> (Core::EditorManager::openEditor(fileName,
            QmakeProjectManager::Constants::PROFILE_EDITOR_ID, Core::EditorManager::DoNotMakeVisible));
    if (!editor)
        return;

    const int endOfDoc = editor->position(TextEditor::ITextEditor::EndOfDoc);
    editor->setCursorPosition(endOfDoc);
    QString snippet = wizard.snippet();

    // add extra \n in case the last line is not empty
    int line, column;
    editor->convertPosition(endOfDoc, &line, &column);
    if (!editor->textDocument()->textAt(endOfDoc - column, column).simplified().isEmpty())
        snippet = QLatin1Char('\n') + snippet;

    editor->insert(snippet);
}


void QmakeManager::runQMake()
{
    runQMake(SessionManager::startupProject(), 0);
}

void QmakeManager::runQMakeContextMenu()
{
    runQMake(m_contextProject, m_contextNode);
}

void QmakeManager::runQMake(ProjectExplorer::Project *p, ProjectExplorer::Node *node)
{
    if (!ProjectExplorer::ProjectExplorerPlugin::instance()->saveModifiedFiles())
        return;
    QmakeProject *qmakeProject = qobject_cast<QmakeProject *>(p);
    QTC_ASSERT(qmakeProject, return);

    if (!qmakeProject->activeTarget() ||
        !qmakeProject->activeTarget()->activeBuildConfiguration())
        return;

    QmakeBuildConfiguration *bc = static_cast<QmakeBuildConfiguration *>(qmakeProject->activeTarget()->activeBuildConfiguration());
    QMakeStep *qs = bc->qmakeStep();
    if (!qs)
        return;

    //found qmakeStep, now use it
    qs->setForced(true);

    if (node != 0 && node != qmakeProject->rootProjectNode())
        if (QmakeProFileNode *profile = qobject_cast<QmakeProFileNode *>(node))
            bc->setSubNodeBuild(profile);

    BuildManager::appendStep(qs, tr("QMake"));
    bc->setSubNodeBuild(0);
}

void QmakeManager::buildSubDirContextMenu()
{
    handleSubDirContextMenu(BUILD, false);
}

void QmakeManager::cleanSubDirContextMenu()
{
    handleSubDirContextMenu(CLEAN, false);
}

void QmakeManager::rebuildSubDirContextMenu()
{
    handleSubDirContextMenu(REBUILD, false);
}

void QmakeManager::buildFileContextMenu()
{
    handleSubDirContextMenu(BUILD, true);
}

void QmakeManager::buildFile()
{
    if (Core::IDocument *currentDocument= Core::EditorManager::currentDocument()) {
        const QString file = currentDocument->filePath();
        FileNode *node  = qobject_cast<FileNode *>(SessionManager::nodeForFile(file));
        Project *project = SessionManager::projectForFile(file);

        if (project && node)
            handleSubDirContextMenu(BUILD, true, project, node->projectNode(), node);
    }
}

void QmakeManager::handleSubDirContextMenu(QmakeManager::Action action, bool isFileBuild)
{
    handleSubDirContextMenu(action, isFileBuild, m_contextProject, m_contextNode, m_contextFile);
}

void QmakeManager::handleSubDirContextMenu(QmakeManager::Action action, bool isFileBuild,
                                         ProjectExplorer::Project *contextProject,
                                         ProjectExplorer::Node *contextNode,
                                         ProjectExplorer::FileNode *contextFile)
{
    QmakeProject *qmakeProject = qobject_cast<QmakeProject *>(contextProject);
    QTC_ASSERT(qmakeProject, return);

    if (!qmakeProject->activeTarget() ||
        !qmakeProject->activeTarget()->activeBuildConfiguration())
    return;

    if (!contextNode || !contextFile)
        isFileBuild = false;
    QmakeBuildConfiguration *bc = qobject_cast<QmakeBuildConfiguration *>(qmakeProject->activeTarget()->activeBuildConfiguration());
    if (!bc)
        return;

    if (contextNode) {
        if (QmakePriFileNode *prifile = qobject_cast<QmakePriFileNode *>(contextNode)) {
            if (QmakeProFileNode *profile = prifile->proFileNode()) {
                if (profile != qmakeProject->rootProjectNode() || isFileBuild)
                    bc->setSubNodeBuild(profile);
            }
        }
    }

    if (isFileBuild)
        bc->setFileNodeBuild(contextFile);
    if (ProjectExplorerPlugin::instance()->saveModifiedFiles()) {
        const Core::Id buildStep = ProjectExplorer::Constants::BUILDSTEPS_BUILD;
        const Core::Id cleanStep = ProjectExplorer::Constants::BUILDSTEPS_CLEAN;
        if (action == BUILD) {
            const QString name = ProjectExplorerPlugin::displayNameForStepId(buildStep);
            BuildManager::buildList(bc->stepList(buildStep), name);
        } else if (action == CLEAN) {
            const QString name = ProjectExplorerPlugin::displayNameForStepId(cleanStep);
            BuildManager::buildList(bc->stepList(cleanStep), name);
        } else if (action == REBUILD) {
            QStringList names;
            names << ProjectExplorerPlugin::displayNameForStepId(cleanStep)
                  << ProjectExplorerPlugin::displayNameForStepId(buildStep);

            QList<ProjectExplorer::BuildStepList *> stepLists;
            stepLists << bc->stepList(cleanStep) << bc->stepList(buildStep);
            BuildManager::buildLists(stepLists, names);
        }
    }

    bc->setSubNodeBuild(0);
    bc->setFileNodeBuild(0);
}

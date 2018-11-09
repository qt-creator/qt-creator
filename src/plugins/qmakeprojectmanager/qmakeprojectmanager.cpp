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

#include "qmakeprojectmanager.h"

#include "qmakeprojectmanagerconstants.h"
#include "qmakeprojectmanagerplugin.h"
#include "qmakenodes.h"
#include "qmakeproject.h"
#include "profileeditor.h"
#include "qmakestep.h"
#include "qmakebuildconfiguration.h"
#include "addlibrarywizard.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>
#include <texteditor/texteditor.h>

#include <QDir>
#include <QFileInfo>
#include <QVariant>

using namespace ProjectExplorer;
using namespace TextEditor;

namespace QmakeProjectManager {

Node *QmakeManager::contextNode()
{
    return ProjectTree::findCurrentNode();
}

Project *QmakeManager::contextProject()
{
    return ProjectTree::currentProject();
}

static QmakeProFileNode *buildableFileProFile(Node *node)
{
    if (node) {
        auto subPriFileNode = dynamic_cast<QmakePriFileNode *>(node);
        if (!subPriFileNode)
            subPriFileNode = dynamic_cast<QmakePriFileNode *>(node->parentProjectNode());
        if (subPriFileNode)
            return subPriFileNode->proFileNode();
    }
    return nullptr;
}

FileNode *QmakeManager::contextBuildableFileNode()
{
    Node *node = contextNode();

    QmakeProFileNode *subProjectNode = buildableFileProFile(node);
    FileNode *fileNode = node ? node->asFileNode() : nullptr;
    bool buildFilePossible = subProjectNode && fileNode
            && (fileNode->fileType() == ProjectExplorer::FileType::Source);

    return buildFilePossible ? fileNode : nullptr;
}

void QmakeManager::addLibrary()
{
    if (auto editor = qobject_cast<BaseTextEditor *>(Core::EditorManager::currentEditor()))
        addLibraryImpl(editor->document()->filePath().toString(), editor);
}

void QmakeManager::addLibraryContextMenu()
{
    QString projectPath;

    Node *node = contextNode();
    if (ContainerNode *cn = node->asContainerNode())
        projectPath = cn->project()->projectFilePath().toString();
    else if (dynamic_cast<QmakeProFileNode *>(node))
        projectPath = node->filePath().toString();

    addLibraryImpl(projectPath, nullptr);
}

void QmakeManager::addLibraryImpl(const QString &fileName, BaseTextEditor *editor)
{
    if (fileName.isEmpty())
        return;

    Internal::AddLibraryWizard wizard(fileName, Core::ICore::dialogParent());
    if (wizard.exec() != QDialog::Accepted)
        return;

    if (!editor)
        editor = qobject_cast<BaseTextEditor *>(Core::EditorManager::openEditor(fileName,
            Constants::PROFILE_EDITOR_ID, Core::EditorManager::DoNotMakeVisible));
    if (!editor)
        return;

    const int endOfDoc = editor->position(EndOfDocPosition);
    editor->setCursorPosition(endOfDoc);
    QString snippet = wizard.snippet();

    // add extra \n in case the last line is not empty
    int line, column;
    editor->convertPosition(endOfDoc, &line, &column);
    const int positionInBlock = column - 1;
    if (!editor->textAt(endOfDoc - positionInBlock, positionInBlock).simplified().isEmpty())
        snippet = QLatin1Char('\n') + snippet;

    editor->insert(snippet);
}

void QmakeManager::runQMake()
{
    runQMakeImpl(SessionManager::startupProject(), nullptr);
}

void QmakeManager::runQMakeContextMenu()
{
    runQMakeImpl(contextProject(), contextNode());
}

void QmakeManager::runQMakeImpl(ProjectExplorer::Project *p, ProjectExplorer::Node *node)
{
    if (!ProjectExplorerPlugin::saveModifiedFiles())
        return;
    auto *qmakeProject = qobject_cast<QmakeProject *>(p);
    QTC_ASSERT(qmakeProject, return);

    if (!qmakeProject->activeTarget() || !qmakeProject->activeTarget()->activeBuildConfiguration())
        return;

    auto *bc = static_cast<QmakeBuildConfiguration *>(qmakeProject->activeTarget()->activeBuildConfiguration());
    QMakeStep *qs = bc->qmakeStep();
    if (!qs)
        return;

    //found qmakeStep, now use it
    qs->setForced(true);

    if (node && node != qmakeProject->rootProjectNode())
        if (auto *profile = dynamic_cast<QmakeProFileNode *>(node))
            bc->setSubNodeBuild(profile);

    BuildManager::appendStep(qs, tr("QMake"));
    bc->setSubNodeBuild(nullptr);
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
        const Utils::FileName file = currentDocument->filePath();
        Node *n = ProjectTree::nodeForFile(file);
        FileNode *node  = n ? n->asFileNode() : nullptr;
        Project *project = SessionManager::projectForFile(file);

        if (project && node)
            handleSubDirContextMenu(BUILD, true, project, node->parentProjectNode(), node);
    }
}

void QmakeManager::handleSubDirContextMenu(QmakeManager::Action action, bool isFileBuild)
{
    handleSubDirContextMenu(action, isFileBuild, contextProject(),
                            buildableFileProFile(contextNode()), contextBuildableFileNode());
}

void QmakeManager::handleSubDirContextMenu(QmakeManager::Action action, bool isFileBuild,
                                           Project *contextProject, Node *contextNode,
                                           FileNode *buildableFile)
{
    QTC_ASSERT(contextProject, return);
    Target *target = contextProject->activeTarget();
    if (!target)
        return;

    auto *bc = qobject_cast<QmakeBuildConfiguration *>(target->activeBuildConfiguration());
    if (!bc)
        return;

    if (!contextNode || !buildableFile)
        isFileBuild = false;

    if (auto *prifile = dynamic_cast<QmakePriFileNode *>(contextNode)) {
        if (QmakeProFileNode *profile = prifile->proFileNode()) {
            if (profile != contextProject->rootProjectNode() || isFileBuild)
                bc->setSubNodeBuild(profile->proFileNode());
        }
    }

    if (isFileBuild)
        bc->setFileNodeBuild(buildableFile);
    if (ProjectExplorerPlugin::saveModifiedFiles()) {
        const Core::Id buildStep = ProjectExplorer::Constants::BUILDSTEPS_BUILD;
        const Core::Id cleanStep = ProjectExplorer::Constants::BUILDSTEPS_CLEAN;
        if (action == BUILD) {
            BuildManager::buildList(bc->stepList(buildStep));
        } else if (action == CLEAN) {
            BuildManager::buildList(bc->stepList(cleanStep));
        } else if (action == REBUILD) {
            QStringList names;
            names << ProjectExplorerPlugin::displayNameForStepId(cleanStep)
                  << ProjectExplorerPlugin::displayNameForStepId(buildStep);

            QList<ProjectExplorer::BuildStepList *> stepLists;
            stepLists << bc->stepList(cleanStep) << bc->stepList(buildStep);
            BuildManager::buildLists(stepLists, names);
        }
    }

    bc->setSubNodeBuild(nullptr);
    bc->setFileNodeBuild(nullptr);
}

} // namespace QmakeProjectManager

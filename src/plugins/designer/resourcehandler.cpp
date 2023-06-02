// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "resourcehandler.h"

#include "designerconstants.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>

#include <resourceeditor/resourcenode.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDesignerFormWindowInterface>

using namespace ProjectExplorer;
using namespace Utils;

namespace Designer {
namespace Internal {

// ------------ ResourceHandler
ResourceHandler::ResourceHandler(QDesignerFormWindowInterface *fw) :
    QObject(fw),
    m_form(fw)
{
}

void ResourceHandler::ensureInitialized()
{
    if (m_initialized)
        return;

    m_initialized = true;

    auto connector = [this](Project *p) {
        connect(p,
                &Project::fileListChanged,
                this,
                &ResourceHandler::updateResources,
                Qt::QueuedConnection);
    };

    for (Project *p : ProjectManager::projects())
        connector(p);

    connect(ProjectManager::instance(), &ProjectManager::projectAdded, this, connector);

    m_originalUiQrcPaths = m_form->activeResourceFilePaths();
    if (Designer::Constants::Internal::debug)
        qDebug() << "ResourceHandler::ensureInitialized() origPaths=" << m_originalUiQrcPaths;
}

ResourceHandler::~ResourceHandler() = default;

void ResourceHandler::updateResourcesHelper(bool updateProjectResources)
{
    if (m_handlingResources)
        return;

    ensureInitialized();

    const QString fileName = m_form->fileName();
    QTC_ASSERT(!fileName.isEmpty(), return);

    if (Designer::Constants::Internal::debug)
        qDebug() << "ResourceHandler::updateResources()" << fileName;

    // Filename could change in the meantime.
    Project *project = ProjectManager::projectForFile(Utils::FilePath::fromUserInput(fileName));
    const bool dirty = m_form->property("_q_resourcepathchanged").toBool();
    if (dirty)
        m_form->setDirty(true);

    // Does the file belong to a project?
    if (project && project->rootProjectNode()) {
        // Collect project resource files.

        // Find the (sub-)project the file belongs to. We don't want to find resources
        // from other parts of the project tree, e.g. via a qmake subdirs project.
        Node * const fileNode = project->rootProjectNode()->findNode([&fileName](const Node *n) {
            return n->filePath().toString() == fileName;
        });
        ProjectNode *projectNodeForUiFile = nullptr;
        if (fileNode) {
            // We do not want qbs groups or qmake .pri files here, as they contain only a subset
            // of the relevant files.
            projectNodeForUiFile = fileNode->parentProjectNode();
            while (projectNodeForUiFile && !projectNodeForUiFile->isProduct())
                projectNodeForUiFile = projectNodeForUiFile->parentProjectNode();
        }
        if (!projectNodeForUiFile)
            projectNodeForUiFile = project->rootProjectNode();

        const auto useQrcFile = [projectNodeForUiFile, project](const Node *qrcNode) {
            if (projectNodeForUiFile == project->rootProjectNode())
                return true;
            ProjectNode *projectNodeForQrcFile = qrcNode->parentProjectNode();
            while (projectNodeForQrcFile && !projectNodeForQrcFile->isProduct())
                projectNodeForQrcFile = projectNodeForQrcFile->parentProjectNode();
            return !projectNodeForQrcFile
                    || projectNodeForQrcFile == projectNodeForUiFile
                    || projectNodeForQrcFile->productType() != ProductType::App;
        };

        QStringList projectQrcFiles;
        project->rootProjectNode()->forEachNode([&](FileNode *node) {
            if (node->fileType() == FileType::Resource && useQrcFile(node))
                projectQrcFiles.append(node->filePath().toString());
        }, [&](FolderNode *node) {
            if (dynamic_cast<ResourceEditor::ResourceTopLevelNode *>(node) && useQrcFile(node))
                projectQrcFiles.append(node->filePath().toString());
        });
        // Check if the user has chosen to update the lacking resource inside designer
        if (dirty && updateProjectResources) {
            QStringList qrcPathsToBeAdded;
            for (const QString &originalQrcPath : std::as_const(m_originalUiQrcPaths)) {
                if (!projectQrcFiles.contains(originalQrcPath) && !qrcPathsToBeAdded.contains(originalQrcPath))
                    qrcPathsToBeAdded.append(originalQrcPath);
            }
            if (!qrcPathsToBeAdded.isEmpty()) {
                m_handlingResources = true;
                projectNodeForUiFile->addFiles(FileUtils::toFilePathList(qrcPathsToBeAdded));
                m_handlingResources = false;
                projectQrcFiles += qrcPathsToBeAdded;
            }
        }

        m_form->activateResourceFilePaths(projectQrcFiles);
        m_form->setResourceFileSaveMode(QDesignerFormWindowInterface::SaveOnlyUsedResourceFiles);
        if (Designer::Constants::Internal::debug)
            qDebug() << "ResourceHandler::updateResources()" << fileName
                    << " associated with project" << project->rootProjectNode()->filePath()
                    <<  " using project qrc files" << projectQrcFiles.size();
    } else {
        // Use resource file originally used in form
        m_form->activateResourceFilePaths(m_originalUiQrcPaths);
        m_form->setResourceFileSaveMode(QDesignerFormWindowInterface::SaveAllResourceFiles);
        if (Designer::Constants::Internal::debug)
            qDebug() << "ResourceHandler::updateResources()" << fileName << " not associated with project, using loaded qrc files.";
    }
}

} // namespace Internal
} // namespace Designer

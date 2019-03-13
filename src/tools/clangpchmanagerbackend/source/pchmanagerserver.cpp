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

#include "pchmanagerserver.h"

#include <pchmanagerclientinterface.h>
#include <precompiledheadersupdatedmessage.h>
#include <progressmessage.h>
#include <pchtaskgeneratorinterface.h>
#include <removegeneratedfilesmessage.h>
#include <removeprojectpartsmessage.h>
#include <updategeneratedfilesmessage.h>
#include <updateprojectpartsmessage.h>

#include <utils/smallstring.h>

#include <QApplication>

namespace ClangBackEnd {

PchManagerServer::PchManagerServer(ClangPathWatcherInterface &fileSystemWatcher,
                                   PchTaskGeneratorInterface &pchTaskGenerator,
                                   ProjectPartsManagerInterface &projectParts,
                                   GeneratedFilesInterface &generatedFiles)
    : m_fileSystemWatcher(fileSystemWatcher)
    , m_pchTaskGenerator(pchTaskGenerator)
    , m_projectPartsManager(projectParts)
    , m_generatedFiles(generatedFiles)
{
    m_fileSystemWatcher.setNotifier(this);
}

void PchManagerServer::end()
{
    QCoreApplication::exit();
}

void PchManagerServer::updateProjectParts(UpdateProjectPartsMessage &&message)
{
    m_toolChainsArgumentsCache.update(message.projectsParts, message.toolChainArguments);

    ProjectPartContainers newProjectParts = m_projectPartsManager.update(message.takeProjectsParts());

    if (m_generatedFiles.isValid()) {
        m_pchTaskGenerator.addProjectParts(std::move(newProjectParts),
                                           std::move(message.toolChainArguments));
    } else  {
        m_projectPartsManager.updateDeferred(newProjectParts);
    }
}

void PchManagerServer::removeProjectParts(RemoveProjectPartsMessage &&message)
{
    m_fileSystemWatcher.removeIds(message.projectsPartIds);

    m_projectPartsManager.remove(message.projectsPartIds);

    m_pchTaskGenerator.removeProjectParts(message.projectsPartIds);

    m_toolChainsArgumentsCache.remove(message.projectsPartIds);
}

namespace {
ProjectPartIds projectPartIds(const ProjectPartContainers &projectParts)
{
    ProjectPartIds ids;
    ids.reserve(projectParts.size());

    std::transform(projectParts.cbegin(),
                   projectParts.cend(),
                   std::back_inserter(ids),
                   [](const ProjectPartContainer &projectPart) { return projectPart.projectPartId; });

    return ids;
}
}

void PchManagerServer::updateGeneratedFiles(UpdateGeneratedFilesMessage &&message)
{
    m_generatedFiles.update(message.takeGeneratedFiles());

    if (m_generatedFiles.isValid()) {
        ProjectPartContainers deferredProjectParts = m_projectPartsManager.deferredUpdates();
        ArgumentsEntries entries = m_toolChainsArgumentsCache.arguments(
            projectPartIds(deferredProjectParts));

        for (ArgumentsEntry &entry : entries) {
            m_pchTaskGenerator.addProjectParts(std::move(deferredProjectParts),
                                               std::move(entry.arguments));
        }
    }
}

void PchManagerServer::removeGeneratedFiles(RemoveGeneratedFilesMessage &&message)
{
    m_generatedFiles.remove(message.takeGeneratedFiles());
}

void PchManagerServer::pathsWithIdsChanged(const ProjectPartIds &ids)
{
    ArgumentsEntries entries = m_toolChainsArgumentsCache.arguments(ids);

    for (ArgumentsEntry &entry : entries) {
        m_pchTaskGenerator.addProjectParts(m_projectPartsManager.projects(entry.ids),
                                           std::move(entry.arguments));
    }
}

void PchManagerServer::pathsChanged(const FilePathIds &/*filePathIds*/)
{
}

void PchManagerServer::setPchCreationProgress(int progress, int total)
{
    client()->progress({ProgressType::PrecompiledHeader, progress, total});
}

void PchManagerServer::setDependencyCreationProgress(int progress, int total)
{
    client()->progress({ProgressType::DependencyCreation, progress, total});
}

} // namespace ClangBackEnd

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

#pragma once

#include "clangpathwatcherinterface.h"
#include "clangpathwatchernotifier.h"
#include "pchcreatorinterface.h"
#include "pchmanagerserverinterface.h"
#include "projectpartsmanagerinterface.h"
#include "toolchainargumentscache.h"

#include <generatedfilesinterface.h>
#include <ipcclientprovider.h>

namespace ClangBackEnd {

class SourceRangesAndDiagnosticsForQueryMessage;
class PchTaskGeneratorInterface;
class BuildDependenciesStorageInterface;
class FilePathCachingInterface;

class PchManagerServer : public PchManagerServerInterface,
                         public ClangPathWatcherNotifier,
                         public IpcClientProvider<PchManagerClientInterface>

{
public:
    PchManagerServer(ClangPathWatcherInterface &fileSystemWatcher,
                     PchTaskGeneratorInterface &pchTaskGenerator,
                     ProjectPartsManagerInterface &projectParts,
                     GeneratedFilesInterface &generatedFiles,
                     BuildDependenciesStorageInterface &buildDependenciesStorage,
                     FilePathCachingInterface &filePathCache);

    void end() override;
    void updateProjectParts(UpdateProjectPartsMessage &&message) override;
    void removeProjectParts(RemoveProjectPartsMessage &&message) override;
    void updateGeneratedFiles(UpdateGeneratedFilesMessage &&message) override;
    void removeGeneratedFiles(RemoveGeneratedFilesMessage &&message) override;

    void pathsWithIdsChanged(const std::vector<IdPaths> &idPaths) override;
    void pathsChanged(const FilePathIds &filePathIds) override;

    void setPchCreationProgress(int progress, int total);
    void setDependencyCreationProgress(int progress, int total);

private:
    void addCompleteProjectParts(const ProjectPartIds &projectPartIds);
    void addNonSystemProjectParts(const ProjectPartIds &projectPartIds);

private:
    ClangPathWatcherInterface &m_fileSystemWatcher;
    PchTaskGeneratorInterface &m_pchTaskGenerator;
    ProjectPartsManagerInterface &m_projectPartsManager;
    GeneratedFilesInterface &m_generatedFiles;
    BuildDependenciesStorageInterface &m_buildDependenciesStorage;
    ToolChainsArgumentsCache m_toolChainsArgumentsCache;
    FilePathCachingInterface &m_filePathCache;
};

} // namespace ClangBackEnd

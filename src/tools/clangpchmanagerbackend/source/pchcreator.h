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

#include "pchcreatorinterface.h"

#include "idpaths.h"
#include "sourceentry.h"
#include "clangtool.h"

#include <filepathcaching.h>
#include <projectpartpch.h>
#include <projectpartcontainer.h>

#include <vector>
#include <random>

QT_FORWARD_DECLARE_CLASS(QFile)
QT_FORWARD_DECLARE_CLASS(QCryptographicHash)
QT_FORWARD_DECLARE_CLASS(QProcess)

namespace ClangBackEnd {

class Environment;
class GeneratedFiles;
class PchManagerClientInterface;
class ClangPathWatcherInterface;
class BuildDependenciesStorageInterface;

class PchCreator final : public PchCreatorInterface
{
public:
    PchCreator(Environment &environment,
               FilePathCaching &filePathCache,
               PchManagerClientInterface &pchManagerClient,
               ClangPathWatcherInterface &clangPathwatcher,
               BuildDependenciesStorageInterface &buildDependenciesStorage)
        : m_filePathCache(filePathCache)
        , m_environment(environment)
        , m_pchManagerClient(pchManagerClient)
        , m_clangPathwatcher(clangPathwatcher)
        , m_buildDependenciesStorage(buildDependenciesStorage)
    {}

    void generatePch(PchTask &&pchTask) override;
    const ProjectPartPch &projectPartPch() override;
    void setUnsavedFiles(const V2::FileContainers &fileContainers) override;
    void setIsUsed(bool isUsed) override;
    bool isUsed() const override;
    void clear() override;
    void doInMainThreadAfterFinished() override;

    const FilePathCachingInterface &filePathCache();

    Utils::SmallString generatePchIncludeFileContent(const FilePathIds &includeIds) const;
    bool generatePch(NativeFilePathView path, Utils::SmallStringView content);

    FilePath generatePchFilePath() const;
    static Utils::SmallStringVector generateClangCompilerArguments(const PchTask &pchTask,
                                                                   FilePathView pchPath);

    const ClangTool &clangTool() const { return m_clangTool; }

    FilePathIds existingSources(const FilePathIds &sources) const;

    const FilePathIds &watchedSystemIncludes() const { return m_watchedSystemIncludes; }
    const FilePathIds &watchedProjectIncludes() const { return m_watchedProjectIncludes; }
    const FilePathIds &watchedUserIncludes() const { return m_watchedUserIncludes; }
    const FilePathIds &watchedSources() const { return m_watchedSources; }

private:
    mutable std::mt19937_64 randomNumberGenator{std::random_device{}()};
    ClangTool m_clangTool;
    ProjectPartPch m_projectPartPch;
    CopyableFilePathCaching m_filePathCache;
    FilePathIds m_watchedSystemIncludes;
    FilePathIds m_watchedProjectIncludes;
    FilePathIds m_watchedUserIncludes;
    FilePathIds m_watchedSources;
    FilePathIds m_generatedFilePathIds;
    Environment &m_environment;
    PchManagerClientInterface &m_pchManagerClient;
    ClangPathWatcherInterface &m_clangPathwatcher;
    BuildDependenciesStorageInterface &m_buildDependenciesStorage;
    bool m_isUsed = false;
};

} // namespace ClangBackEnd

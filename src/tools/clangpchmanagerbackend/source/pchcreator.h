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

#include <filepathcaching.h>
#include <projectpartpch.h>
#include <projectpartcontainerv2.h>

#include <vector>

QT_FORWARD_DECLARE_CLASS(QFile)
QT_FORWARD_DECLARE_CLASS(QCryptographicHash)
QT_FORWARD_DECLARE_CLASS(QProcess)

namespace ClangBackEnd {

class Environment;
class GeneratedFiles;
class PchManagerClientInterface;
class ClangPathWatcherInterface;

class PchCreatorIncludes
{
public:
    FilePathIds includeIds;
    FilePathIds topIncludeIds;
    FilePathIds topSystemIncludeIds;
};

class PchCreator final : public PchCreatorInterface
{
public:
    PchCreator(Environment &environment,
               Sqlite::Database &database,
               PchManagerClientInterface &pchManagerClient,
               ClangPathWatcherInterface &fileSystemWatcher)
        : m_filePathCache(database),
          m_environment(environment),
          m_pchManagerClient(pchManagerClient),
          m_fileSystemWatcher(fileSystemWatcher)
    {
    }

    void generatePch(const V2::ProjectPartContainer &projectsPart) override;
    IdPaths takeProjectIncludes() override;
    const ProjectPartPch &projectPartPch() override;
    void setUnsavedFiles(const V2::FileContainers &fileContainers) override;
    void setIsUsed(bool isUsed) override;
    bool isUsed() const override;
    void clear() override;
    void doInMainThreadAfterFinished() override;

    const IdPaths &projectIncludes() const;
    const FilePathCaching &filePathCache();

    Utils::SmallString generatePchIncludeFileContent(const FilePathIds &includeIds) const;
    bool generatePch(Utils::SmallStringVector &&commandLineArguments);

    static QStringList convertToQStringList(const Utils::SmallStringVector &convertToQStringList);

    Utils::SmallStringVector generateProjectPartCommandLine(
            const V2::ProjectPartContainer &projectPart) const;
    Utils::SmallString generateProjectPartPchFilePathWithoutExtension(
            const V2::ProjectPartContainer &projectPart) const;
    Utils::PathStringVector generateProjectPartHeaders(
            const V2::ProjectPartContainer &projectPart) const;
    Utils::SmallString generateProjectPartSourcesContent(
            const V2::ProjectPartContainer &projectPart) const;
    ClangBackEnd::FilePaths generateProjectPartSourcePaths(
            const V2::ProjectPartContainer &projectPart) const;
    SourceEntries generateProjectPartPchIncludes(
            const V2::ProjectPartContainer &projectPart) const;
    Utils::SmallString generateProjectPathPchHeaderFilePath(
            const V2::ProjectPartContainer &projectPart) const;
    Utils::SmallString  generateProjectPartPchFilePath(
           const V2::ProjectPartContainer &projectPart) const;
    Utils::SmallString  generateProjectPartSourceFilePath(
           const V2::ProjectPartContainer &projectPart) const;
    Utils::SmallStringVector generateProjectPartPchCompilerArguments(
            const V2::ProjectPartContainer &projectPart) const;
    Utils::SmallStringVector generateProjectPartClangCompilerArguments(
             const V2::ProjectPartContainer &projectPart) const;
    IdPaths generateProjectPartPch(
            const V2::ProjectPartContainer &projectPart);
    static std::unique_ptr<QFile> generateFileWithContent(
            const Utils::SmallString &filePath,
            const Utils::SmallString &content);

    static FilePathIds topIncludeIds(const SourceEntries &includes);
    static FilePathIds allIncludeIds(const SourceEntries &includes);

private:
    static QByteArray projectPartHash(const V2::ProjectPartContainer &projectPart);

private:
    ProjectPartPch m_projectPartPch;
    IdPaths m_projectIncludeIds;
    FilePathCaching m_filePathCache;
    V2::FileContainers m_unsavedFiles;
    Environment &m_environment;
    PchManagerClientInterface &m_pchManagerClient;
    ClangPathWatcherInterface &m_fileSystemWatcher;
    bool m_isUsed = false;
};

} // namespace ClangBackEnd

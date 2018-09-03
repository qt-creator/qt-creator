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

#include "clangpchmanager_global.h"

#include <compilermacro.h>
#include <filecontainerv2.h>
#include <filepathcachinginterface.h>
#include <generatedfiles.h>

#include <projectexplorer/headerpath.h>

namespace ProjectExplorer {
class Macro;
using Macros = QVector<Macro>;
}

namespace CppTools {
class ProjectPart;
class ProjectFile;
}

namespace ClangBackEnd {
class ProjectManagementServerInterface;

namespace V2 {
class ProjectPartContainer;
}
}

QT_FORWARD_DECLARE_CLASS(QStringList)

#include <vector>

namespace ClangPchManager {

class HeaderAndSources;
class PchManagerClient;

class CLANGPCHMANAGER_EXPORT ProjectUpdater
{
public:
    ProjectUpdater(ClangBackEnd::ProjectManagementServerInterface &server,
                   ClangBackEnd::FilePathCachingInterface &filePathCache);

    void updateProjectParts(const std::vector<CppTools::ProjectPart *> &projectParts);
    void removeProjectParts(const QStringList &projectPartIds);

    void updateGeneratedFiles(ClangBackEnd::V2::FileContainers &&generatedFiles);
    void removeGeneratedFiles(ClangBackEnd::FilePaths &&filePaths);

unittest_public:
    void setExcludedPaths(ClangBackEnd::FilePaths &&excludedPaths);
    const ClangBackEnd::FilePaths &excludedPaths() const;

    const ClangBackEnd::GeneratedFiles &generatedFiles() const;

    HeaderAndSources headerAndSourcesFromProjectPart(CppTools::ProjectPart *projectPart) const;
    ClangBackEnd::V2::ProjectPartContainer toProjectPartContainer(
            CppTools::ProjectPart *projectPart) const;
    std::vector<ClangBackEnd::V2::ProjectPartContainer> toProjectPartContainers(
            std::vector<CppTools::ProjectPart *> projectParts) const;
    void addToHeaderAndSources(HeaderAndSources &headerAndSources,
                               const CppTools::ProjectFile &projectFile) const;
    static QStringList compilerArguments(CppTools::ProjectPart *projectPart);
    static ClangBackEnd::CompilerMacros createCompilerMacros(
            const ProjectExplorer::Macros &projectMacros);
    static Utils::SmallStringVector createIncludeSearchPaths(
            const ProjectExplorer::HeaderPaths &projectPartHeaderPaths);
    static ClangBackEnd::FilePaths createExcludedPaths(
            const ClangBackEnd::V2::FileContainers &generatedFiles);

private:
    ClangBackEnd::GeneratedFiles m_generatedFiles;
    ClangBackEnd::FilePaths m_excludedPaths;
    ClangBackEnd::ProjectManagementServerInterface &m_server;
    ClangBackEnd::FilePathCachingInterface &m_filePathCache;
};

} // namespace ClangPchManager

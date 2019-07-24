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

#include "clangsupport_global.h"

#include "compilermacro.h"
#include "filepathid.h"
#include "includesearchpath.h"
#include "projectpartartefact.h"
#include "projectpartid.h"
#include "sourceentry.h"

#include <utils/cpplanguage_details.h>
#include <utils/smallstringio.h>

namespace ClangBackEnd {

class ProjectPartContainer : public ProjectPartArtefact
{
    using uchar = unsigned char;
public:
    ProjectPartContainer() = default;
    ProjectPartContainer(ProjectPartId projectPartId,
                         Utils::SmallStringVector &&toolChainArguments,
                         CompilerMacros &&compilerMacros,
                         IncludeSearchPaths &&systemIncludeSearchPaths,
                         IncludeSearchPaths &&projectIncludeSearchPaths,
                         FilePathIds &&headerPathIds,
                         FilePathIds &&sourcePathIds,
                         Utils::Language language,
                         Utils::LanguageVersion languageVersion,
                         Utils::LanguageExtension languageExtension)
        : ProjectPartArtefact(projectPartId,
                              std::move(toolChainArguments),
                              std::move(compilerMacros),
                              std::move(systemIncludeSearchPaths),
                              std::move(projectIncludeSearchPaths),
                              language,
                              languageVersion,
                              languageExtension)
        , headerPathIds(std::move(headerPathIds))
        , sourcePathIds(std::move(sourcePathIds))

    {}

    ProjectPartContainer(Utils::SmallStringView compilerArgumentsText,
                         Utils::SmallStringView compilerMacrosText,
                         Utils::SmallStringView systemIncludeSearchPathsText,
                         Utils::SmallStringView projectIncludeSearchPathsText,
                         int projectPartId,
                         int language,
                         int languageVersion,
                         int languageExtension)
        : ProjectPartArtefact(compilerArgumentsText,
                              compilerMacrosText,
                              systemIncludeSearchPathsText,
                              projectIncludeSearchPathsText,
                              projectPartId,
                              language,
                              languageVersion,
                              languageExtension)
    {}

    friend QDataStream &operator<<(QDataStream &out, const ProjectPartContainer &container)
    {
        out << container.projectPartId;
        out << container.toolChainArguments;
        out << container.compilerMacros;
        out << container.systemIncludeSearchPaths;
        out << container.projectIncludeSearchPaths;
        out << container.headerPathIds;
        out << container.sourcePathIds;
        out << uchar(container.language);
        out << uchar(container.languageVersion);
        out << uchar(container.languageExtension);
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ProjectPartContainer &container)
    {
        uchar language;
        uchar languageVersion;
        uchar languageExtension;

        in >> container.projectPartId;
        in >> container.toolChainArguments;
        in >> container.compilerMacros;
        in >> container.systemIncludeSearchPaths;
        in >> container.projectIncludeSearchPaths;
        in >> container.headerPathIds;
        in >> container.sourcePathIds;
        in >> language;
        in >> languageVersion;
        in >> languageExtension;

        container.language = static_cast<Utils::Language>(language);
        container.languageVersion = static_cast<Utils::LanguageVersion>(languageVersion);
        container.languageExtension = static_cast<Utils::LanguageExtension>(languageExtension);

        return in;
    }

    friend bool operator==(const ProjectPartContainer &first, const ProjectPartContainer &second)
    {
        return first.projectPartId == second.projectPartId
               && first.toolChainArguments == second.toolChainArguments
               && first.compilerMacros == second.compilerMacros
               && first.systemIncludeSearchPaths == second.systemIncludeSearchPaths
               && first.projectIncludeSearchPaths == second.projectIncludeSearchPaths
               && first.headerPathIds == second.headerPathIds
               && first.sourcePathIds == second.sourcePathIds && first.language == second.language
               && first.languageVersion == second.languageVersion
               && first.languageExtension == second.languageExtension;
    }

    friend bool operator<(const ProjectPartContainer &first, const ProjectPartContainer &second)
    {
        return std::tie(first.projectPartId,
                        first.toolChainArguments,
                        first.compilerMacros,
                        first.systemIncludeSearchPaths,
                        first.projectIncludeSearchPaths,
                        first.headerPathIds,
                        first.sourcePathIds,
                        first.language,
                        first.languageVersion,
                        first.languageExtension,
                        first.preCompiledHeaderWasGenerated)
               < std::tie(second.projectPartId,
                          second.toolChainArguments,
                          second.compilerMacros,
                          second.systemIncludeSearchPaths,
                          second.projectIncludeSearchPaths,
                          second.headerPathIds,
                          second.sourcePathIds,
                          second.language,
                          second.languageVersion,
                          second.languageExtension,
                          second.preCompiledHeaderWasGenerated);
    }

    ProjectPartContainer clone() const
    {
        return *this;
    }

public:
    FilePathIds headerPathIds;
    FilePathIds sourcePathIds;
    bool updateIsDeferred = false;
    bool preCompiledHeaderWasGenerated = true;
};

using ProjectPartContainerReference = std::reference_wrapper<ProjectPartContainer>;
using ProjectPartContainers = std::vector<ProjectPartContainer>;
using ProjectPartContainerReferences = std::vector<ProjectPartContainerReference>;

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const ProjectPartContainer &container);

} // namespace ClangBackEnd

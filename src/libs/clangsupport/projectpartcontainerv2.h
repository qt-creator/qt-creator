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

#include <utils/smallstringio.h>

namespace ClangBackEnd {
namespace V2 {

class ProjectPartContainer
{
public:
    ProjectPartContainer() = default;
    ProjectPartContainer(Utils::SmallString &&projectPartId,
                         Utils::SmallStringVector &&arguments,
                         CompilerMacros &&compilerMacros,
                         Utils::SmallStringVector &&includeSearchPaths,
                         FilePathIds &&headerPathIds,
                         FilePathIds &&sourcePathIds)
        : projectPartId(std::move(projectPartId)),
          arguments(std::move(arguments)),
          compilerMacros(std::move(compilerMacros)),
          includeSearchPaths(std::move(includeSearchPaths)),
          headerPathIds(std::move(headerPathIds)),
          sourcePathIds(std::move(sourcePathIds))
    {
    }

    friend QDataStream &operator<<(QDataStream &out, const ProjectPartContainer &container)
    {
        out << container.projectPartId;
        out << container.arguments;
        out << container.compilerMacros;
        out << container.includeSearchPaths;
        out << container.headerPathIds;
        out << container.sourcePathIds;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ProjectPartContainer &container)
    {
        in >> container.projectPartId;
        in >> container.arguments;
        in >> container.compilerMacros;
        in >> container.includeSearchPaths;
        in >> container.headerPathIds;
        in >> container.sourcePathIds;

        return in;
    }

    friend bool operator==(const ProjectPartContainer &first, const ProjectPartContainer &second)
    {
        return first.projectPartId == second.projectPartId
            && first.arguments == second.arguments
            && first.compilerMacros == second.compilerMacros
            && first.includeSearchPaths == second.includeSearchPaths
            && first.headerPathIds == second.headerPathIds
            && first.sourcePathIds == second.sourcePathIds;
    }

    friend bool operator<(const ProjectPartContainer &first, const ProjectPartContainer &second)
    {
        return std::tie(first.projectPartId,
                        first.arguments,
                        first.compilerMacros,
                        first.includeSearchPaths,
                        first.headerPathIds,
                        first.sourcePathIds)
             < std::tie(second.projectPartId,
                        second.arguments,
                        second.compilerMacros,
                        first.includeSearchPaths,
                        second.headerPathIds,
                        second.sourcePathIds);
    }

    ProjectPartContainer clone() const
    {
        return *this;
    }

public:
    Utils::SmallString projectPartId;
    Utils::SmallStringVector arguments;
    CompilerMacros compilerMacros;
    Utils::SmallStringVector includeSearchPaths;
    FilePathIds headerPathIds;
    FilePathIds sourcePathIds;
};

using ProjectPartContainers = std::vector<ProjectPartContainer>;

QDebug operator<<(QDebug debug, const ProjectPartContainer &container);
} // namespace V2
} // namespace ClangBackEnd

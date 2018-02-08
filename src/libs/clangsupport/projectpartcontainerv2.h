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
        : m_projectPartId(std::move(projectPartId)),
          m_arguments(std::move(arguments)),
          m_compilerMacros(std::move(compilerMacros)),
          m_includeSearchPaths(std::move(includeSearchPaths)),
          m_headerPathIds(std::move(headerPathIds)),
          m_sourcePathIds(std::move(sourcePathIds))
    {
    }

    const Utils::SmallString &projectPartId() const
    {
        return m_projectPartId;
    }

    const Utils::SmallStringVector &arguments() const
    {
        return m_arguments;
    }

    Utils::SmallStringVector takeArguments()
    {
        return std::move(m_arguments);
    }

    const CompilerMacros &compilerMacros() const
    {
        return m_compilerMacros;
    }

    const Utils::SmallStringVector &includeSearchPaths() const
    {
        return m_includeSearchPaths;
    }

    const FilePathIds &sourcePathIds() const
    {
        return m_sourcePathIds;
    }

    const FilePathIds &headerPathIds() const
    {
        return m_headerPathIds;
    }

    friend QDataStream &operator<<(QDataStream &out, const ProjectPartContainer &container)
    {
        out << container.m_projectPartId;
        out << container.m_arguments;
        out << container.m_compilerMacros;
        out << container.m_includeSearchPaths;
        out << container.m_headerPathIds;
        out << container.m_sourcePathIds;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ProjectPartContainer &container)
    {
        in >> container.m_projectPartId;
        in >> container.m_arguments;
        in >> container.m_compilerMacros;
        in >> container.m_includeSearchPaths;
        in >> container.m_headerPathIds;
        in >> container.m_sourcePathIds;

        return in;
    }

    friend bool operator==(const ProjectPartContainer &first, const ProjectPartContainer &second)
    {
        return first.m_projectPartId == second.m_projectPartId
            && first.m_arguments == second.m_arguments
            && first.m_compilerMacros == second.m_compilerMacros
            && first.m_includeSearchPaths == second.m_includeSearchPaths
            && first.m_headerPathIds == second.m_headerPathIds
            && first.m_sourcePathIds == second.m_sourcePathIds;
    }

    friend bool operator<(const ProjectPartContainer &first, const ProjectPartContainer &second)
    {
        return std::tie(first.m_projectPartId,
                        first.m_arguments,
                        first.m_compilerMacros,
                        first.m_includeSearchPaths,
                        first.m_headerPathIds,
                        first.m_sourcePathIds)
             < std::tie(second.m_projectPartId,
                        second.m_arguments,
                        second.m_compilerMacros,
                        first.m_includeSearchPaths,
                        second.m_headerPathIds,
                        second.m_sourcePathIds);
    }

    ProjectPartContainer clone() const
    {
        return *this;
    }

private:
    Utils::SmallString m_projectPartId;
    Utils::SmallStringVector m_arguments;
    CompilerMacros m_compilerMacros;
    Utils::SmallStringVector m_includeSearchPaths;
    FilePathIds m_headerPathIds;
    FilePathIds m_sourcePathIds;
};

using ProjectPartContainers = std::vector<ProjectPartContainer>;

QDebug operator<<(QDebug debug, const ProjectPartContainer &container);
} // namespace V2
} // namespace ClangBackEnd

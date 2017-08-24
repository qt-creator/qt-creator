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

#include <utils/smallstringio.h>

namespace ClangBackEnd {
namespace V2 {

class ProjectPartContainer
{
public:
    ProjectPartContainer() = default;
    ProjectPartContainer(Utils::SmallString &&projectPartId,
                         Utils::SmallStringVector &&arguments,
                         Utils::PathStringVector &&headerPaths,
                         Utils::PathStringVector &&sourcePaths)
        : m_projectPartId(std::move(projectPartId)),
          m_arguments(std::move(arguments)),
          m_headerPaths(std::move(headerPaths)),
          m_sourcePaths(std::move(sourcePaths))
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

    const Utils::PathStringVector &sourcePaths() const
    {
        return m_sourcePaths;
    }

    const Utils::PathStringVector &headerPaths() const
    {
        return m_headerPaths;
    }

    friend QDataStream &operator<<(QDataStream &out, const ProjectPartContainer &container)
    {
        out << container.m_projectPartId;
        out << container.m_arguments;
        out << container.m_headerPaths;
        out << container.m_sourcePaths;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ProjectPartContainer &container)
    {
        in >> container.m_projectPartId;
        in >> container.m_arguments;
        in >> container.m_headerPaths;
        in >> container.m_sourcePaths;

        return in;
    }

    friend bool operator==(const ProjectPartContainer &first, const ProjectPartContainer &second)
    {
        return first.m_projectPartId == second.m_projectPartId
            && first.m_arguments == second.m_arguments
            && first.m_headerPaths == second.m_headerPaths
            && first.m_sourcePaths == second.m_sourcePaths;
    }

    friend bool operator<(const ProjectPartContainer &first, const ProjectPartContainer &second)
    {
        return std::tie(first.m_projectPartId, first.m_arguments, first.m_headerPaths, first.m_sourcePaths)
             < std::tie(second.m_projectPartId, second.m_arguments, second.m_headerPaths, second.m_sourcePaths);
    }

    ProjectPartContainer clone() const
    {
        return ProjectPartContainer(m_projectPartId.clone(),
                                    m_arguments.clone(),
                                    m_headerPaths.clone(),
                                    m_sourcePaths.clone());
    }

private:
    Utils::SmallString m_projectPartId;
    Utils::SmallStringVector m_arguments;
    Utils::PathStringVector m_headerPaths;
    Utils::PathStringVector m_sourcePaths;
};

using ProjectPartContainers = std::vector<ProjectPartContainer>;

QDebug operator<<(QDebug debug, const ProjectPartContainer &container);
std::ostream &operator<<(std::ostream &out, const ProjectPartContainer &container);
} // namespace V2
} // namespace ClangBackEnd

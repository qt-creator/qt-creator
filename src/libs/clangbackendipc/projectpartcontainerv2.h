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

#include "clangbackendipc_global.h"

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
        : projectPartId_(std::move(projectPartId)),
          arguments_(std::move(arguments)),
          headerPaths_(std::move(headerPaths)),
          sourcePaths_(std::move(sourcePaths))
    {
    }

    const Utils::SmallString &projectPartId() const
    {
        return projectPartId_;
    }

    const Utils::SmallStringVector &arguments() const
    {
        return arguments_;
    }

    const Utils::PathStringVector &sourcePaths() const
    {
        return sourcePaths_;
    }

    const Utils::PathStringVector &headerPaths() const
    {
        return headerPaths_;
    }

    friend QDataStream &operator<<(QDataStream &out, const ProjectPartContainer &container)
    {
        out << container.projectPartId_;
        out << container.arguments_;
        out << container.headerPaths_;
        out << container.sourcePaths_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ProjectPartContainer &container)
    {
        in >> container.projectPartId_;
        in >> container.arguments_;
        in >> container.headerPaths_;
        in >> container.sourcePaths_;

        return in;
    }

    friend bool operator==(const ProjectPartContainer &first, const ProjectPartContainer &second)
    {
        return first.projectPartId_ == second.projectPartId_
            && first.arguments_ == second.arguments_
            && first.headerPaths_ == second.headerPaths_
            && first.sourcePaths_ == second.sourcePaths_;
    }

    friend bool operator<(const ProjectPartContainer &first, const ProjectPartContainer &second)
    {
        return std::tie(first.projectPartId_, first.arguments_, first.headerPaths_, first.sourcePaths_)
             < std::tie(second.projectPartId_, second.arguments_, second.headerPaths_, second.sourcePaths_);
    }

    ProjectPartContainer clone() const
    {
        return ProjectPartContainer(projectPartId_.clone(),
                                    arguments_.clone(),
                                    headerPaths_.clone(),
                                    sourcePaths_.clone());
    }

private:
    Utils::SmallString projectPartId_;
    Utils::SmallStringVector arguments_;
    Utils::PathStringVector headerPaths_;
    Utils::PathStringVector sourcePaths_;
};

using ProjectPartContainers = std::vector<ProjectPartContainer>;

QDebug operator<<(QDebug debug, const ProjectPartContainer &container);
std::ostream &operator<<(std::ostream &out, const ProjectPartContainer &container);
} // namespace V2
} // namespace ClangBackEnd

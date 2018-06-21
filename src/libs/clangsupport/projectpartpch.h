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

class ProjectPartPch
{
public:
    ProjectPartPch() = default;
    ProjectPartPch(Utils::SmallString &&projectPartId,
                   Utils::SmallString &&pchPath,
                   long long lastModified)
        : projectPartId(std::move(projectPartId)),
          pchPath(std::move(pchPath)),
          lastModified(lastModified)
    {}
    ProjectPartPch(Utils::SmallStringView pchPath,
                   long long lastModified)
        : pchPath(pchPath),
          lastModified(lastModified)
    {}

    friend QDataStream &operator<<(QDataStream &out, const ProjectPartPch &container)
    {
        out << container.projectPartId;
        out << container.pchPath;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ProjectPartPch &container)
    {
        in >> container.projectPartId;
        in >> container.pchPath;

        return in;
    }

    friend bool operator==(const ProjectPartPch &first,
                           const ProjectPartPch &second)
    {
        return first.projectPartId == second.projectPartId
            && first.pchPath == second.pchPath;
    }

    ProjectPartPch clone() const
    {
        return *this;
    }

public:
    Utils::SmallString projectPartId;
    Utils::SmallString pchPath;
    long long lastModified = -1;
};

using ProjectPartPchs = std::vector<ProjectPartPch>;

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const ProjectPartPch &projectPartPch);
} // namespace ClangBackEnd

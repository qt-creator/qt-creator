/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QDataStream>

namespace ClangBackEnd {

class ProjectPartId
{
public:
    constexpr ProjectPartId() = default;

    ProjectPartId(const char *) = delete;

    ProjectPartId(int projectPathId)
        : projectPathId(projectPathId)
    {}

    bool isValid() const { return projectPathId >= 0; }

    friend bool operator==(ProjectPartId first, ProjectPartId second)
    {
        return first.isValid() && first.projectPathId == second.projectPathId;
    }

    friend bool operator!=(ProjectPartId first, ProjectPartId second) { return !(first == second); }

    friend bool operator<(ProjectPartId first, ProjectPartId second)
    {
        return first.projectPathId < second.projectPathId;
    }

    friend QDataStream &operator<<(QDataStream &out, const ProjectPartId &projectPathId)
    {
        out << projectPathId.projectPathId;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ProjectPartId &projectPathId)
    {
        in >> projectPathId.projectPathId;

        return in;
    }

public:
    int projectPathId = -1;
};

using ProjectPartIds = std::vector<ProjectPartId>;

} // namespace ClangBackEnd

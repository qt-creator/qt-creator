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

#include "builddependency.h"

#include <compilermacro.h>

#include <utils/smallstringvector.h>

namespace ClangBackEnd {

class PchTask
{
public:
    PchTask(Utils::SmallString &&projectPartId,
            FilePathIds &&includes,
            CompilerMacros &&compilerMacros,
            UsedMacros &&usedMacros)
        : projectPartId(projectPartId)
        , includes(includes)
        , compilerMacros(compilerMacros)
        , usedMacros(usedMacros)
    {}

    friend bool operator==(const PchTask &first, const PchTask &second)
    {
        return first.projectPartId == second.projectPartId
               && first.dependentIds == second.dependentIds && first.includes == second.includes
               && first.compilerMacros == second.compilerMacros
               && first.usedMacros == second.usedMacros;
    }

public:
    Utils::SmallString projectPartId;
    Utils::SmallStringVector dependentIds;
    FilePathIds includes;
    CompilerMacros compilerMacros;
    UsedMacros usedMacros;
};

class PchTaskSet
{
public:
    PchTaskSet(PchTask &&system, PchTask &&project)
        : system(std::move(system))
        , project(std::move(project))
    {}

public:
    PchTask system;
    PchTask project;
};

using PchTasks = std::vector<PchTask>;
using PchTaskSets = std::vector<PchTaskSet>;
} // namespace ClangBackEnd

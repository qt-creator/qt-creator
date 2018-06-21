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

#include "projectpart.h"

namespace CppTools {

enum class Language { C, Cxx };

class ProjectPartInfo {
public:
    enum Hint {
        NoHint = 0,
        IsFallbackMatch  = 1 << 0,
        IsAmbiguousMatch = 1 << 1,
        IsPreferredMatch = 1 << 2,
        IsFromProjectMatch = 1 << 3,
        IsFromDependenciesMatch = 1 << 4,
    };
    Q_DECLARE_FLAGS(Hints, Hint)

    ProjectPartInfo() = default;
    ProjectPartInfo(const ProjectPart::Ptr &projectPart,
                    const QList<ProjectPart::Ptr> &projectParts,
                    Hints hints)
        : projectPart(projectPart)
        , projectParts(projectParts)
        , hints(hints)
    {
    }

public:
    ProjectPart::Ptr projectPart;
    QList<ProjectPart::Ptr> projectParts; // The one above as first plus alternatives.
    Hints hints = NoHint;
};

} // namespace CppTools

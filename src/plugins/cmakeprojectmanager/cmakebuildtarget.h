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

#include "cmake_global.h"

#include <projectexplorer/projectmacro.h>

#include <utils/fileutils.h>

#include <QStringList>

namespace CMakeProjectManager {

enum TargetType {
    ExecutableType = 0,
    StaticLibraryType = 2,
    DynamicLibraryType = 3,
    UtilityType = 64
};

class CMAKE_EXPORT CMakeBuildTarget
{
public:
    QString title;
    Utils::FileName executable; // TODO: rename to output?
    TargetType targetType = UtilityType;
    Utils::FileName workingDirectory;
    Utils::FileName sourceDirectory;
    Utils::FileName makeCommand;

    // code model
    QList<Utils::FileName> includeFiles;
    QStringList compilerOptions;
    ProjectExplorer::Macros macros;
    QList<Utils::FileName> files;

    void clear();
};

} // namespace CMakeProjectManager

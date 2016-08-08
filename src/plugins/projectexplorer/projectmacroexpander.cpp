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

#include "projectmacroexpander.h"
#include "kit.h"
#include "projectexplorerconstants.h"

namespace ProjectExplorer {

ProjectMacroExpander::ProjectMacroExpander(const QString &mainFilePath, const QString &projectName,
                                           const Kit *kit, const QString &bcName,
                                           BuildConfiguration::BuildType buildType)
{
    registerFileVariables(Constants::VAR_CURRENTPROJECT_PREFIX,
                     QCoreApplication::translate("ProjectExplorer", "Main file of current project"),
                     [mainFilePath]() -> QString { return mainFilePath; });

    registerVariable(Constants::VAR_CURRENTPROJECT_NAME,
                     QCoreApplication::translate("ProjectExplorer", "Name of current project"),
                     [projectName] { return projectName; });

    registerVariable(Constants::VAR_CURRENTBUILD_NAME,
                     QCoreApplication::translate("ProjectExplorer", "Name of current build"),
                     [bcName] { return bcName; });

    registerVariable(Constants::VAR_CURRENTBUILD_TYPE,
                     QCoreApplication::translate("ProjectExplorer", "Type of current build"),
                     [buildType] { return BuildConfiguration::buildTypeName(buildType); });

    registerSubProvider([kit] { return kit->macroExpander(); });
}

} // namespace ProjectExplorer

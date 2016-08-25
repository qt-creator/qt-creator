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

#include "cmakebuildconfiguration.h"
#include "cmakeconfigitem.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

namespace CMakeProjectManager {

class CMakeBuildInfo : public ProjectExplorer::BuildInfo
{
public:
    CMakeBuildInfo(const ProjectExplorer::IBuildConfigurationFactory *f) : ProjectExplorer::BuildInfo(f)
    { }

    CMakeBuildInfo(const Internal::CMakeBuildConfiguration *bc) :
        ProjectExplorer::BuildInfo(ProjectExplorer::IBuildConfigurationFactory::find(bc->target()))
    {
        displayName = bc->displayName();
        buildDirectory = bc->buildDirectory();
        kitId = bc->target()->kit()->id();

        QTC_ASSERT(bc->target()->project(), return);
        sourceDirectory = bc->target()->project()->projectDirectory().toString();
        configuration = bc->cmakeConfiguration();
    }

    bool operator==(const BuildInfo &o) const final
    {
        if (!ProjectExplorer::BuildInfo::operator==(o))
            return false;

        auto other = static_cast<const CMakeBuildInfo *>(&o);
        return sourceDirectory == other->sourceDirectory
                && configuration == other->configuration;
    }

    QString sourceDirectory;
    CMakeConfig configuration;
};

} // namespace CMakeProjectManager

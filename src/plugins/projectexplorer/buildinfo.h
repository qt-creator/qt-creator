// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "buildconfiguration.h"

#include <utils/fileutils.h>
#include <utils/id.h>

namespace ProjectExplorer {

class BuildConfigurationFactory;

class PROJECTEXPLORER_EXPORT BuildInfo final
{
public:
    BuildInfo() = default;

    QString displayName;
    QString typeName;
    Utils::FilePath buildDirectory;
    Utils::Id kitId;
    BuildConfiguration::BuildType buildType = BuildConfiguration::Unknown;

    QVariant extraInfo;
    const BuildConfigurationFactory *factory = nullptr;

    bool operator==(const BuildInfo &o) const
    {
        return factory == o.factory
                && displayName == o.displayName && typeName == o.typeName
                && buildDirectory == o.buildDirectory && kitId == o.kitId
                && buildType == o.buildType;
    }
};

} // namespace ProjectExplorer

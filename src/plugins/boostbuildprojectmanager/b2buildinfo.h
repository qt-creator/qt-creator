//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#ifndef BBBUILDINFO_HPP
#define BBBUILDINFO_HPP

#include "b2buildconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <utils/fileutils.h>

namespace BoostBuildProjectManager {
namespace Internal {

class BuildConfigurationFactory;

class BuildInfo : public ProjectExplorer::BuildInfo
{
public:
    explicit BuildInfo(BuildConfigurationFactory const* f);

    // Boost.Build option variant={debug|release}
    // By default, the debug variant is set.
    BuildConfiguration::BuildType buildType;

    // Boost.Build command working directory.
    // By default, empty what indicates project path from where Jamfile was opened.
    Utils::FileName workingDirectory;
};

} // namespace Internal
} // namespace BoostBuildProjectManager

#endif // BBBUILDINFO_HPP

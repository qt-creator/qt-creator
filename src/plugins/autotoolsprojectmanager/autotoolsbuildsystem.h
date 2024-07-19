// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildsystem.h>

namespace AutotoolsProjectManager::Internal {

ProjectExplorer::BuildSystem *createAutotoolsBuildSystem(ProjectExplorer::Target *target);

} // AutotoolsProjectManager::Internal

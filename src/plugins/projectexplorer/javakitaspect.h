// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/filepath.h>

#include <functional>

namespace Utils { class Id; }

namespace ProjectExplorer {
class Kit;

class PROJECTEXPLORER_EXPORT JavaKitAspect
{
public:
    static Utils::Id id();
    static Utils::FilePath javaHome(const Kit *k);
    static void setJavaHome(Kit *k, const Utils::FilePath &javaHome);
};

// Plugins that know of a Java development kit of their own, such as the one
// Android is configured with, offer it here for the automatic setup of a kit.
// A provider is asked about the kit being set up, so that it can keep quiet
// about the kits that are none of its business.
using JavaHomeCandidateProvider = std::function<Utils::FilePaths(const Kit *)>;
PROJECTEXPLORER_EXPORT QList<JavaHomeCandidateProvider> &javaHomeCandidateProviders();

} // namespace ProjectExplorer

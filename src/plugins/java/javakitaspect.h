// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "java_global.h"

#include <utils/filepath.h>

#include <functional>
#include <optional>

namespace ProjectExplorer { class Kit; }
namespace Utils { class Id; }

namespace Java {

class JAVA_EXPORT JavaKitAspect
{
public:
    static Utils::Id id();
    static Utils::FilePath javaHome(const ProjectExplorer::Kit *k);
    // Enters that directory into the list of the JDKs of the machine if it is
    // not there yet, and names it from the kit. A directory that holds no JDK
    // leaves the kit without one, to be set up again once there is one.
    static void setJavaHome(ProjectExplorer::Kit *k, const Utils::FilePath &javaHome);
};

// Plugins that know of a Java development kit of their own, such as the one
// Android is configured with, offer it here for the automatic setup of a kit.
// A provider is asked about the kit being set up, so that it can keep quiet
// about the kits that are none of its business.
//
// std::nullopt is that silence, and such a kit is given the default JDK of
// the machine. A provider that answers speaks for the kit: an answer holding
// no candidate leaves that kit without a JDK rather than with the default,
// which is what a plugin whose kits take theirs from their toolchain asks
// for.
using JavaHomeCandidateProvider = std::function<std::optional<Utils::FilePaths>(const ProjectExplorer::Kit *)>;
JAVA_EXPORT QList<JavaHomeCandidateProvider> &javaHomeCandidateProviders();

} // namespace Java

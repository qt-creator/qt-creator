// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "java_global.h"

#include <utils/filepath.h>

#include <functional>
#include <optional>

namespace ProjectExplorer { class BuildConfiguration; }

namespace Java {

// Where the Java code of a project lives and what it is compiled against.
// A language server needs both: it resolves a class through the directories
// the sources are in and the archives of the class path.
class JavaClassPath
{
public:
    Utils::FilePaths sourceDirectories;
    Utils::FilePaths libraries;

    bool isEmpty() const { return sourceDirectories.isEmpty() && libraries.isEmpty(); }
};

// Plugins that know how a build configuration lays its Java code out say so
// here, and what they say is added to what the build system said. A provider
// asked about a build configuration that is none of its business answers with
// nothing, so that a project none of them knows and no build system speaks
// for is left to the language server, which reads a Gradle or a Maven project
// by itself.
using JavaClassPathProvider
    = std::function<std::optional<JavaClassPath>(ProjectExplorer::BuildConfiguration *)>;
JAVA_EXPORT QList<JavaClassPathProvider> &javaClassPathProviders();

JAVA_EXPORT std::optional<JavaClassPath> javaClassPath(ProjectExplorer::BuildConfiguration *bc);

// What the two have to say about one project taken together, each of them
// nothing where it has nothing to say. One directory or library named twice
// is named once.
JAVA_EXPORT std::optional<JavaClassPath> mergedClassPath(
    const std::optional<JavaClassPath> &first, const std::optional<JavaClassPath> &second);

// The directories the package hierarchies of those files start at.
Utils::FilePaths javaSourceRoots(const Utils::FilePaths &files);

// The package a compilation unit declares itself to be in, empty for one that
// declares none.
QString javaPackageOf(const QString &contents);

} // namespace Java

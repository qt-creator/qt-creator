// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "fileapidataextractor.h"

#include <utils/filepath.h>

#include <QFuture>
#include <QList>
#include <QSet>

namespace CMakeProjectManager::Internal {

// A source file a CMake file names inside a branch that the configure run did
// not take, and that the build therefore does not know about.
class ConditionalSource
{
public:
    Utils::FilePath path;
    Utils::FilePath directory; // The directory of the CMake file naming it.
    QString target;            // Empty when no command names a target for it.
};

// The conditional sources of a single parsed CMakeLists.txt, in source order
// and without subtracting what the build already knows. A relative path is
// held against the directory of cmakeFile, which is what CMake does for a
// CMakeLists.txt and not for a file it includes.
QList<ConditionalSource> conditionalSourcesOf(const CMakeLang::DocumentPtr &document,
                                              const Utils::FilePath &cmakeFile,
                                              const Utils::FilePath &sourceDir,
                                              const Utils::FilePath &buildDir);

// The conditional sources of a whole project. A file the build knows about is
// in knownFiles, so a branch that the configure run did take drops out here
// and no condition has to be evaluated anywhere.
QList<ConditionalSource> conditionalSources(const QFuture<void> &cancelFuture,
                                            const QSet<CMakeFileInfo> &cmakeFiles,
                                            const QSet<Utils::FilePath> &knownFiles,
                                            const Utils::FilePath &sourceDir,
                                            const Utils::FilePath &buildDir);

#ifdef WITH_TESTS
QObject *createConditionalSourcesTest();
#endif

} // CMakeProjectManager::Internal

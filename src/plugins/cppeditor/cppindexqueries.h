// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <utils/filepath.h>
#include <utils/link.h>

#include <QSet>

QT_BEGIN_NAMESPACE
class QObject;
QT_END_NAMESPACE

namespace CPlusPlus { class Snapshot; }

namespace CppEditor {

// The location of every #include directive that resolved to filePath.
CPPEDITOR_EXPORT Utils::Links includersOf(const CPlusPlus::Snapshot &snapshot,
                                          const Utils::FilePath &filePath);

// The files that the #include directives of filePath resolved to, in order.
CPPEDITOR_EXPORT Utils::FilePaths includedFilesOf(const CPlusPlus::Snapshot &snapshot,
                                                  const Utils::FilePath &filePath);

// The files reachable from filePath via #include directives.
// Contains filePath itself only if it is part of an include cycle.
CPPEDITOR_EXPORT QSet<Utils::FilePath> includeClosureOf(const CPlusPlus::Snapshot &snapshot,
                                                        const Utils::FilePath &filePath);

#ifdef WITH_TESTS
namespace Internal { QObject *createIndexQueriesTest(); }
#endif

} // namespace CppEditor

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

// The files from which filePath is reachable via #include directives.
// Contains filePath itself only if it is part of an include cycle.
CPPEDITOR_EXPORT Utils::FilePaths filesDependingOn(const CPlusPlus::Snapshot &snapshot,
                                                   const Utils::FilePath &filePath);

// Matches by file name, resolved or not, for headers like ui_<form>.h whose
// location is unknown.
CPPEDITOR_EXPORT Utils::FilePaths includersOfFileNamed(const CPlusPlus::Snapshot &snapshot,
                                                       const QString &fileName);

// The files with an #include directive that may resolve differently now that newFiles
// exist, plus all files that include them. Includes that already resolve to one of
// newFiles do not count.
CPPEDITOR_EXPORT QSet<Utils::FilePath> filesAffectedByNewFiles(
    const CPlusPlus::Snapshot &snapshot, const QSet<Utils::FilePath> &newFiles);

#ifdef WITH_TESTS
namespace Internal { QObject *createIndexQueriesTest(); }
#endif

} // namespace CppEditor

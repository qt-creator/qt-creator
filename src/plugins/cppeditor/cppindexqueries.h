// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <utils/link.h>

QT_BEGIN_NAMESPACE
class QObject;
QT_END_NAMESPACE

namespace CPlusPlus { class Snapshot; }
namespace Utils { class FilePath; }

namespace CppEditor {

// The location of every #include directive that resolved to filePath.
CPPEDITOR_EXPORT Utils::Links includersOf(const CPlusPlus::Snapshot &snapshot,
                                          const Utils::FilePath &filePath);

#ifdef WITH_TESTS
namespace Internal { QObject *createIndexQueriesTest(); }
#endif

} // namespace CppEditor

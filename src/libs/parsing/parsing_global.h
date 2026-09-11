// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(PARSING_LIBRARY)
#  define PARSING_EXPORT Q_DECL_EXPORT
#elif defined(PARSING_STATIC_LIBRARY)
#  define PARSING_EXPORT
#else
#  define PARSING_EXPORT Q_DECL_IMPORT
#endif

namespace Parsing {

class Diagnostic;
class Engine;
class Managed;
class MemoryPool;

} // namespace Parsing

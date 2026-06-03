// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(COMMONTRACEFORMAT_LIBRARY)
#define CMNTRCEFMT_EXPORT Q_DECL_EXPORT
#elif defined(COMMONTRACEFORMAT_STATIC_LIBRARY)
#define CMNTRCEFMT_EXPORT
#else
#define CMNTRCEFMT_EXPORT Q_DECL_IMPORT
#endif

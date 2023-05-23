// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#if defined(QMLDESIGNERCORE_LIBRARY)
#define QMLDESIGNERCORE_EXPORT Q_DECL_EXPORT
#elif defined(QMLDESIGNERCORE_STATIC_LIBRARY)
#define QMLDESIGNERCORE_EXPORT
#else
#define QMLDESIGNERCORE_EXPORT Q_DECL_IMPORT
#endif

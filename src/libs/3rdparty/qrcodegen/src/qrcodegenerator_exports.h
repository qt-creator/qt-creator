// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#if defined(QRCODEGENERATOR_LIBRARY)
#define QRCODEGENERATOR_EXPORT Q_DECL_EXPORT
#elif defined(QRCODEGENERATOR_STATIC_LIBRARY)
#define QRCODEGENERATOR_EXPORT
#else
#define QRCODEGENERATOR_EXPORT Q_DECL_IMPORT
#endif

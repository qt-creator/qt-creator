// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#ifndef KARCHIVE_EXPORT
#ifdef KARCHIVE_LIBRARY
#define KARCHIVE_EXPORT Q_DECL_EXPORT
#else
#define KARCHIVE_EXPORT Q_DECL_IMPORT
#endif
#endif

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(SCXMLEDITOR_LIBRARY)
#  define SCXMLEDITORSHARED_EXPORT Q_DECL_EXPORT
#else
#  define SCXMLEDITORSHARED_EXPORT Q_DECL_IMPORT
#endif

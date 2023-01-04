// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

# if defined(QMLEDITORWIDGETS_LIBRARY)
#    define QMLEDITORWIDGETS_EXPORT Q_DECL_EXPORT
# elif defined(QMLEDITORWIDGETS_STATIC_LIBRARY)
#    define QMLEDITORWIDGETS_EXPORT
# else
#    define QMLEDITORWIDGETS_EXPORT Q_DECL_IMPORT
#endif

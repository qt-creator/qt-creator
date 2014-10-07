/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#ifndef AUTOTEST_GLOBAL_H
#define AUTOTEST_GLOBAL_H

#include <QtGlobal>

#if defined(AUTOTEST_LIBRARY)
#  define AUTOTESTSHARED_EXPORT Q_DECL_EXPORT
#else
#  define AUTOTESTSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // AUTOTEST_GLOBAL_H


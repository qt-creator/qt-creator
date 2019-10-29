/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

/*
 * This is a precompiled header file for use in Xcode / Mac GCC /
 * GCC >= 3.4 / VC to greatly speed the building of Qt Creator.
 */

#if defined __cplusplus
#include <QtGlobal>

#ifdef Q_OS_WIN
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN

//QT_NO_FLOAT16_OPERATORS is used on Visual Studio 2017 (and earlier):
//when including <QFloat16> and <bitset> in the same translation unit,
//it would cause a compilation error due to a toolchain bug (see [QTBUG-72073])
#if _MSC_VER <= 1920
#define QT_NO_FLOAT16_OPERATORS
#if QT_VERSION < QT_VERSION_CHECK(5, 12, 4)
//https://doc.qt.io/qt-5/qfloat16.html#QT_NO_FLOAT16_OPERATORS
#error QT_NO_FLOAT16_OPERATORS requires Qt 5.12.4
#endif
#endif

// lib/Utils needs defines for Windows 8
#undef WINVER
#define WINVER 0x0602
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0602

#define NOHELP
#include <qt_windows.h>

#undef DELETE
#undef IN
#undef OUT
#undef ERROR
#undef ABSOLUTE

#define _POSIX_
#include <limits.h>
#undef _POSIX_
#endif // Q_OS_WIN

#include <QtCore>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QTextStream>
using Qt::endl;
using Qt::forcesign;
using Qt::noshowbase;
using Qt::dec;
using Qt::showbase;
using Qt::hex;
using Qt::noforcesign;
#endif

#include <stdlib.h>
#endif

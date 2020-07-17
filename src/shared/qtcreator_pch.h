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

#include <QCoreApplication>
#include <QList>
#include <QVariant>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTextCodec>
#include <QPointer>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QDebug>

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

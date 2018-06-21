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

// Include qobjectdefs.h from Qt ...
#include_next <qobjectdefs.h>

#ifndef WRAPPED_QOBJECT_DEFS_H
#define WRAPPED_QOBJECT_DEFS_H

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#pragma clang diagnostic ignored "-Wgnu-string-literal-operator-template"

// ...and redefine macros for tagging signals/slots
#ifdef signals
#  define signals public __attribute__((annotate("qt_signal")))
#endif

#ifdef slots
#  define slots __attribute__((annotate("qt_slot")))
#endif

#ifdef Q_SIGNALS
#  define Q_SIGNALS public __attribute__((annotate("qt_signal")))
#endif

#ifdef Q_SLOTS
#  define Q_SLOTS __attribute__((annotate("qt_slot")))
#endif

#ifdef Q_SIGNAL
#  define Q_SIGNAL __attribute__((annotate("qt_signal")))
#endif

#ifdef Q_SLOT
#  define Q_SLOT __attribute__((annotate("qt_slot")))
#endif

// static_assert can be found as a class child but does not add extra AST nodes for completion
#define Q_PROPERTY(arg) static_assert("Q_PROPERTY", #arg);

#define SIGNAL(arg) #arg
#define SLOT(arg) #arg

#pragma clang diagnostic pop

#endif // WRAPPED_QOBJECT_DEFS_H

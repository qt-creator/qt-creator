// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

#define Q_PROPERTY(arg...) static_assert(static_cast<bool>("Q_PROPERTY"), #arg);

#define SIGNAL(arg) #arg
#define SLOT(arg) #arg

#pragma clang diagnostic pop

#endif // WRAPPED_QOBJECT_DEFS_H

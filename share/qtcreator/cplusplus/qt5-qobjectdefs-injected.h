/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#define QT_NO_META_MACROS

#if defined(QT_NO_KEYWORDS)
# define QT_NO_EMIT
#else
#  ifndef QT_NO_SIGNALS_SLOTS_KEYWORDS
#    define signals public __attribute__((annotate("qt_signal")))
#    define slots __attribute__((annotate("qt_slot")))
#  endif
#endif
#define Q_SIGNALS public __attribute__((annotate("qt_signal")))
#define Q_SLOTS slots __attribute__((annotate("qt_slot")))
#define Q_SIGNAL __attribute__((annotate("qt_signal")))
#define Q_SLOT __attribute__((annotate("qt_slot")))
#define Q_PRIVATE_SLOT(d, signature)

#define Q_EMIT
#ifndef QT_NO_EMIT
# define emit
#endif
#define Q_CLASSINFO(name, value)
#define Q_PLUGIN_METADATA(x)
#define Q_INTERFACES(x)
#define Q_PROPERTY(text)
#define Q_PRIVATE_PROPERTY(d, text)
#define Q_REVISION(v)
#define Q_OVERRIDE(text)
#define Q_ENUMS(x)
#define Q_FLAGS(x)
#define Q_ENUM(x)
#define Q_FLAG(x)
#define Q_SCRIPTABLE
#define Q_INVOKABLE

#define Q_GADGET \
public: \
    static const QMetaObject staticMetaObject; \
private:

#define SIGNAL(a) #a
#define SLOT(a) #a

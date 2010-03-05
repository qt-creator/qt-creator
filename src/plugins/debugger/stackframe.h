/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_STACKFRAME_H
#define DEBUGGER_STACKFRAME_H

#include <QtCore/QString>
#include <QtCore/QMetaType>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

struct StackFrame
{
    StackFrame();
    void clear();
    bool isUsable() const;
    QString toToolTip() const;
    QString toString() const;

    int level;
    QString function;
    QString file;  // We try to put an absolute file name in there.
    QString from;  // Sometimes something like "/usr/lib/libstdc++.so.6"
    QString to;    // Used in ScriptEngine only.
    int line;
    QString address;
};

QDebug operator<<(QDebug d, const  StackFrame &);

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::StackFrame);

#endif // DEBUGGER_STACKFRAME_H

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

#pragma once

#include "debuggerconstants.h"

#include <QCoreApplication>
#include <QMetaType>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class DebuggerRunParameters;
class GdbMi;

class StackFrame
{
public:
    StackFrame();
    void clear();
    bool isUsable() const;
    QString toToolTip() const;
    QString toString() const;
    static StackFrame parseFrame(const GdbMi &data, const DebuggerRunParameters &rp);
    static QList<StackFrame> parseFrames(const GdbMi &data, const DebuggerRunParameters &rp);
    void fixQrcFrame(const DebuggerRunParameters &rp);

public:
    DebuggerLanguage language;
    QString level;
    QString function;
    QString file;        // We try to put an absolute file name in there.
    QString module;      // Sometimes something like "/usr/lib/libstdc++.so.6"
    QString receiver;    // Used in ScriptEngine only.
    qint32 line;
    quint64 address;
    bool usable;
    QString context;  // Opaque value produced and consumed by the native backends.

    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::StackHandler)
};

typedef QList<StackFrame> StackFrames;

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::StackFrame)

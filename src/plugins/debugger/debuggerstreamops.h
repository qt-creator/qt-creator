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

#ifndef DEBUGGERPLUGIN_STREAMOPS_H
#define DEBUGGERPLUGIN_STREAMOPS_H

#include "breakpoint.h"
#include "stackframe.h"
#include "threaddata.h"
#include "watchdata.h"
#include "disassemblerlines.h"

#include <QtCore/QDataStream>
#include <QtCore/QVector>

namespace Debugger {
namespace Internal {

QDataStream &operator<<(QDataStream& stream, const ThreadData &thread);
QDataStream &operator>>(QDataStream& stream, ThreadData &threads);
QDataStream &operator<<(QDataStream& stream, const Threads &threads);
QDataStream &operator>>(QDataStream& stream, Threads &threads);
QDataStream &operator<<(QDataStream& stream, const StackFrame& frame);
QDataStream &operator>>(QDataStream& stream, StackFrame &frame);
QDataStream &operator<<(QDataStream& stream, const StackFrames& frames);
QDataStream &operator>>(QDataStream& stream, StackFrames &frames);
QDataStream &operator<<(QDataStream& stream, const BreakpointParameters &data);
QDataStream &operator>>(QDataStream& stream, BreakpointParameters &data);
QDataStream &operator<<(QDataStream& stream, const BreakpointResponse &data);
QDataStream &operator>>(QDataStream& stream, BreakpointResponse &data);
QDataStream &operator<<(QDataStream& stream, const WatchData &data);
QDataStream &operator>>(QDataStream& stream, WatchData &data);
QDataStream &operator<<(QDataStream& stream, const DisassemblerLine &o);
QDataStream &operator>>(QDataStream& stream, DisassemblerLine &o);
QDataStream &operator<<(QDataStream& stream, const DisassemblerLines &o);
QDataStream &operator>>(QDataStream& stream, DisassemblerLines &o);

} // namespace Internal
} // namespace Debugger

#endif

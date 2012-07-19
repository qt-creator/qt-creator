/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef DEBUGGERPLUGIN_STREAMOPS_H
#define DEBUGGERPLUGIN_STREAMOPS_H

#include "threaddata.h"
#include "stackframe.h"

QT_FORWARD_DECLARE_CLASS(QDataStream)

namespace Debugger {
namespace Internal {

class BreakpointParameters;
class BreakpointResponse;
class WatchData;
class DisassemblerLine;
class DisassemblerLines;

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

/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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

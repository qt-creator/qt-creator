/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cdbdebugoutput.h"
#include "cdbdebugengine.h"
#include "cdbdebugengine_p.h"

#include <windows.h>
#include <inc/dbgeng.h>

namespace Debugger {
namespace Internal {

// ------------------------- CdbDebugOutput

// Return a prefix for debugger messages
static int logChannel(ULONG mask)
{
    if (mask & (DEBUG_OUTPUT_PROMPT_REGISTERS))
        return LogMisc;
    if (mask & (DEBUG_OUTPUT_EXTENSION_WARNING|DEBUG_OUTPUT_WARNING))
        return LogError;
    if (mask & (DEBUG_OUTPUT_ERROR))
        return LogError;
    return LogMisc;
}

enum OutputKind { DebuggerOutput, DebuggerPromptOutput, DebuggeeOutput, DebuggeePromptOutput };

static inline OutputKind outputKind(ULONG mask)
{
    if (mask & DEBUG_OUTPUT_DEBUGGEE)
        return DebuggeeOutput;
    if (mask & DEBUG_OUTPUT_DEBUGGEE_PROMPT)
        return DebuggeePromptOutput;
    if (mask & DEBUG_OUTPUT_PROMPT)
        return DebuggerPromptOutput;
    return DebuggerOutput;
}

CdbDebugOutput::CdbDebugOutput()
{
}

void CdbDebugOutput::output(ULONG mask, const QString &msg)
{
    if (debugCDB > 1)
        qDebug() << Q_FUNC_INFO << "\n    " << msg;

    switch (outputKind(mask)) {
    case DebuggerOutput:
        debuggerOutput(logChannel(mask), msg);
        break;
    case DebuggerPromptOutput:
        emit debuggerInputPrompt(logChannel(mask), msg);
        break;
    case DebuggeeOutput:
        emit debuggeeOutput(msg);
        break;
    case DebuggeePromptOutput:
        emit debuggeeInputPrompt(msg);
        break;
    }
}

} // namespace Internal
} // namespace Debugger

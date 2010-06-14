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

#include "cdbdebugoutput.h"
#include "cdbdebugengine.h"
#include "cdbdebugengine_p.h"
#include "cdbcom.h"
#include "debuggerrunner.h"

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

// ------------------------- CdbDebugOutput

// Return a prefix for debugger messages
static int logChannel(ULONG mask)
{
    if (mask & (DEBUG_OUTPUT_PROMPT_REGISTERS))
        return LogMisc;
    if (mask & (DEBUG_OUTPUT_EXTENSION_WARNING|DEBUG_OUTPUT_WARNING))
        return LogWarning;
    if (mask & (DEBUG_OUTPUT_ERROR))
        return LogError;
    if (mask & DEBUG_OUTPUT_DEBUGGEE)
        //return DebuggeeOutput;
        return AppOutput;
    if (mask & DEBUG_OUTPUT_DEBUGGEE_PROMPT)
        //return DebuggeePromptOutput;
        return AppError;
    if (mask & DEBUG_OUTPUT_PROMPT)
        //return DebuggerPromptOutput;
        return AppError;
    return LogMisc;
}

CdbDebugOutput::CdbDebugOutput(CdbDebugEngine *engine)
  : m_engine(engine)
{
}

void CdbDebugOutput::output(ULONG mask, const QString &msg)
{
    DebuggerRunControl *runControl = m_engine->runControl();
    QTC_ASSERT(runControl, return);
    if (debugCDB > 1)
        qDebug() << Q_FUNC_INFO << "\n    " << msg;
    runControl->showMessage(msg, logChannel(mask));
}

} // namespace Internal
} // namespace Debugger

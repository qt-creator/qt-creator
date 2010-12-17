/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cdbdebugoutput.h"
#include "debuggerrunner.h"
#include "cdbengine_p.h"
#include "cdbcom.h"

#include <QtCore/QDebug>

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

CdbDebugOutput::CdbDebugOutput()
{
}

void CdbDebugOutput::output(ULONG mask, const QString &msg)
{
    if (debugCDB > 1)
        qDebug() << Q_FUNC_INFO << "\n    " << msg;
    emit showMessage(msg, logChannel(mask), -1);
}

} // namespace Internal
} // namespace Debugger

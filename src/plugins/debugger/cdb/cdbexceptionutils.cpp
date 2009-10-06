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

#include "cdbexceptionutils.h"
#include "cdbdebugengine_p.h"
#include "cdbdumperhelper.h"
#include "cdbstacktracecontext.h"

#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>

enum { debugExc = 0 };

// Special exception codes.
enum { cppExceptionCode = 0xe06d7363, startupCompleteTrap = 0x406d1388,
       rpcServerUnavailableExceptionCode = 0x6ba,
       dllNotFoundExceptionCode = 0xc0000135,
       dllInitFailed = 0xc0000142,
       missingSystemFile = 0xc0000143,
       appInitFailed = 0xc0000143
   };

namespace Debugger {
namespace Internal {

static inline void formatDebugFilterExecFlags(ULONG f, const char *title, QTextStream &str)
{
    str.setIntegerBase(16);
    str << ' ' << title << "=0x" << f << " (";
    str.setIntegerBase(10);
    switch (f) {
    case DEBUG_FILTER_BREAK:
        str << "DEBUG_FILTER_BREAK";
        break;
    case DEBUG_FILTER_SECOND_CHANCE_BREAK:
        str << "DEBUG_FILTER_SECOND_CHANCE_BREAK";
        break;
    case DEBUG_FILTER_OUTPUT:
        str << "DEBUG_FILTER_OUTPUT";
        break;
    case DEBUG_FILTER_IGNORE:
        str << "DEBUG_FILTER_IGNORE";
        break;
    }
    str << ')';
}

static inline void formatDebugFilterContFlags(ULONG f, const char *title, QTextStream &str)
{
    str.setIntegerBase(16);
    str << ' ' << title << "=0x" << f << " (";
    str.setIntegerBase(10);
    switch (f) {
    case DEBUG_FILTER_GO_HANDLED:
        str << "DEBUG_FILTER_GO_HANDLED";
        break;
    case DEBUG_FILTER_GO_NOT_HANDLED:
        str << "DEBUG_FILTER_GO_NOT_HANDLED";
    }
    str << ')';
}

ExceptionBlocker::ExceptionBlocker(CIDebugControl *ctrl, ULONG code, Mode m) :
    m_ctrl(ctrl),
    m_code(code),
    m_state(StateError)
{
    // Retrieve current state
    memset(&m_oldParameters, 0, sizeof(DEBUG_EXCEPTION_FILTER_PARAMETERS));
    if (getExceptionParameters(ctrl, code, &m_oldParameters, &m_errorString)) {        
        // Are we in a nested instantiation?
        const ULONG desiredExOption = m == IgnoreException ? DEBUG_FILTER_IGNORE : DEBUG_FILTER_OUTPUT;
        const bool isAlreadyBlocked = m_oldParameters.ExecutionOption == desiredExOption
                                      && m_oldParameters.ContinueOption == DEBUG_FILTER_GO_NOT_HANDLED;
        if (isAlreadyBlocked) {
           m_state = StateNested;
        } else {
            // Nope, block it now.
            DEBUG_EXCEPTION_FILTER_PARAMETERS blockedState = m_oldParameters;
            blockedState.ExecutionOption = desiredExOption;
            blockedState.CommandSize = DEBUG_FILTER_GO_NOT_HANDLED;
            const bool ok = setExceptionParameters(m_ctrl, blockedState, &m_errorString);
            m_state = ok ? StateOk : StateError;
        } // not blocked
    } else {
        m_state = StateError;
    }
    if (debugExc)
        qDebug() << "ExceptionBlocker: state=" << m_state << format(m_oldParameters) << m_errorString;
}

ExceptionBlocker::~ExceptionBlocker()
{
    if (m_state == StateOk) {
        // Restore
        if (debugExc)
            qDebug() << "~ExceptionBlocker: unblocking " << m_oldParameters.ExceptionCode;
        if (!setExceptionParameters(m_ctrl, m_oldParameters, &m_errorString))
            qWarning("Unable to restore exception state for %lu: %s\n", m_oldParameters.ExceptionCode, qPrintable(m_errorString));
    }
}

bool ExceptionBlocker::getExceptionParameters(CIDebugControl *ctrl, ULONG exCode, DEBUG_EXCEPTION_FILTER_PARAMETERS *result, QString *errorMessage)
{
    const HRESULT ihr = ctrl->GetExceptionFilterParameters(1, &exCode, 0, result);
    if (FAILED(ihr)) {
        *errorMessage = msgComFailed("GetExceptionFilterParameters", ihr);
        return false;
    }
    return true;
}

bool ExceptionBlocker::setExceptionParameters(CIDebugControl *ctrl, const DEBUG_EXCEPTION_FILTER_PARAMETERS &p, QString *errorMessage)
{
    const HRESULT ihr = ctrl->SetExceptionFilterParameters(1, const_cast<DEBUG_EXCEPTION_FILTER_PARAMETERS*>(&p));
    if (FAILED(ihr)) {
        *errorMessage = msgComFailed("GetExceptionFilterParameters", ihr);
        return false;
    }
    return true;
}

QString ExceptionBlocker::format(const DEBUG_EXCEPTION_FILTER_PARAMETERS &p)
{
    QString rc;
    QTextStream str(&rc);
    str << "Code=" << p.ExceptionCode;
    formatDebugFilterExecFlags(p.ExecutionOption, "ExecutionOption", str);
    formatDebugFilterContFlags(p.ContinueOption, "ContinueOption", str);
    str << " TextSize=" << p.TextSize << " CommandSizes=" << p.CommandSize << ','
            << p.SecondCommandSize;
    return rc;
}

// ------------------ further exception utilities
// Simple exception formatting
void formatException(const EXCEPTION_RECORD64 *e, QTextStream &str)
{
    str.setIntegerBase(16);
    str << "\nException at 0x"  << e->ExceptionAddress
            <<  ", code: 0x" << e->ExceptionCode << ": ";
    switch (e->ExceptionCode) {
    case cppExceptionCode:
        str << "C++ exception";
        break;
    case startupCompleteTrap:
        str << "Startup complete";
        break;
    case dllNotFoundExceptionCode:
        str << "DLL not found";
        break;
    case dllInitFailed:
        str << "DLL failed to initialize";
        break;
    case missingSystemFile:
        str << "System file is missing";
        break;
    case EXCEPTION_ACCESS_VIOLATION: {
            const bool writeOperation = e->ExceptionInformation[0];
            str << (writeOperation ? "write" : "read")
                << " access violation at: 0x" << e->ExceptionInformation[1];
    }
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        str << "arrary bounds exceeded";
        break;
    case EXCEPTION_BREAKPOINT:
        str << "breakpoint";
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        str << "datatype misalignment";
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        str << "floating point exception";
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        str << "division by zero";
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        str << " floating-point operation cannot be represented exactly as a decimal fraction";
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        str << "invalid floating-point operation";
        break;
    case EXCEPTION_FLT_OVERFLOW:
        str << "floating-point overflow";
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        str << "floating-point operation stack over/underflow";
        break;
    case  EXCEPTION_FLT_UNDERFLOW:
        str << "floating-point UNDERFLOW";
        break;
    case  EXCEPTION_ILLEGAL_INSTRUCTION:
        str << "invalid instruction";
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        str << "page in error";
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        str << "integer division by zero";
        break;
    case EXCEPTION_INT_OVERFLOW:
        str << "integer overflow";
        break;
    case EXCEPTION_INVALID_DISPOSITION:
        str << "invalid disposition to exception dispatcher";
        break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        str << "attempt to continue execution after noncontinuable exception";
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        str << "privileged instruction";
        break;
    case EXCEPTION_SINGLE_STEP:
        str << "single step";
        break;
    case EXCEPTION_STACK_OVERFLOW:
        str << "stack_overflow";
        break;
    }
    str << ", flags=0x" << e->ExceptionFlags;
    if (e->ExceptionFlags == EXCEPTION_NONCONTINUABLE) {
        str << " (execution cannot be continued)";
    }
    str << "\n\n";
    str.setIntegerBase(10);
}

// Format exception with stacktrace in case of C++ exception
void formatException(const EXCEPTION_RECORD64 *e,
                     const QSharedPointer<CdbDumperHelper> &dumper,
                     QTextStream &str)
{
    formatException(e, str);
    if (e->ExceptionCode == cppExceptionCode) {
        QString errorMessage;
        ULONG currentThreadId = 0;
        dumper->comInterfaces()->debugSystemObjects->GetCurrentThreadId(&currentThreadId);
        if (CdbStackTraceContext *stc = CdbStackTraceContext::create(dumper, currentThreadId, &errorMessage)) {
            str << "at:\n";
            stc->format(str);
            str <<'\n';
            delete stc;
        }
    }
}

bool isFatalException(LONG code)
{
    switch (code) {
    case EXCEPTION_BREAKPOINT:
    case EXCEPTION_SINGLE_STEP:
    case startupCompleteTrap: // Mysterious exception at start of application
    case rpcServerUnavailableExceptionCode:
    case dllNotFoundExceptionCode:
    case cppExceptionCode:
        return false;
    default:
        break;
    }
    return true;
}

} // namespace Internal
} // namespace Debugger

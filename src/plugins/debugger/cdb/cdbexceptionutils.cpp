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

#include "cdbexceptionutils.h"
#include "cdbengine_p.h"
#include "stacktracecontext.h"
#include "dbgwinutils.h"

#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>

enum { debugExc = 0 };

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
        *errorMessage = CdbCore::msgComFailed("GetExceptionFilterParameters", ihr);
        return false;
    }
    return true;
}

bool ExceptionBlocker::setExceptionParameters(CIDebugControl *ctrl, const DEBUG_EXCEPTION_FILTER_PARAMETERS &p, QString *errorMessage)
{
    const HRESULT ihr = ctrl->SetExceptionFilterParameters(1, const_cast<DEBUG_EXCEPTION_FILTER_PARAMETERS*>(&p));
    if (FAILED(ihr)) {
        *errorMessage = CdbCore::msgComFailed("GetExceptionFilterParameters", ihr);
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
    formatWindowsException(e->ExceptionCode, e->ExceptionAddress,
                           e->ExceptionFlags,
                           e->ExceptionInformation[0],
                           e->ExceptionInformation[1],
                           str);
    str << "\n\n";
}

// Format exception with stacktrace in case of C++ exception
void formatException(const EXCEPTION_RECORD64 *e,
                     const CdbCore::ComInterfaces *cif,
                     QTextStream &str)
{
    formatException(e, str);
    if (e->ExceptionCode == winExceptionCppException) {
        QString errorMessage;
        if (CdbCore::StackTraceContext *stc = CdbCore::StackTraceContext::create(cif, 9999, &errorMessage)) {
            str << "at:\n";
            stc->format(str);
            str <<'\n';
            delete stc;
        }
    }
}

} // namespace Internal
} // namespace Debugger

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

#ifndef CDBEXCEPTIONUTILS_H
#define CDBEXCEPTIONUTILS_H

#include "cdbcom.h"

#include <QtCore/QString>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace CdbCore {
    struct ComInterfaces;
}

namespace Debugger {
namespace Internal {

// Special exception codes.
enum { winExceptionCppException = 0xe06d7363,
       winExceptionStartupCompleteTrap = 0x406d1388,
       winExceptionRpcServerUnavailable = 0x6ba,
       winExceptionDllNotFound = 0xc0000135,
       winExceptionDllInitFailed = 0xc0000142,
       winExceptionMissingSystemFile = 0xc0000143,
       winExceptionAppInitFailed = 0xc0000143
};


class CdbDumperHelper;

// Utility class that blocks out exception handling (breaking)
// for a specific exception (like EXCEPTION_ACCESS_VIOLATION) while in scope.
class ExceptionBlocker {
    Q_DISABLE_COPY(ExceptionBlocker)
public:
    // Log mode. Note: Does not influence the output callbacks.
    enum Mode {
        IgnoreException,  // Ignore & suppress debugger console notification
        LogException      // Ignore, still print console notification
    };

    ExceptionBlocker(CIDebugControl *ctrl, ULONG exceptionCode, Mode mode);
    ~ExceptionBlocker();

    operator bool() const { return m_state != StateError; }
    QString errorString() const { return m_errorString; }

    // Helpers
    static bool getExceptionParameters(CIDebugControl *ctrl, ULONG exCode, DEBUG_EXCEPTION_FILTER_PARAMETERS *result, QString *errorMessage);
    static bool setExceptionParameters(CIDebugControl *ctrl, const DEBUG_EXCEPTION_FILTER_PARAMETERS &p, QString *errorMessage);
    static QString format(const DEBUG_EXCEPTION_FILTER_PARAMETERS &p);

private:
    enum State { StateOk,
                 StateNested,  // Nested call, exception already blocked, do nothing
                 StateError };

    CIDebugControl *m_ctrl;
    const LONG m_code;
    DEBUG_EXCEPTION_FILTER_PARAMETERS m_oldParameters;
    State m_state;
    QString m_errorString;
};

// Format exception
void formatException(const EXCEPTION_RECORD64 *e, QTextStream &str);

// Format exception with stacktrace in case of C++ exception
void formatException(const EXCEPTION_RECORD64 *e,
                     const CdbCore::ComInterfaces *cif,
                     QTextStream &str);

// Is this a crash/recoverable?
bool isFatalException(LONG code);

} // namespace Internal
} // namespace Debugger

#endif // CDBEXCEPTIONUTILS_H

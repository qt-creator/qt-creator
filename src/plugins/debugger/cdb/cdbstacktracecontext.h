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

#ifndef CDBSTACKTRACECONTEXT_H
#define CDBSTACKTRACECONTEXT_H

#include "stackhandler.h"

#include "cdbcom.h"

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

struct CdbComInterfaces;
class CdbSymbolGroupContext;
class CdbStackFrameContext;
class CdbDumperHelper;
struct ThreadData;

/* Context representing a break point stack consisting of several frames.
 * Maintains an on-demand constructed list of CdbStackFrameContext
 * containining the local variables of the stack. */

class CdbStackTraceContext
{
    Q_DISABLE_COPY(CdbStackTraceContext)

    explicit CdbStackTraceContext(const QSharedPointer<CdbDumperHelper> &dumper);
public:
    enum { maxFrames = 100 };

    // Some well known-functions
    static const char *winFuncFastSystemCallRet;
    // WaitFor...
    static const char *winFuncWaitForPrefix;
    static const char *winFuncMsgWaitForPrefix;

    // Dummy function used for interrupting a debuggee
    static const char *winFuncDebugBreakPoint;

    ~CdbStackTraceContext();
    static CdbStackTraceContext *create(const QSharedPointer<CdbDumperHelper> &dumper,
                                        unsigned long threadid,
                                        QString *errorMessage);

    QList<StackFrame> frames() const { return m_frames; }
    inline int frameCount() const { return m_frames.size(); }
    // Search for function. Should ideally contain the module as 'module!foo'.
    int indexOf(const QString &function) const;

    // Top-Level instruction offset for disassembler
    ULONG64 instructionOffset() const { return m_instructionOffset; }

    CdbStackFrameContext *frameContextAt(int index, QString *errorMessage);

    // Format for logging
    void format(QTextStream &str) const;
    QString toString() const;

    // Retrieve information about threads. When stopped, add
    // current stack frame.
    static bool getThreads(const CdbComInterfaces &cif,
                           bool isStopped,
                           QList<ThreadData> *threads,
                           ULONG *currentThreadId,
                           QString *errorMessage);

private:
    bool init(unsigned long frameCount, QString *errorMessage);
    CIDebugSymbolGroup *createSymbolGroup(int index, QString *errorMessage);

    const QSharedPointer<CdbDumperHelper> m_dumper;
    CdbComInterfaces *m_cif;

    DEBUG_STACK_FRAME m_cdbFrames[maxFrames];
    QVector <CdbStackFrameContext*> m_frameContexts;
    QList<StackFrame> m_frames;
    ULONG64 m_instructionOffset;
};

} // namespace Internal
} // namespace Debugger

#endif // CDBSTACKTRACECONTEXT_H

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

#ifndef CORESTACKTRACECONTEXT_H
#define CORESTACKTRACECONTEXT_H

#include "cdbcom.h"

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QSharedPointer>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QTextStream;
class QDebug;
QT_END_NAMESPACE

namespace CdbCore {

struct ComInterfaces;
class SymbolGroupContext;


struct StackFrame {
    StackFrame();

    bool hasFile() const { return !fileName.isEmpty(); }
    void format(QTextStream &) const;

    QString toString() const;

    QString module;
    QString function;
    QString fileName;
    ULONG line;
    ULONG64 address;
};

QDebug operator<<(QDebug d, const StackFrame &);

/* Context representing a break point stack consisting of several frames.
 * Maintains an on-demand constructed list of SymbolGroupContext
 * containining the local variables of the stack. */

class StackTraceContext
{
    Q_DISABLE_COPY(StackTraceContext)

protected:
    explicit StackTraceContext(const ComInterfaces *cif);
    bool init(unsigned long maxFramesIn, QString *errorMessage);

public:
    // Utilities to check for special functions
    enum SpecialFunction {
        BreakPointFunction,
        WaitFunction,
        KiFastSystemCallRet,
        None
    };
    static SpecialFunction specialFunction(const QString &module, const QString &function);

    // A map of thread id, stack frame
    typedef QMap<unsigned long, StackFrame> ThreadIdFrameMap;

    enum { maxFrames = 100 };

    ~StackTraceContext();
    static StackTraceContext *create(const ComInterfaces *cif,
                                     unsigned long maxFramesIn,
                                     QString *errorMessage);

    // Search for function. Empty module means "don't match on module"
    int indexOf(const QString &function, const QString &module = QString()) const;

    // Top-Level instruction offset for disassembler
    ULONG64 instructionOffset() const { return m_instructionOffset; }
    int frameCount() const { return m_frames.size(); }

    SymbolGroupContext *symbolGroupContextAt(int index, QString *errorMessage);
    const StackFrame stackFrameAt(int index) const { return m_frames.at(index); }

    // Format for logging
    void format(QTextStream &str) const;
    QString toString() const;

    // Thread helpers: Retrieve a list of thread ids. Also works when running.
    static inline bool getThreadIds(const CdbCore::ComInterfaces &cif,
                                    QVector<ULONG> *threadIds,
                                    ULONG *currentThreadId,
                                    QString *errorMessage);

    // Retrieve detailed information about a threads in stopped state.
    // Potentially changes current thread id.
    static inline bool getStoppedThreadState(const CdbCore::ComInterfaces &cif,
                                             unsigned long id,
                                             StackFrame *topFrame,
                                             QString *errorMessage);

    // Retrieve detailed information about all threads, works in stopped state.
    static bool getThreads(const CdbCore::ComInterfaces &cif,
                           ThreadIdFrameMap *threads,
                           ULONG *currentThreadId,
                           QString *errorMessage);

    static QString formatThreads(const ThreadIdFrameMap &threads);

protected:
    virtual SymbolGroupContext *createSymbolGroup(const ComInterfaces &cif,
                                                  int index,
                                                  const QString &prefix,
                                                  CIDebugSymbolGroup *comSymbolGroup,
                                                  QString *errorMessage);

    static QString msgFrameContextFailed(int index, const StackFrame &f, const QString &why);

private:    
    CIDebugSymbolGroup *createCOM_SymbolGroup(int index, QString *errorMessage);
    inline static StackFrame frameFromFRAME(const CdbCore::ComInterfaces &cif,
                                            const DEBUG_STACK_FRAME &s);

    // const QSharedPointer<CdbDumperHelper> m_dumper;
    const ComInterfaces *m_cif;

    DEBUG_STACK_FRAME m_cdbFrames[maxFrames];
    QVector <SymbolGroupContext*> m_frameContexts;
    QVector<StackFrame> m_frames;
    ULONG64 m_instructionOffset;
};

QDebug operator<<(QDebug d, const StackTraceContext &);

} // namespace Internal

#endif // CORESTACKTRACECONTEXT_H

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

struct Thread {
    explicit Thread(unsigned long id = 0, unsigned long sysId = 0);
    QString toString() const;

    unsigned long id;
    unsigned long systemId;
    quint64 dataOffset; // Only for current.
    QString name;
};

inline bool operator<(const Thread &t1, const Thread &t2) { return t1.id < t2.id; }

QDebug operator<<(QDebug d, const StackFrame &);
QDebug operator<<(QDebug d, const Thread &);

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
    static bool getThreadList(const CdbCore::ComInterfaces &cif,
                              QVector<Thread> *threads,
                              ULONG *currentThreadId,
                              QString *errorMessage);

    // Retrieve detailed information about a threads in stopped state.
    // Potentially changes current thread id.
    static inline bool getStoppedThreadState(const CdbCore::ComInterfaces &cif,
                                             unsigned long id,
                                             StackFrame *topFrame,
                                             QString *errorMessage);

    // Get the stack traces for threads in stopped state (only, fails when running).
    // Potentially changes and restores current thread.
    static bool getStoppedThreadFrames(const CdbCore::ComInterfaces &cif,
                                       ULONG currentThreadId,
                                       const QVector<Thread> &threads,
                                       QVector<StackFrame> *frames,
                                       QString *errorMessage);

protected:
    virtual SymbolGroupContext *createSymbolGroup(const ComInterfaces &cif,
                                                  int index,
                                                  const QString &prefix,
                                                  CIDebugSymbolGroup *comSymbolGroup,
                                                  QString *errorMessage);

    static QString msgFrameContextFailed(int index, const StackFrame &f, const QString &why);

private:    
    bool setScope(int index, QString *errorMessage);
    CIDebugSymbolGroup *createCOM_SymbolGroup(int index, QString *errorMessage);
    inline static StackFrame frameFromFRAME(const CdbCore::ComInterfaces &cif,
                                            const DEBUG_STACK_FRAME &s);

    // const QSharedPointer<CdbDumperHelper> m_dumper;
    const ComInterfaces *m_cif;

    DEBUG_STACK_FRAME m_cdbFrames[maxFrames];
    QVector <SymbolGroupContext*> m_frameContexts;
    QVector<StackFrame> m_frames;
    ULONG64 m_instructionOffset;
    int m_lastIndex;
};

QDebug operator<<(QDebug d, const StackTraceContext &);

} // namespace Internal

#endif // CORESTACKTRACECONTEXT_H

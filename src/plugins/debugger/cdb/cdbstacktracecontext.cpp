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

#include "cdbstacktracecontext.h"

#include "cdbsymbolgroupcontext.h"
#include "cdbdumperhelper.h"
#include "cdbengine_p.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "watchutils.h"
#include "threadshandler.h"

#include <utils/savedaction.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>

enum { debug = 0 };

namespace Debugger {
namespace Internal {

CdbStackTraceContext::CdbStackTraceContext(const QSharedPointer<CdbDumperHelper> &dumper) :
        CdbCore::StackTraceContext(dumper->comInterfaces()),
        m_dumper(dumper)
{
}

CdbStackTraceContext *CdbStackTraceContext::create(const QSharedPointer<CdbDumperHelper> &dumper,
                                                   QString *errorMessage)
{
    CdbStackTraceContext *ctx = new CdbStackTraceContext(dumper);
    if (!ctx->init(UINT_MAX, errorMessage)) {
        delete ctx;
        return 0;
    }
    return ctx;
}

CdbCore::SymbolGroupContext *
CdbStackTraceContext::createSymbolGroup(const CdbCore::ComInterfaces & /* cif */,
                                        int index,
                                        const QString &prefix,
                                        CIDebugSymbolGroup *comSymbolGroup,
                                        QString *errorMessage)
{
    // Exclude uninitialized variables if desired
    QStringList uninitializedVariables;
    const CdbCore::StackFrame &frame = stackFrameAt(index);
    if (debuggerCore()->action(UseCodeModel)->isChecked())
        getUninitializedVariables(debuggerCore()->cppCodeModelSnapshot(),
            frame.function, frame.fileName, frame.line, &uninitializedVariables);
    if (debug)
        qDebug() << frame << uninitializedVariables;
    CdbSymbolGroupContext *sc = CdbSymbolGroupContext::create(prefix,
                                                              comSymbolGroup,
                                                              m_dumper,
                                                              uninitializedVariables,
                                                              errorMessage);
    if (!sc) {
        *errorMessage = msgFrameContextFailed(index, frame, *errorMessage);
        return 0;
    }
    return sc;
}

CdbSymbolGroupContext *CdbStackTraceContext::cdbSymbolGroupContextAt(int index, QString *errorMessage)
{
    return static_cast<CdbSymbolGroupContext *>(symbolGroupContextAt(index, errorMessage));
}

QList<StackFrame> CdbStackTraceContext::stackFrames() const
{
    // Convert from Core data structures
    QList<StackFrame> rc;
    const int count = frameCount();
    for(int i = 0; i < count; i++) {
        const CdbCore::StackFrame &coreFrame = stackFrameAt(i);
        StackFrame frame;
        frame.level = i;
        frame.file = coreFrame.fileName;
        frame.usable = !frame.file.isEmpty() && QFileInfo(frame.file).isFile();
        frame.line = coreFrame.line;
        frame.function = coreFrame.function;
        frame.from = coreFrame.module;
        frame.address = coreFrame.address;
        rc.push_back(frame);
    }
    return rc;
}

bool CdbStackTraceContext::getThreads(const CdbCore::ComInterfaces &cif,
                                      bool stopped,
                                      Threads *threads,
                                      ULONG *currentThreadId,
                                      QString *errorMessage)
{

    QVector<CdbCore::Thread> coreThreads;
    if (!CdbCore::StackTraceContext::getThreadList(cif, &coreThreads, currentThreadId, errorMessage))
        return false;
    // Get frames only if stopped.
    QVector<CdbCore::StackFrame> frames;
    if (stopped)
        if (!CdbCore::StackTraceContext::getStoppedThreadFrames(cif, *currentThreadId,
                                                                coreThreads, &frames, errorMessage))
        return false;
    // Convert from Core data structures
    threads->clear();
    const int count = coreThreads.size();
    if (!count)
        return true;
    threads->reserve(count);
    const QChar slash(QLatin1Char('/'));
    for (int i = 0; i < count; i++) {
        const CdbCore::Thread &coreThread = coreThreads.at(i);
        ThreadData data(coreThread.id);
        data.targetId = QLatin1String("0x") + QString::number(coreThread.systemId);
        data.name = coreThread.name;
        if (stopped) {
            const CdbCore::StackFrame &coreFrame = frames.at(i);
            data.address = coreFrame.address;
            data.function = coreFrame.function;
            data.lineNumber = coreFrame.line;
            // Basename only for brevity
            const int slashPos = coreFrame.fileName.lastIndexOf(slash);
            data.fileName = slashPos == -1 ? coreFrame.fileName : coreFrame.fileName.mid(slashPos + 1);
        }
        threads->push_back(data);
    }
    return true;
}

} // namespace Internal
} // namespace Debugger

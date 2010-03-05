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

#include "cdbstacktracecontext.h"
#include "cdbsymbolgroupcontext.h"
#include "cdbdumperhelper.h"
#include "cdbdebugengine_p.h"
#include "debuggeractions.h"
#include "watchutils.h"

#include <QtCore/QDebug>

enum { debug = 1 };

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

CdbCore::SymbolGroupContext
        *CdbStackTraceContext::createSymbolGroup(const CdbCore::ComInterfaces & /* cif */,
                                                 int index,
                                                 const QString &prefix,
                                                 CIDebugSymbolGroup *comSymbolGroup,
                                                 QString *errorMessage)
{
    // Exclude uninitialized variables if desired
    QStringList uninitializedVariables;
    const CdbCore::StackFrame &frame = stackFrameAt(index);
    if (theDebuggerAction(UseCodeModel)->isChecked())
        getUninitializedVariables(DebuggerManager::instance()->cppCodeModelSnapshot(), frame.function, frame.fileName, frame.line, &uninitializedVariables);
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
    const QString hexPrefix = QLatin1String("0x");
    for(int i = 0; i < count; i++) {
        const CdbCore::StackFrame &coreFrame = stackFrameAt(i);
        StackFrame frame;
        frame.level = i;
        frame.file = coreFrame.fileName;
        frame.line = coreFrame.line;
        frame.function =coreFrame.function;
        frame.from = coreFrame.module;
        frame.address = hexPrefix + QString::number(coreFrame.address, 16);
        rc.push_back(frame);
    }
    return rc;
}

bool CdbStackTraceContext::getThreads(const CdbCore::ComInterfaces &cif,
                                      QList<ThreadData> *threads,
                                      ULONG *currentThreadId,
                                      QString *errorMessage)
{
    // Convert from Core data structures
    threads->clear();
    ThreadIdFrameMap threadMap;
    if (!CdbCore::StackTraceContext::getThreads(cif, &threadMap,
                                               currentThreadId, errorMessage))
        return false;
    const QChar slash = QLatin1Char('/');
    const ThreadIdFrameMap::const_iterator cend = threadMap.constEnd();
    for (ThreadIdFrameMap::const_iterator it = threadMap.constBegin(); it != cend; ++it) {
        ThreadData data(it.key());
        const CdbCore::StackFrame &coreFrame = it.value();
        data.address = coreFrame.address;
        data.function = coreFrame.function;
        data.line = coreFrame.line;
        // Basename only for brevity
        const int slashPos = coreFrame.fileName.lastIndexOf(slash);
        data.file = slashPos == -1 ? coreFrame.fileName : coreFrame.fileName.mid(slashPos + 1);
        threads->push_back(data);
    }
    return true;
}


} // namespace Internal
} // namespace Debugger

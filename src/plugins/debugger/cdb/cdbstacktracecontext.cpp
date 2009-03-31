/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cdbstacktracecontext.h"
#include "cdbsymbolgroupcontext.h"
#include "cdbdebugengine_p.h"

namespace Debugger {
namespace Internal {

CdbStackTraceContext::CdbStackTraceContext(IDebugSystemObjects4* pDebugSystemObjects,
                                           IDebugSymbols3* pDebugSymbols) :
        m_pDebugSystemObjects(pDebugSystemObjects),
        m_pDebugSymbols(pDebugSymbols)
{
}

CdbStackTraceContext *CdbStackTraceContext::create(IDebugControl4* pDebugControl,
                                                   IDebugSystemObjects4* pDebugSystemObjects,
                                                   IDebugSymbols3* pDebugSymbols,
                                                   unsigned long threadId,
                                                   QString *errorMessage)
{    
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << threadId;
    HRESULT hr = pDebugSystemObjects->SetCurrentThreadId(threadId);
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("%1: SetCurrentThreadId %2 failed: %3").
                        arg(QString::fromLatin1(Q_FUNC_INFO)).
                        arg(threadId).
                        arg(msgDebugEngineComResult(hr));
        return 0;
    }
    // fill the DEBUG_STACK_FRAME array
    ULONG frameCount;
    CdbStackTraceContext *ctx = new CdbStackTraceContext(pDebugSystemObjects, pDebugSymbols);
    hr = pDebugControl->GetStackTrace(0, 0, 0, ctx->m_cdbFrames, CdbStackTraceContext::maxFrames, &frameCount);
    if (FAILED(hr)) {
        delete ctx;
         *errorMessage = msgComFailed("GetStackTrace", hr);
        return 0;
    }
    if (!ctx->init(frameCount, errorMessage)) {
        delete ctx;
        return 0;

    }
    return ctx;
}

CdbStackTraceContext::~CdbStackTraceContext()
{
    qDeleteAll(m_symbolContexts);
}

bool CdbStackTraceContext::init(unsigned long frameCount, QString * /*errorMessage*/)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << frameCount;

    m_symbolContexts.resize(frameCount);
    qFill(m_symbolContexts, static_cast<CdbSymbolGroupContext*>(0));

    // Convert the DEBUG_STACK_FRAMEs to our StackFrame structure and populate the frames
    WCHAR wszBuf[MAX_PATH];
    for (ULONG i=0; i < frameCount; ++i) {
        StackFrame frame(i);
        const ULONG64 instructionOffset = m_cdbFrames[i].InstructionOffset;
        frame.address = QString::fromLatin1("0x%1").arg(instructionOffset, 0, 16);

        m_pDebugSymbols->GetNameByOffsetWide(instructionOffset, wszBuf, MAX_PATH, 0, 0);
        frame.function = QString::fromUtf16(wszBuf);

        ULONG ulLine;
        ULONG ulFileNameSize;
        ULONG64 ul64Displacement;
        const HRESULT hr = m_pDebugSymbols->GetLineByOffsetWide(instructionOffset, &ulLine, wszBuf, MAX_PATH, &ulFileNameSize, &ul64Displacement);
        if (SUCCEEDED(hr)) {
            frame.line = ulLine;
            frame.file = QString::fromUtf16(wszBuf, ulFileNameSize);
        }
        m_frames.push_back(frame);
    }
    return true;
}

CdbSymbolGroupContext *CdbStackTraceContext::symbolGroupContextAt(int index, QString *errorMessage)
{
    // Create a symbol group on demand
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << index << m_symbolContexts.at(index);

    if (index < 0 || index >= m_symbolContexts.size()) {
        *errorMessage = QString::fromLatin1("%1: Index %2 out of range %3.").
                        arg(QLatin1String(Q_FUNC_INFO)).arg(index).arg(m_symbolContexts.size());
        return 0;
    }

    if (m_symbolContexts.at(index))
        return m_symbolContexts.at(index);
    IDebugSymbolGroup2 *sg  = createSymbolGroup(index, errorMessage);
    if (!sg)
        return 0;
    CdbSymbolGroupContext *sc = CdbSymbolGroupContext::create(QLatin1String("local"), sg, errorMessage);
    if (!sc)
        return 0;                                \
    m_symbolContexts[index] = sc;
    return sc;
}

IDebugSymbolGroup2 *CdbStackTraceContext::createSymbolGroup(int index, QString *errorMessage)
{
    IDebugSymbolGroup2 *sg = 0;
    HRESULT hr = m_pDebugSymbols->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, NULL, &sg);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("GetScopeSymbolGroup", hr);
        return 0;
    }

    hr = m_pDebugSymbols->SetScope(0, m_cdbFrames + index, NULL, 0);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("SetScope", hr);
        sg->Release();
        return 0;
    }
    // refresh with current frame
    hr = m_pDebugSymbols->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, sg, &sg);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("GetScopeSymbolGroup", hr);
        sg->Release();
        return 0;
    }
    return sg;
}

}
}

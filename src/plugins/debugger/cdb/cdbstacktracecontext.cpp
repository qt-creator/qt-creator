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

#include "cdbstacktracecontext.h"
#include "cdbstackframecontext.h"
#include "cdbbreakpoint.h"
#include "cdbsymbolgroupcontext.h"
#include "cdbdebugengine_p.h"
#include "cdbdumperhelper.h"

#include <QtCore/QDir>
#include <QtCore/QTextStream>

namespace Debugger {
namespace Internal {

CdbStackTraceContext::CdbStackTraceContext(const QSharedPointer<CdbDumperHelper> &dumper) :
        m_dumper(dumper),
        m_cif(dumper->comInterfaces()),
        m_instructionOffset(0)
{
}

CdbStackTraceContext *CdbStackTraceContext::create(const QSharedPointer<CdbDumperHelper> &dumper,
                                                   unsigned long threadId,
                                                   QString *errorMessage)
{    
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << threadId;
    // fill the DEBUG_STACK_FRAME array
    ULONG frameCount;
    CdbStackTraceContext *ctx = new CdbStackTraceContext(dumper);
    const HRESULT hr = dumper->comInterfaces()->debugControl->GetStackTrace(0, 0, 0, ctx->m_cdbFrames, CdbStackTraceContext::maxFrames, &frameCount);
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
    qDeleteAll(m_frameContexts);
}

bool CdbStackTraceContext::init(unsigned long frameCount, QString * /*errorMessage*/)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << frameCount;

    m_frameContexts.resize(frameCount);
    qFill(m_frameContexts, static_cast<CdbStackFrameContext*>(0));

    // Convert the DEBUG_STACK_FRAMEs to our StackFrame structure and populate the frames
    WCHAR wszBuf[MAX_PATH];
    for (ULONG i=0; i < frameCount; ++i) {
        StackFrame frame;
        frame.level = i;
        const ULONG64 instructionOffset = m_cdbFrames[i].InstructionOffset;
        if (i == 0)
            m_instructionOffset = instructionOffset;
        frame.address = QString::fromLatin1("0x%1").arg(instructionOffset, 0, 16);

        m_cif->debugSymbols->GetNameByOffsetWide(instructionOffset, wszBuf, MAX_PATH, 0, 0);
        frame.function = QString::fromUtf16(reinterpret_cast<const ushort *>(wszBuf));

        ULONG ulLine;
        ULONG64 ul64Displacement;
        const HRESULT hr = m_cif->debugSymbols->GetLineByOffsetWide(instructionOffset, &ulLine, wszBuf, MAX_PATH, 0, &ul64Displacement);
        if (SUCCEEDED(hr)) {
            frame.line = ulLine;
            // Vitally important  to use canonical file that matches editormanager,
            // else the marker will not show.
            frame.file = CDBBreakPoint::canonicalSourceFile(QString::fromUtf16(reinterpret_cast<const ushort *>(wszBuf)));
        }
        m_frames.push_back(frame);
    }
    return true;
}

int CdbStackTraceContext::indexOf(const QString &function) const
{    

    const QChar exclamationMark = QLatin1Char('!');
    const int count = m_frames.size();
    // Module contained ('module!foo'). Exact match
    if (function.contains(exclamationMark)) {

        for (int i = 0; i < count; i++)
            if (m_frames.at(i).function == function)
                return i;
        return -1;
    }
    // No module, fuzzy match
    QString pattern = exclamationMark + function;
    for (int i = 0; i < count; i++)
        if (m_frames.at(i).function.endsWith(pattern))
            return i;
    return -1;
}

static inline QString msgFrameContextFailed(int index, const StackFrame &f, const QString &why)
{
    return QString::fromLatin1("Unable to create stack frame context #%1, %2:%3 (%4): %5").
            arg(index).arg(f.function).arg(f.line).arg(f.file, why);
}

CdbStackFrameContext *CdbStackTraceContext::frameContextAt(int index, QString *errorMessage)
{
    // Create a frame on demand
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << index;

    if (index < 0 || index >= m_frameContexts.size()) {
        *errorMessage = QString::fromLatin1("%1: Index %2 out of range %3.").
                        arg(QLatin1String(Q_FUNC_INFO)).arg(index).arg(m_frameContexts.size());
        return 0;
    }
    if (m_frameContexts.at(index))
        return m_frameContexts.at(index);
    CIDebugSymbolGroup *sg  = createSymbolGroup(index, errorMessage);
    if (!sg) {
        *errorMessage = msgFrameContextFailed(index, m_frames.at(index), *errorMessage);
        return 0;
    }
    CdbSymbolGroupContext *sc = CdbSymbolGroupContext::create(QLatin1String("local"), sg, errorMessage);
    if (!sc) {
        *errorMessage = msgFrameContextFailed(index, m_frames.at(index), *errorMessage);
        return 0;
    }
    CdbStackFrameContext *fr = new CdbStackFrameContext(m_dumper, sc);
    m_frameContexts[index] = fr;
    return fr;
}

CIDebugSymbolGroup *CdbStackTraceContext::createSymbolGroup(int index, QString *errorMessage)
{
    CIDebugSymbolGroup *sg = 0;
    HRESULT hr = m_cif->debugSymbols->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, NULL, &sg);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("GetScopeSymbolGroup", hr);
        return 0;
    }

    hr = m_cif->debugSymbols->SetScope(0, m_cdbFrames + index, NULL, 0);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("SetScope", hr);
        sg->Release();
        return 0;
    }
    // refresh with current frame
    hr = m_cif->debugSymbols->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, sg, &sg);
    if (FAILED(hr)) {
        *errorMessage = msgComFailed("GetScopeSymbolGroup", hr);
        sg->Release();
        return 0;
    }
    return sg;
}

QString CdbStackTraceContext::toString() const
{
    QString rc;
    QTextStream str(&rc);
    format(str);
    return rc;
}

void CdbStackTraceContext::format(QTextStream &str) const
{
    const int count = m_frames.count();
    const int defaultFieldWidth = str.fieldWidth();
    const QTextStream::FieldAlignment defaultAlignment = str.fieldAlignment();
    for (int f = 0; f < count; f++) {
        const StackFrame &frame = m_frames.at(f);
        const bool hasFile = !frame.file.isEmpty();
        // left-pad level
        str << qSetFieldWidth(6) << left << f;
        str.setFieldWidth(defaultFieldWidth);
        str.setFieldAlignment(defaultAlignment);
        if (hasFile)
            str << QDir::toNativeSeparators(frame.file) << ':' << frame.line << " (";
        str << frame.function;
        if (hasFile)
            str << ')';
        str << '\n';
    }
}

} // namespace Internal
} // namespace Debugger

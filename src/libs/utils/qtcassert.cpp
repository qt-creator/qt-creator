// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qtcassert.h"

#include <QByteArray>
#include <QDebug>
#include <QMutex>

#if defined(Q_OS_UNIX)
#include <stdio.h>
#include <signal.h>
#include <execinfo.h>
#elif defined(_MSC_VER)
#ifdef QTCREATOR_PCH_H
#define CALLBACK WINAPI
#define OUT
#define IN
#endif
#include <Windows.h>
#include <DbgHelp.h>
#endif

namespace Utils {

void dumpBacktrace(int maxdepth)
{
    const int ArraySize = 1000;
    if (maxdepth < 0 || maxdepth > ArraySize)
        maxdepth = ArraySize;
#if defined(Q_OS_UNIX)
    void *bt[ArraySize] = {nullptr};
    int size = backtrace(bt, maxdepth);
    char **lines = backtrace_symbols(bt, size);
    for (int i = 0; i < size; ++i)
        qDebug() << "0x" + QByteArray::number(quintptr(bt[i]), 16) << lines[i];
    free(lines);
#elif defined(_MSC_VER)
    DWORD machineType;
#if defined(Q_OS_WIN64)
    machineType = IMAGE_FILE_MACHINE_AMD64;
#else
    return;
#endif
    static QMutex mutex;
    mutex.lock();
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    CONTEXT ctx;
    RtlCaptureContext(&ctx);
    STACKFRAME64 frame;
    memset(&frame, 0, sizeof(STACKFRAME64));
#if defined(Q_OS_WIN64)
    frame.AddrPC.Offset = ctx.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
#endif
    int depth = 0;

    static bool symbolsInitialized = false;
    if (!symbolsInitialized) {
        SymInitialize(process, NULL, TRUE);
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        symbolsInitialized = true;
    }

    while (StackWalk64(machineType,
                       process,
                       thread,
                       &frame,
                       &ctx,
                       NULL,
                       &SymFunctionTableAccess64,
                       &SymGetModuleBase64,
                       NULL)) {
        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 displacement = 0;
        if (!SymFromAddr(process, frame.AddrPC.Offset, &displacement, pSymbol))
            break;

        DWORD symDisplacement = 0;
        IMAGEHLP_LINE64 lineInfo;
        SymSetOptions(SYMOPT_LOAD_LINES);

        QString out = QString("%1: 0x%2 at %3")
                          .arg(depth)
                          .arg(QString::number(pSymbol->Address, 16))
                          .arg(QString::fromLatin1(&pSymbol->Name[0], pSymbol->NameLen));
        if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &symDisplacement, &lineInfo)) {
            out.append(QString(" at %3:%4")
                           .arg(QString::fromLatin1(lineInfo.FileName),
                                QString::number(lineInfo.LineNumber)));
        }
        qDebug() << out;
        if (++depth == maxdepth)
            break;
    }
    mutex.unlock();
#endif
}

void writeAssertLocation(const char *msg)
{
    static bool goBoom = qEnvironmentVariableIsSet("QTC_FATAL_ASSERTS");
    if (goBoom)
        qFatal("SOFT ASSERT made fatal: %s", msg);
    else
        qDebug("SOFT ASSERT: %s", msg);

    static int maxdepth = qEnvironmentVariableIntValue("QTC_BACKTRACE_MAXDEPTH");
    if (maxdepth != 0)
        dumpBacktrace(maxdepth);
}

} // namespace Utils

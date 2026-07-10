// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggerengineinterface.h"

#include <utils/environment.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace Debugger::Internal {

bool AcceptsBreakpointQuery::isCppBreakpoint() const
{
    if (type == BreakpointAtJavaScriptThrow || type == BreakpointOnQmlSignalEmit)
        return false;
    if (type == BreakpointByFileAndLine)
        return !isQmlFileAndLineBreakpoint();
    return true;
}

bool AcceptsBreakpointQuery::isQmlFileAndLineBreakpoint() const
{
    if (type != BreakpointByFileAndLine)
        return false;

    static const QStringList qmlFileExtensions = [] {
        QString qmlExtensionString = qtcEnvironmentVariable("QTC_QMLDEBUGGER_FILEEXTENSIONS");
        if (qmlExtensionString.isEmpty())
            qmlExtensionString = ".qml;.js;.mjs";
        return qmlExtensionString.split(';', Qt::SkipEmptyParts);
    }();

    const QString file = fileName.path();
    for (const QString &extension : qmlFileExtensions) {
        if (file.endsWith(extension, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

bool DebuggerEngineInterface::hasCapability(unsigned cap, DebuggerStartMode startMode) const
{
    if (startMode != AttachToCore)
        return m_setupData.capabilities & cap;
    QTC_CHECK(m_setupData.attachToCoreCapabilities != 0);
    return m_setupData.attachToCoreCapabilities & cap;
}

bool DebuggerEngineInterface::hasExtraCapability(DebuggerExtraCapability cap) const
{
    return m_setupData.extraCapabilities.testFlag(cap);
}
} // namespace Debugger::Internal

// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggerengineinterface.h"

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace Debugger::Internal {

bool AcceptsBreakpointQuery::isCppBreakpoint() const
{
    if (type == BreakpointAtJavaScriptThrow || type == BreakpointOnQmlSignalEmit)
        return false;
    // A native debugger cannot stop in the sources of a virtual machine language.
    if (type == BreakpointByFileAndLine)
        return !isQmlFileAndLineBreakpoint() && !isJvmSource(fileName);
    return true;
}

bool AcceptsBreakpointQuery::isVirtualMachineBreakpoint() const
{
    return Internal::isVirtualMachineBreakpoint(type, fileName, functionName, isJavaDebugging);
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

QString mappedSourcePath(const QList<QPair<QString, QString>> &sourcePathMap,
                         const QString &path, bool toLocal)
{
    if (path.isEmpty() || sourcePathMap.isEmpty())
        return path;
    const FilePath given = FilePath::fromUserInput(path);
    const QString givenPath = given.path();
    for (const QPair<QString, QString> &entry : sourcePathMap) {
        const FilePath from = FilePath::fromUserInput(toLocal ? entry.first : entry.second);
        const FilePath to = FilePath::fromUserInput(toLocal ? entry.second : entry.first);
        if (!given.isSameDevice(from))
            continue;
        // A build path from the debug information usually does not exist here, so
        // FilePath's comparison cannot fall back to the file system for the case.
        const QString fromPath = from.path();
        if (fromPath.isEmpty())
            continue;
        const bool isWindowsPath = fromPath.size() >= 2 && fromPath.at(1) == ':';
        const Qt::CaseSensitivity cs = isWindowsPath ? Qt::CaseInsensitive : Qt::CaseSensitive;
        if (givenPath.compare(fromPath, cs) == 0)
            return to.path();
        const QString prefix = fromPath.endsWith('/') ? fromPath : fromPath + '/';
        if (givenPath.startsWith(prefix, cs))
            return to.pathAppended(givenPath.mid(prefix.size())).path();
    }
    return path;
}
} // namespace Debugger::Internal

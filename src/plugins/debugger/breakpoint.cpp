/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "breakpoint.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerprotocol.h"

#include <projectexplorer/abi.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QFileInfo>
#include <QDir>

namespace Debugger {
namespace Internal {

/*!
    \class Debugger::Internal::BreakpointParameters

    Data type holding the parameters of a breakpoint.
*/

BreakpointParameters::BreakpointParameters(BreakpointType t)
  : type(t), enabled(true), pathUsage(BreakpointPathUsageEngineDefault),
    ignoreCount(0), lineNumber(0), address(0), size(0),
    bitpos(0), bitsize(0), threadSpec(-1),
    tracepoint(false), oneShot(false)
{}

BreakpointParts BreakpointParameters::differencesTo
    (const BreakpointParameters &rhs) const
{
    BreakpointParts parts = BreakpointParts();
    if (type != rhs.type)
        parts |= TypePart;
    if (enabled != rhs.enabled)
        parts |= EnabledPart;
    if (pathUsage != rhs.pathUsage)
        parts |= PathUsagePart;
    if (fileName != rhs.fileName)
        parts |= FileAndLinePart;
    if (!conditionsMatch(rhs.condition))
        parts |= ConditionPart;
    if (ignoreCount != rhs.ignoreCount)
        parts |= IgnoreCountPart;
    if (lineNumber != rhs.lineNumber)
        parts |= FileAndLinePart;
    if (address != rhs.address)
        parts |= AddressPart;
    if (threadSpec != rhs.threadSpec)
        parts |= ThreadSpecPart;
    if (functionName != rhs.functionName)
        parts |= FunctionPart;
    if (tracepoint != rhs.tracepoint)
        parts |= TracePointPart;
    if (module != rhs.module)
        parts |= ModulePart;
    if (command != rhs.command)
        parts |= CommandPart;
    if (message != rhs.message)
        parts |= MessagePart;
    if (oneShot != rhs.oneShot)
        parts |= OneShotPart;
    return parts;
}

bool BreakpointParameters::isValid() const
{
    switch (type) {
    case BreakpointByFileAndLine:
        return !fileName.isEmpty() && lineNumber > 0;
    case BreakpointByFunction:
        return !functionName.isEmpty();
    case WatchpointAtAddress:
    case BreakpointByAddress:
        return address != 0;
    case BreakpointAtThrow:
    case BreakpointAtCatch:
    case BreakpointAtMain:
    case BreakpointAtFork:
    case BreakpointAtExec:
    case BreakpointAtSysCall:
    case BreakpointOnQmlSignalEmit:
    case BreakpointAtJavaScriptThrow:
        break;
    case WatchpointAtExpression:
        return !expression.isEmpty();
    case UnknownBreakpointType:
    case LastBreakpointType:
        return false;
    }
    return true;
}

bool BreakpointParameters::equals(const BreakpointParameters &rhs) const
{
    return !differencesTo(rhs);
}

bool BreakpointParameters::conditionsMatch(const QString &other) const
{
    // Some versions of gdb "beautify" the passed condition.
    QString s1 = condition;
    s1.replace(' ', "");
    QString s2 = other;
    s2.replace(' ', "");
    return s1 == s2;
}

void BreakpointParameters::updateLocation(const QString &location)
{
    if (location.size()) {
        int pos = location.indexOf(':');
        lineNumber = location.mid(pos + 1).toInt();
        QString file = location.left(pos);
        if (file.startsWith('"') && file.endsWith('"'))
            file = file.mid(1, file.size() - 2);
        QFileInfo fi(file);
        if (fi.isReadable())
            fileName = fi.absoluteFilePath();
    }
}

bool BreakpointParameters::isQmlFileAndLineBreakpoint() const
{
    if (type != BreakpointByFileAndLine)
        return false;

    QString qmlExtensionString = QString::fromLocal8Bit(qgetenv("QTC_QMLDEBUGGER_FILEEXTENSIONS"));
    if (qmlExtensionString.isEmpty())
        qmlExtensionString = ".qml;.js";

    const auto qmlFileExtensions = qmlExtensionString.splitRef(';', QString::SkipEmptyParts);
    for (QStringRef extension : qmlFileExtensions) {
        if (fileName.endsWith(extension, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

bool BreakpointParameters::isCppBreakpoint() const
{
    // Qml specific breakpoint types.
    if (type == BreakpointAtJavaScriptThrow || type == BreakpointOnQmlSignalEmit)
        return false;

    // Qml is currently only file.
    if (type == BreakpointByFileAndLine)
        return !isQmlFileAndLineBreakpoint();

    return true;
}

QString BreakpointParameters::toString() const
{
    QString result;
    QTextStream ts(&result);
    ts << "Type: " << type;
    switch (type) {
    case BreakpointByFileAndLine:
        ts << " FileName: " << fileName << ':' << lineNumber
           << " PathUsage: " << pathUsage;
        break;
    case BreakpointByFunction:
    case BreakpointOnQmlSignalEmit:
        ts << " FunctionName: " << functionName;
        break;
    case BreakpointByAddress:
    case WatchpointAtAddress:
        ts << " Address: " << address;
        break;
    case WatchpointAtExpression:
        ts << " Expression: " << expression;
        break;
    case BreakpointAtThrow:
    case BreakpointAtCatch:
    case BreakpointAtMain:
    case BreakpointAtFork:
    case BreakpointAtExec:
    case BreakpointAtSysCall:
    case BreakpointAtJavaScriptThrow:
    case UnknownBreakpointType:
    case LastBreakpointType:
        break;
    }
    ts << (enabled ? " [enabled]" : " [disabled]");
    if (!condition.isEmpty())
        ts << " Condition: " << condition;
    if (ignoreCount)
        ts << " IgnoreCount: " << ignoreCount;
    if (tracepoint)
        ts << " [tracepoint]";
    if (!module.isEmpty())
        ts << " Module: " << module;
    if (!command.isEmpty())
        ts << " Command: " << command;
    if (!message.isEmpty())
        ts << " Message: " << message;

    if (pending)
        ts << " [pending]";
    if (!functionName.isEmpty())
        ts << " Function: " << functionName;
    ts << " Hit: " << hitCount << " times";
    ts << ' ';

    return result;
}

static QString cleanupFullName(const QString &fileName)
{
    QString cleanFilePath = fileName;

    // Gdb running on windows often delivers "fullnames" which
    // (a) have no drive letter and (b) are not normalized.
    if (ProjectExplorer::Abi::hostAbi().os() == ProjectExplorer::Abi::WindowsOS) {
        if (fileName.isEmpty())
            return QString();
        QFileInfo fi(fileName);
        if (fi.isReadable())
            cleanFilePath = QDir::cleanPath(fi.absoluteFilePath());
    }

//    if (!boolSetting(AutoEnrichParameters))
//        return cleanFilePath;

//    const QString sysroot = runParameters().sysRoot;
//    if (QFileInfo(cleanFilePath).isReadable())
//        return cleanFilePath;
//    if (!sysroot.isEmpty() && fileName.startsWith('/')) {
//        cleanFilePath = sysroot + fileName;
//        if (QFileInfo(cleanFilePath).isReadable())
//            return cleanFilePath;
//    }
//    if (m_baseNameToFullName.isEmpty()) {
//        QString debugSource = sysroot + "/usr/src/debug";
//        if (QFileInfo(debugSource).isDir()) {
//            QDirIterator it(debugSource, QDirIterator::Subdirectories);
//            while (it.hasNext()) {
//                it.next();
//                QString name = it.fileName();
//                if (!name.startsWith('.')) {
//                    QString path = it.filePath();
//                    m_baseNameToFullName.insert(name, path);
//                }
//            }
//        }
//    }

//    cleanFilePath.clear();
//    const QString base = FileName::fromString(fileName).fileName();

//    QMap<QString, QString>::const_iterator jt = m_baseNameToFullName.constFind(base);
//    while (jt != m_baseNameToFullName.constEnd() && jt.key() == base) {
//        // FIXME: Use some heuristics to find the "best" match.
//        return jt.value();
//        //++jt;
//    }

    return cleanFilePath;
}
void BreakpointParameters::updateFromGdbOutput(const GdbMi &bkpt)
{
    QTC_ASSERT(bkpt.isValid(), return);

    QString originalLocation;
    QString file;
    QString fullName;
    QString internalId;

    enabled = true;
    pending = false;
    condition.clear();
    for (const GdbMi &child : bkpt) {
        if (child.hasName("number")) {
            // Handled on caller side.
        } else if (child.hasName("func")) {
            functionName = child.data();
        } else if (child.hasName("addr")) {
            // <MULTIPLE> happens in constructors, inline functions, and
            // at other places like 'foreach' lines. In this case there are
            // fields named "addr" in the response and/or the address
            // is called <MULTIPLE>.
            //qDebug() << "ADDR: " << child.data() << (child.data() == "<MULTIPLE>");
            if (child.data().startsWith("0x"))
                address = child.toAddress();
        } else if (child.hasName("file")) {
            file = child.data();
        } else if (child.hasName("fullname")) {
            fullName = child.data();
        } else if (child.hasName("line")) {
            lineNumber = child.toInt();
        } else if (child.hasName("cond")) {
            // gdb 6.3 likes to "rewrite" conditions. Just accept that fact.
            condition = child.data();
        } else if (child.hasName("enabled")) {
            enabled = (child.data() == "y");
        } else if (child.hasName("disp")) {
            oneShot = child.data() == "del";
        } else if (child.hasName("pending")) {
            // Any content here would be interesting only if we did accept
            // spontaneously appearing breakpoints (user using gdb commands).
            if (file.isEmpty())
                file = child.data();
            pending = true;
        } else if (child.hasName("at")) {
            // Happens with gdb 6.4 symbianelf.
            QString ba = child.data();
            if (ba.startsWith('<') && ba.endsWith('>'))
                ba = ba.mid(1, ba.size() - 2);
            functionName = ba;
        } else if (child.hasName("thread")) {
            threadSpec = child.toInt();
        } else if (child.hasName("type")) {
            // "breakpoint", "hw breakpoint", "tracepoint", "hw watchpoint"
            // {bkpt={number="2",type="hw watchpoint",disp="keep",enabled="y",
            //  what="*0xbfffed48",times="0",original-location="*0xbfffed48"}}
            if (child.data().contains("tracepoint")) {
                tracepoint = true;
            } else if (child.data() == "hw watchpoint" || child.data() == "watchpoint") {
                QString what = bkpt["what"].data();
                if (what.startsWith("*0x")) {
                    type = WatchpointAtAddress;
                    address = what.mid(1).toULongLong(0, 0);
                } else {
                    type = WatchpointAtExpression;
                    expression = what;
                }
            } else if (child.data() == "breakpoint") {
                QString catchType = bkpt["catch-type"].data();
                if (catchType == "throw")
                    type = BreakpointAtThrow;
                else if (catchType == "catch")
                    type = BreakpointAtCatch;
                else if (catchType == "fork")
                    type = BreakpointAtFork;
                else if (catchType == "exec")
                    type = BreakpointAtExec;
                else if (catchType == "syscall")
                    type = BreakpointAtSysCall;
            }
        } else if (child.hasName("times")) {
            hitCount = child.toInt();
        } else if (child.hasName("original-location")) {
            originalLocation = child.data();
        }
        // This field is not present.  Contents needs to be parsed from
        // the plain "ignore"
        //else if (child.hasName("ignore"))
        //    ignoreCount = child.data();
    }

    QString name;
    if (!fullName.isEmpty()) {
        name = cleanupFullName(fullName);
        fileName = name;
        //if (data->markerFileName().isEmpty())
        //    data->setMarkerFileName(name);
    } else {
        name = file;
        // Use fullName() once we have a mapping which is more complete than
        // gdb's own. No point in assigning markerFileName for now.
    }
    if (!name.isEmpty())
        fileName = name;

    if (fileName.isEmpty())
        updateLocation(originalLocation);
}

} // namespace Internal
} // namespace Debugger

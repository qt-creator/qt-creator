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

#include <utils/qtcassert.h>

#include <QDebug>
#include <QFileInfo>

namespace Debugger {
namespace Internal {

/*!
    \class Debugger::Internal::BreakpointIdBase

    Convenience base class for BreakpointModelId and
    BreakpointResponseId.
*/

QDebug operator<<(QDebug d, const BreakpointIdBase &id)
{
    d << qPrintable(id.toString());
    return d;
}

QString BreakpointIdBase::toString() const
{
    if (!isValid())
        return QLatin1String("<invalid bkpt>");
    if (isMinor())
        return QString("%1.%2").arg(m_majorPart).arg(m_minorPart);
    return QString::number(m_majorPart);
}


/*!
    \class Debugger::Internal::BreakpointModelId

    This identifies a breakpoint in the \c BreakHandler. The
    major parts are strictly increasing over time.

    The minor part identifies a multiple breakpoint
    set for example by gdb in constructors.
*/


BreakpointModelId::BreakpointModelId(const QString &ba)
{
    int pos = ba.indexOf('\'');
    if (pos == -1) {
        m_majorPart = ba.toUShort();
        m_minorPart = 0;
    } else {
        m_majorPart = ba.left(pos).toUShort();
        m_minorPart = ba.mid(pos + 1).toUShort();
    }
}

/*!
    \class Debugger::Internal::BreakpointResponseId

    This is what the external debuggers use to identify a breakpoint.
    It is only valid for one debugger run.

    In gdb, the breakpoint number is used, which is constant
    during a session. CDB's breakpoint numbers vary if breakpoints
    are deleted, so, the ID is used.
*/

BreakpointResponseId::BreakpointResponseId(const QString &ba)
{
    int pos = ba.indexOf('.');
    if (pos == -1) {
        m_majorPart = ba.toInt();
        m_minorPart = 0;
    } else {
        m_majorPart = ba.left(pos).toInt();
        m_minorPart = ba.mid(pos + 1).toInt();
    }
}

//////////////////////////////////////////////////////////////////
//
// BreakpointParameters
//
//////////////////////////////////////////////////////////////////

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

bool BreakpointParameters::isCppBreakpoint() const
{
    // Qml specific breakpoint types.
    if (type == BreakpointAtJavaScriptThrow
            || type == BreakpointOnQmlSignalEmit)
        return false;

    // Qml is currently only file.
    if (type == BreakpointByFileAndLine) {
        auto qmlExtensionString = QString::fromLocal8Bit(qgetenv("QTC_QMLDEBUGGER_FILEEXTENSIONS"));
        if (qmlExtensionString.isEmpty())
            qmlExtensionString = ".qml;.js";

        auto qmlFileExtensions = qmlExtensionString.splitRef(QLatin1Char(';'), QString::SkipEmptyParts);
        foreach (QStringRef extension, qmlFileExtensions) {
            if (fileName.endsWith(extension, Qt::CaseInsensitive))
                return false;
        }
    }

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
    return result;
}

//////////////////////////////////////////////////////////////////
//
// BreakpointResponse
//
//////////////////////////////////////////////////////////////////

/*!
    \class Debugger::Internal::BreakpointResponse

    This is what debuggers produce in response to the attempt to
    insert a breakpoint. The data might differ from the requested bits.
*/

BreakpointResponse::BreakpointResponse()
{
    pending = true;
    hitCount = 0;
    multiple = false;
    correctedLineNumber = 0;
}

QString BreakpointResponse::toString() const
{
    QString result = BreakpointParameters::toString();
    QTextStream ts(&result);
    ts << " Number: " << id.toString();
    if (pending)
        ts << " [pending]";
    if (!functionName.isEmpty())
        ts << " Function: " << functionName;
    if (multiple)
        ts << " Multiple: " << multiple;
    if (correctedLineNumber)
        ts << " CorrectedLineNumber: " << correctedLineNumber;
    ts << " Hit: " << hitCount << " times";
    ts << ' ';
    return result + BreakpointParameters::toString();
}

void BreakpointResponse::fromParameters(const BreakpointParameters &p)
{
    BreakpointParameters::operator=(p);
    id = BreakpointResponseId();
    multiple = false;
    correctedLineNumber = 0;
    hitCount = 0;
}

} // namespace Internal
} // namespace Debugger

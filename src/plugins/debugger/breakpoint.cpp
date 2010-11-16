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

#include "breakpoint.h"

#include <QtCore/QByteArray>
#include <QtCore/QDebug>

namespace Debugger {
namespace Internal {

//////////////////////////////////////////////////////////////////
//
// BreakpointData
//
//////////////////////////////////////////////////////////////////

BreakpointParameters::BreakpointParameters(BreakpointType t) :
    type(t), enabled(true), useFullPath(false),
    ignoreCount(0), lineNumber(0), address(0)
{
}

bool BreakpointParameters::equals(const BreakpointParameters &rhs) const
{
    return type == rhs.type && enabled == rhs.enabled
            && useFullPath == rhs.useFullPath
            && fileName == rhs.fileName && condition == rhs.condition
            && ignoreCount == rhs.ignoreCount && lineNumber == rhs.lineNumber
            && address == rhs.address && threadSpec == rhs.threadSpec
            && functionName == rhs.functionName;
}

BreakpointData::BreakpointData(BreakpointType type)
    : m_parameters(type)
{}

BreakpointResponse::BreakpointResponse()
    : number(0), multiple(false)
{}

#define SETIT(var, value) return (var != value) && (var = value, true)

bool BreakpointData::setUseFullPath(bool on)
{
    SETIT(m_parameters.useFullPath, on);
}

bool BreakpointData::setFileName(const QString &file)
{
    SETIT(m_parameters.fileName, file);
}

bool BreakpointData::setEnabled(bool on)
{
    SETIT(m_parameters.enabled, on);
}

bool BreakpointData::setIgnoreCount(int count)
{
    SETIT(m_parameters.ignoreCount, count);
}

bool BreakpointData::setFunctionName(const QString &name)
{
    SETIT(m_parameters.functionName, name);
}

bool BreakpointData::setLineNumber(int line)
{
    SETIT(m_parameters.lineNumber, line);
}

bool BreakpointData::setAddress(quint64 address)
{
    SETIT(m_parameters.address, address);
}

bool BreakpointData::setThreadSpec(const QByteArray &spec)
{
    SETIT(m_parameters.threadSpec, spec);
}

bool BreakpointData::setType(BreakpointType type)
{
    SETIT(m_parameters.type, type);
}

bool BreakpointData::setCondition(const QByteArray &cond)
{
    SETIT(m_parameters.condition, cond);
}

#undef SETIT

bool BreakpointParameters::conditionsMatch(const QByteArray &other) const
{
    // Some versions of gdb "beautify" the passed condition.
    QByteArray s1 = condition;
    s1.replace(' ', "");
    QByteArray s2 = other;
    s2.replace(' ', "");
    return s1 == s2;
}

QString BreakpointData::toString() const
{
    QString result;
    QTextStream ts(&result);
    ts << fileName();
    ts << condition();
    ts << ignoreCount();
    ts << lineNumber();
    ts << address();
    ts << functionName();
    ts << useFullPath();
    return result;
}

QString BreakpointResponse::toString() const
{
    QString result;
    QTextStream ts(&result);
    ts << number;
    ts << condition;
    ts << ignoreCount;
    ts << fileName;
    ts << fullName;
    ts << lineNumber;
    ts << threadSpec;
    ts << functionName;
    ts << address;
    return result;
}

void BreakpointResponse::fromParameters(const BreakpointParameters &p)
{
    BreakpointParameters::operator=(p);
    number = 0;
    fullName.clear();
    multiple = false;
    state.clear();
}

} // namespace Internal
} // namespace Debugger

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
// BreakpointParameters
//
//////////////////////////////////////////////////////////////////

BreakpointParameters::BreakpointParameters(BreakpointType t)
  : type(t), enabled(true), useFullPath(false),
    ignoreCount(0), lineNumber(0), address(0), threadSpec(0)
{}

bool BreakpointParameters::equals(const BreakpointParameters &rhs) const
{
    return type == rhs.type
        && enabled == rhs.enabled
        && useFullPath == rhs.useFullPath
        && fileName == rhs.fileName
        && conditionsMatch(rhs.condition)
        && ignoreCount == rhs.ignoreCount
        && lineNumber == rhs.lineNumber
        && address == rhs.address
        && threadSpec == rhs.threadSpec
        && functionName == rhs.functionName;
}

bool BreakpointParameters::conditionsMatch(const QByteArray &other) const
{
    // Some versions of gdb "beautify" the passed condition.
    QByteArray s1 = condition;
    s1.replace(' ', "");
    QByteArray s2 = other;
    s2.replace(' ', "");
    return s1 == s2;
}

QString BreakpointParameters::toString() const
{
    QString result;
    QTextStream ts(&result);
    ts << " FileName: " << fileName;
    ts << " Condition: " << condition;
    ts << " IgnoreCount: " << ignoreCount;
    ts << " LineNumber: " << lineNumber;
    ts << " Address: " << address;
    ts << " FunctionName: " << functionName;
    ts << " UseFullPath: " << useFullPath;
    return result;
}


//////////////////////////////////////////////////////////////////
//
// BreakpointParameters
//
//////////////////////////////////////////////////////////////////

BreakpointResponse::BreakpointResponse()
    : number(0), pending(true), multiple(false), correctedLineNumber(0)
{}

QString BreakpointResponse::toString() const
{
    QString result;
    QTextStream ts(&result);
    ts << " Number: " << number;
    ts << " Pending: " << pending;
    ts << " FullName: " << fullName;
    ts << " Multiple: " << multiple;
    ts << " Extra: " << extra;
    ts << " CorrectedLineNumber: " << correctedLineNumber;
    return result + BreakpointParameters::toString();
}

void BreakpointResponse::fromParameters(const BreakpointParameters &p)
{
    BreakpointParameters::operator=(p);
    number = 0;
    fullName.clear();
    multiple = false;
    extra.clear();
    correctedLineNumber = 0;
}

} // namespace Internal
} // namespace Debugger

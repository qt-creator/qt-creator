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
#include "stackframe.h"

#include <utils/qtcassert.h>

#include <QtCore/QByteArray>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

namespace Debugger {
namespace Internal {

//////////////////////////////////////////////////////////////////
//
// BreakpointData
//
//////////////////////////////////////////////////////////////////

const char *BreakpointData::throwFunction = "throw";
const char *BreakpointData::catchFunction = "catch";

static quint64 nextBPId()
{
    // Ok to be not thread-safe. The order does not matter and only the gui
    // produces authoritative ids.
    static quint64 i = 0;
    return ++i;
}

BreakpointData::BreakpointData()
{
    m_state = BreakpointNew;
    m_engine = 0;
    m_enabled = true;
    m_type = BreakpointByFileAndLine;
    m_ignoreCount = 0;
    m_lineNumber = 0;
    m_address = 0;
    m_useFullPath = false;
    m_markerLineNumber = 0;
}

BreakpointResponse::BreakpointResponse()
{
    bpNumber = 0;
    bpIgnoreCount = 0;
    bpLineNumber = 0;
    //bpCorrectedLineNumber = 0;
    bpAddress = 0;
    bpMultiple = false;
    bpEnabled = true;
}

#define SETIT(var, value) return (var != value) && (var = value, true)

bool BreakpointData::setUseFullPath(bool on)
{ SETIT(m_useFullPath, on); }

bool BreakpointData::setMarkerFileName(const QString &file)
{ SETIT(m_markerFileName, file); }

bool BreakpointData::setMarkerLineNumber(int line)
{ SETIT(m_markerLineNumber, line); }

bool BreakpointData::setFileName(const QString &file)
{ SETIT(m_fileName, file); }

bool BreakpointData::setEnabled(bool on)
{ SETIT(m_enabled, on); }

bool BreakpointData::setIgnoreCount(bool count)
{ SETIT(m_ignoreCount, count); }

bool BreakpointData::setFunctionName(const QString &name)
{ SETIT(m_functionName, name); }

bool BreakpointData::setLineNumber(int line)
{ SETIT(m_lineNumber, line); }

bool BreakpointData::setAddress(quint64 address)
{ SETIT(m_address, address); }

bool BreakpointData::setThreadSpec(const QByteArray &spec)
{ SETIT(m_threadSpec, spec); }

bool BreakpointData::setType(BreakpointType type)
{ SETIT(m_type, type); }

bool BreakpointData::setCondition(const QByteArray &cond)
{ SETIT(m_condition, cond); }

bool BreakpointData::setState(BreakpointState state)
{ SETIT(m_state, state); }

bool BreakpointData::setEngine(DebuggerEngine *engine)
{ SETIT(m_engine, engine); }

#undef SETIT


static void formatAddress(QTextStream &str, quint64 address)
{
    if (address) {
        str << "0x";
        str.setIntegerBase(16);
        str << address;
        str.setIntegerBase(10);
    }
}

QString BreakpointData::toToolTip() const
{
    QString t;
    switch (m_type) {
        case BreakpointByFileAndLine:
            t = tr("Breakpoint by File and Line");
            break;
        case BreakpointByFunction:
            t = tr("Breakpoint by Function");
            break;
        case BreakpointByAddress:
            t = tr("Breakpoint by Address");
            break;
        case Watchpoint:
            t = tr("Watchpoint");
            break;
        case UnknownType:
            t = tr("Unknown Breakpoint Type");
    }

    QString rc;
    QTextStream str(&rc);
    str << "<html><body><table>"
        //<< "<tr><td>" << tr("Id:")
        //<< "</td><td>" << m_id << "</td></tr>"
        << "<tr><td>" << tr("State:")
        << "</td><td>" << m_state << "</td></tr>"
        << "<tr><td>" << tr("Engine:")
        << "</td><td>" << m_engine << "</td></tr>"
        << "<tr><td>" << tr("Marker File:")
        << "</td><td>" << QDir::toNativeSeparators(m_markerFileName) << "</td></tr>"
        << "<tr><td>" << tr("Marker Line:")
        << "</td><td>" << m_markerLineNumber << "</td></tr>"
        //<< "<tr><td>" << tr("Breakpoint Number:")
        //<< "</td><td>" << bpNumber << "</td></tr>"
        << "<tr><td>" << tr("Breakpoint Type:")
        << "</td><td>" << t << "</td></tr>"
        << "<tr><td>" << tr("State:")
        //<< "</td><td>" << bpState << "</td></tr>"
        << "</table><br><hr><table>"
        << "<tr><th>" << tr("Property")
        << "</th><th>" << tr("Requested")
        << "</th><th>" << tr("Obtained") << "</th></tr>"
        << "<tr><td>" << tr("Internal Number:")
        //<< "</td><td>&mdash;</td><td>" << bpNumber << "</td></tr>"
        << "<tr><td>" << tr("File Name:")
        << "</td><td>" << QDir::toNativeSeparators(m_fileName)
        //<< "</td><td>" << QDir::toNativeSeparators(bpFileName) << "</td></tr>"
        << "<tr><td>" << tr("Function Name:")
        << "</td><td>" << m_functionName // << "</td><td>" << bpFuncName << "</td></tr>"
        << "<tr><td>" << tr("Line Number:") << "</td><td>";
    if (m_lineNumber)
        str << m_lineNumber;
    //str << "</td><td>";
    //if (bpLineNumber)
    //    str << bpLineNumber;
    str << "</td></tr>"
        << "<tr><td>" << tr("Breakpoint Address:")
        << "</td><td>";
    formatAddress(str, m_address);
    str << "</td><td>";
    //formatAddress(str, bpAddress);
    //str <<  "</td></tr>"
    //    << "<tr><td>" << tr("Corrected Line Number:")
    //    << "</td><td>-</td><td>";
    //if (bpCorrectedLineNumber > 0) {
    //    str << bpCorrectedLineNumber;
   // } else {
   //     str << '-';
   // }
    str << "</td></tr>"
        << "<tr><td>" << tr("Condition:")
    //    << "</td><td>" << m_condition << "</td><td>" << bpCondition << "</td></tr>"
        << "<tr><td>" << tr("Ignore Count:") << "</td><td>";
    if (m_ignoreCount)
        str << m_ignoreCount;
    str << "</td><td>";
    //if (bpIgnoreCount)
    //    str << bpIgnoreCount;
    str << "</td></tr>"
        << "<tr><td>" << tr("Thread Specification:")
     //   << "</td><td>" << m_threadSpec << "</td><td>" << bpThreadSpec << "</td></tr>"
        << "</table></body></html>";
    return rc;
}

// Compare file names case insensitively on Windows.
static inline bool fileNameMatch(const QString &f1, const QString &f2)
{
#ifdef Q_OS_WIN
    return f1.compare(f2, Qt::CaseInsensitive) == 0;
#else
    return f1 == f2;
#endif
}

bool BreakpointData::isLocatedAt(const QString &fileName, int lineNumber, 
    bool useMarkerPosition) const
{
    int line = useMarkerPosition ? m_markerLineNumber : m_lineNumber;
    return lineNumber == line && fileNameMatch(fileName, m_markerFileName);
}

bool BreakpointData::conditionsMatch(const QString &other) const
{
    // Some versions of gdb "beautify" the passed condition.
    QString s1 = m_condition;
    s1.remove(QChar(' '));
    QString s2 = other;
    s2.remove(QChar(' '));
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
    ts << bpNumber;
    ts << bpCondition;
    ts << bpIgnoreCount;
    ts << bpFileName;
    ts << bpFullName;
    ts << bpLineNumber;
    ts << bpThreadSpec;
    ts << bpFuncName;
    ts << bpAddress;
    return result;
}

} // namespace Internal
} // namespace Debugger

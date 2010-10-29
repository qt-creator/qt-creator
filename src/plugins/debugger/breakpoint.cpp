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

static quint64 nextBPId() {
    /* ok to be not thread-safe
       the order does not matter and only the gui
       produces authoritative ids. well, for now...
    */
    static quint64 i=1;
    return ++i;
}

BreakpointData::BreakpointData() :
    id(nextBPId()), enabled(true),
    pending(true), type(BreakpointType),
    ignoreCount(0), lineNumber(0), address(0),
    useFullPath(false),
    bpIgnoreCount(0), bpLineNumber(0),
    bpCorrectedLineNumber(0), bpAddress(0),
    bpMultiple(false), bpEnabled(true),
    m_markerLineNumber(0)
{
}

BreakpointData *BreakpointData::clone() const
{
    BreakpointData *data = new BreakpointData();
    data->enabled = enabled;
    data->type = type;
    data->fileName = fileName;
    data->condition = condition;
    data->ignoreCount = ignoreCount;
    data->lineNumber = lineNumber;
    data->address = address;
    data->threadSpec = threadSpec;
    data->funcName = funcName;
    data->useFullPath = useFullPath;
    if (isSetByFunction()) {
        // FIXME: Would removing it be better then leaving this 
        // "history" around?
        data->m_markerFileName = m_markerFileName;
        data->m_markerLineNumber = m_markerLineNumber;
    } else {
        data->m_markerFileName = fileName;
        data->m_markerLineNumber = lineNumber;
    }
    return data;
}

BreakpointData::~BreakpointData()
{
}

void BreakpointData::clear()
{
    pending = true;
    bpNumber.clear();
    bpCondition.clear();
    bpIgnoreCount = 0;
    bpFileName.clear();
    bpFullName.clear();
    bpLineNumber = 0;
    bpCorrectedLineNumber = 0;
    bpThreadSpec.clear();
    bpFuncName.clear();
    bpAddress = 0;
    bpMultiple = false;
    bpEnabled = true;
    bpState.clear();
    m_markerFileName = fileName;
    m_markerLineNumber = lineNumber;
}

void BreakpointData::setMarkerFileName(const QString &fileName)
{
    m_markerFileName = fileName;
}

void BreakpointData::setMarkerLineNumber(int lineNumber)
{
    m_markerLineNumber = lineNumber;
}

static inline void formatAddress(QTextStream &str, quint64 address)
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
    QString rc;
    QTextStream str(&rc);
    str << "<html><body><table>"
        << "<tr><td>" << tr("Marker File:")
        << "</td><td>" << QDir::toNativeSeparators(m_markerFileName) << "</td></tr>"
        << "<tr><td>" << tr("Marker Line:")
        << "</td><td>" << m_markerLineNumber << "</td></tr>"
        << "<tr><td>" << tr("Breakpoint Number:")
        << "</td><td>" << bpNumber << "</td></tr>"
        << "<tr><td>" << tr("Breakpoint Type:")
        << "</td><td>"
        << (type == BreakpointType ? tr("Breakpoint")
            : type == WatchpointType ? tr("Watchpoint")
            : tr("Unknown breakpoint type"))
        << "</td></tr>"
        << "<tr><td>" << tr("State:")
        << "</td><td>" << bpState << "</td></tr>"
        << "</table><br><hr><table>"
        << "<tr><th>" << tr("Property")
        << "</th><th>" << tr("Requested")
        << "</th><th>" << tr("Obtained") << "</th></tr>"
        << "<tr><td>" << tr("Internal Number:")
        << "</td><td>&mdash;</td><td>" << bpNumber << "</td></tr>"
        << "<tr><td>" << tr("File Name:")
        << "</td><td>" << QDir::toNativeSeparators(fileName) << "</td><td>" << QDir::toNativeSeparators(bpFileName) << "</td></tr>"
        << "<tr><td>" << tr("Function Name:")
        << "</td><td>" << funcName << "</td><td>" << bpFuncName << "</td></tr>"
        << "<tr><td>" << tr("Line Number:") << "</td><td>";
    if (lineNumber)
        str << lineNumber;
    str << "</td><td>";
    if (bpLineNumber)
        str << bpLineNumber;
    str << "</td></tr>"
        << "<tr><td>" << tr("Breakpoint Address:")
        << "</td><td>";
    formatAddress(str, address);
    str << "</td><td>";
    formatAddress(str, bpAddress);
    str <<  "</td></tr>"
        << "<tr><td>" << tr("Corrected Line Number:")
        << "</td><td>-</td><td>";
    if (bpCorrectedLineNumber > 0) {
        str << bpCorrectedLineNumber;
    } else {
        str << '-';
    }
    str << "</td></tr>"
        << "<tr><td>" << tr("Condition:")
        << "</td><td>" << condition << "</td><td>" << bpCondition << "</td></tr>"
        << "<tr><td>" << tr("Ignore Count:") << "</td><td>";
    if (ignoreCount)
        str << ignoreCount;
    str << "</td><td>";
    if (bpIgnoreCount)
        str << bpIgnoreCount;
    str << "</td></tr>"
        << "<tr><td>" << tr("Thread Specification:")
        << "</td><td>" << threadSpec << "</td><td>" << bpThreadSpec << "</td></tr>"
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

bool BreakpointData::isLocatedAt(const QString &fileName_, int lineNumber_, 
    bool useMarkerPosition) const
{
    int line = useMarkerPosition ? m_markerLineNumber : lineNumber;
    return lineNumber_ == line && fileNameMatch(fileName_, m_markerFileName);
}

bool BreakpointData::isSimilarTo(const BreakpointData *needle) const
{
    //qDebug() << "COMPARING " << toString() << " WITH " << needle->toString();

    if (id == needle->id && id != 0)
        return true;

    // Clear hit.
    if (bpNumber == needle->bpNumber
            && !bpNumber.isEmpty()
            && bpNumber.toInt() != 0)
        return true;

    // Clear miss.
    if (type != needle->type)
        return false;

    // We have numbers, but they are different.
    if (!bpNumber.isEmpty() && !needle->bpNumber.isEmpty()
            && !bpNumber.startsWith(needle->bpNumber)
            && !needle->bpNumber.startsWith(bpNumber))
        return false;

    if (address && needle->address && address == needle->address)
        return true;

    // At least at a position we were looking for.
    // FIXME: breaks multiple breakpoints at the same location
    if (!fileName.isEmpty()
            && fileNameMatch(fileName, needle->fileName)
            && lineNumber == needle->lineNumber)
        return true;

    // At least at a position we were looking for.
    // FIXME: breaks multiple breakpoints at the same location
    if (!fileName.isEmpty()
            && fileNameMatch(fileName, needle->bpFileName)
            && lineNumber == needle->bpLineNumber)
        return true;

    return false;
}

bool BreakpointData::conditionsMatch() const
{
    // Some versions of gdb "beautify" the passed condition.
    QString s1 = condition;
    s1.remove(QChar(' '));
    QString s2 = bpCondition;
    s2.remove(QChar(' '));
    return s1 == s2;
}

} // namespace Internal
} // namespace Debugger


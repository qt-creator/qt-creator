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

#include "breakhandler.h"

#include "debuggeractions.h"
#include "debuggerengine.h"
#include "debuggerstringutils.h"
#include "threadshandler.h"
#include "stackhandler.h"
#include "stackframe.h"

#include <texteditor/basetextmark.h>
#include <utils/qtcassert.h>

#include <QtCore/QByteArray>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

//////////////////////////////////////////////////////////////////
//
// BreakpointMarker
//
//////////////////////////////////////////////////////////////////

// Compare file names case insensitively on Windows.
static inline bool fileNameMatch(const QString &f1, const QString &f2)
{
#ifdef Q_OS_WIN
    return f1.compare(f2, Qt::CaseInsensitive) == 0;
#else
    return f1 == f2;
#endif
}

namespace Debugger {
namespace Internal {

// The red blob on the left side in the cpp editor.
class BreakpointMarker : public TextEditor::BaseTextMark
{
    Q_OBJECT
public:
    BreakpointMarker(BreakpointData *data, const QString &fileName, int lineNumber)
      : BaseTextMark(fileName, lineNumber)
    {
        m_data = data;
        m_pending = true;
        m_enabled = true;
        //qDebug() << "CREATE MARKER " << fileName << lineNumber;
    }

    ~BreakpointMarker()
    {
        //qDebug() << "REMOVE MARKER ";
        m_data = 0;
    }

    QIcon icon() const
    {
        const BreakHandler *handler = m_data->handler();
        if (!handler->isActive())
            return handler->emptyIcon();
        if (!m_enabled)
            return handler->disabledBreakpointIcon();
        return m_pending ? handler->pendingBreakPointIcon() : handler->breakpointIcon();
    }

    void setPending(bool pending, bool enabled)
    {
        if (pending == m_pending && enabled == m_enabled)
            return;
        m_pending = pending;
        m_enabled = enabled;
        updateMarker();
    }

    void updateBlock(const QTextBlock &)
    {
        //qDebug() << "BREAKPOINT MARKER UPDATE BLOCK";
    }

    void removedFromEditor()
    {
        if (!m_data)
            return;

        BreakHandler *handler = m_data->handler();
        handler->removeBreakpoint(m_data);
        //handler->saveBreakpoints();
        handler->updateMarkers();
    }

    void updateLineNumber(int lineNumber)
    {
        if (!m_data)
            return;
        //if (m_data->markerLineNumber == lineNumber)
        //    return;
        if (m_data->markerLineNumber() != lineNumber) {
            m_data->setMarkerLineNumber(lineNumber);
            // FIXME: Should we tell gdb about the change?
            // Ignore it for now, as we would require re-compilation
            // and debugger re-start anyway.
            if (0 && m_data->bpLineNumber) {
                if (!m_data->bpNumber.trimmed().isEmpty()) {
                    m_data->pending = true;
                }
            }
        }
        // Ignore updates to the "real" line number while the debugger is
        // running, as this can be triggered by moving the breakpoint to
        // the next line that generated code.
        // FIXME: Do we need yet another data member?
        if (m_data->bpNumber.trimmed().isEmpty()) {
            m_data->lineNumber = lineNumber;
            m_data->handler()->updateMarkers();
        }
    }

private:
    BreakpointData *m_data;
    bool m_pending;
    bool m_enabled;
};


//////////////////////////////////////////////////////////////////
//
// BreakpointData
//
//////////////////////////////////////////////////////////////////

BreakpointData::BreakpointData() :
    m_handler(0), enabled(true),
    pending(true), type(BreakpointType),
    ignoreCount(0), lineNumber(0), address(0),
    useFullPath(false),
    bpIgnoreCount(0), bpLineNumber(0),
    bpCorrectedLineNumber(0), bpAddress(0),
    bpMultiple(false), bpEnabled(true),
    m_markerLineNumber(0), marker(0)
{
}

BreakpointData *BreakpointData::clone() const
{
    BreakpointData *data = new BreakpointData();
    data->m_handler = m_handler;
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
    removeMarker();
}

void BreakpointData::removeMarker()
{
    BreakpointMarker *m = marker;
    marker = 0;
    delete m;
}

void BreakpointData::updateMarker()
{
    if (marker && (m_markerFileName != marker->fileName()
            || m_markerLineNumber != marker->lineNumber()))
        removeMarker();

    if (!marker && !m_markerFileName.isEmpty() && m_markerLineNumber > 0)
        marker = new BreakpointMarker(this, m_markerFileName, m_markerLineNumber);

    if (marker)
        marker->setPending(pending, enabled);
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
        << "<tr><td>" << BreakHandler::tr("Marker File:")
        << "</td><td>" << QDir::toNativeSeparators(m_markerFileName) << "</td></tr>"
        << "<tr><td>" << BreakHandler::tr("Marker Line:")
        << "</td><td>" << m_markerLineNumber << "</td></tr>"
        << "<tr><td>" << BreakHandler::tr("Breakpoint Number:")
        << "</td><td>" << bpNumber << "</td></tr>"
        << "<tr><td>" << BreakHandler::tr("Breakpoint Type:")
        << "</td><td>"
        << (type == BreakpointType ? BreakHandler::tr("Breakpoint")
            : type == WatchpointType ? BreakHandler::tr("Watchpoint")
            : BreakHandler::tr("Unknown breakpoint type"))
        << "</td></tr>"
        << "<tr><td>" << BreakHandler::tr("State:")
        << "</td><td>" << bpState << "</td></tr>"
        << "</table><br><hr><table>"
        << "<tr><th>" << BreakHandler::tr("Property")
        << "</th><th>" << BreakHandler::tr("Requested")
        << "</th><th>" << BreakHandler::tr("Obtained") << "</th></tr>"
        << "<tr><td>" << BreakHandler::tr("Internal Number:")
        << "</td><td>&mdash;</td><td>" << bpNumber << "</td></tr>"
        << "<tr><td>" << BreakHandler::tr("File Name:")
        << "</td><td>" << fileName << "</td><td>" << QDir::toNativeSeparators(bpFileName) << "</td></tr>"
        << "<tr><td>" << BreakHandler::tr("Function Name:")
        << "</td><td>" << funcName << "</td><td>" << bpFuncName << "</td></tr>"
        << "<tr><td>" << BreakHandler::tr("Line Number:") << "</td><td>";
    if (lineNumber)
        str << lineNumber;
    str << "</td><td>";
    if (bpLineNumber)
        str << bpLineNumber;
    str << "</td></tr>"
        << "<tr><td>" << BreakHandler::tr("Breakpoint Address:")
        << "</td><td>";
    formatAddress(str, address);
    str << "</td><td>";
    formatAddress(str, bpAddress);
    str <<  "</td></tr>"
        << "<tr><td>" << BreakHandler::tr("Corrected Line Number:")
        << "</td><td>-</td><td>";
    if (bpCorrectedLineNumber > 0) {
        str << bpCorrectedLineNumber;
    } else {
        str << '-';
    }
    str << "</td></tr>"
        << "<tr><td>" << BreakHandler::tr("Condition:")
        << "</td><td>" << condition << "</td><td>" << bpCondition << "</td></tr>"
        << "<tr><td>" << BreakHandler::tr("Ignore Count:") << "</td><td>";
    if (ignoreCount)
        str << ignoreCount;
    str << "</td><td>";
    if (bpIgnoreCount)
        str << bpIgnoreCount;
    str << "</td></tr>"
        << "<tr><td>" << BreakHandler::tr("Thread Specification:")
        << "</td><td>" << threadSpec << "</td><td>" << bpThreadSpec << "</td></tr>"
        << "</table></body></html>";
    return rc;
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

#include "breakpoint.moc"

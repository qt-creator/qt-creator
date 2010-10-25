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

#include "breakpointmarker.h"
#include "breakhandler.h"

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


namespace Debugger {
namespace Internal {

BreakpointMarker::BreakpointMarker(BreakHandler *handler, BreakpointData *data, 
        const QString &fileName, int lineNumber)
    : BaseTextMark(fileName, lineNumber)
    , m_handler(handler)
    , m_data(data)
    , m_pending(true)
{
    //qDebug() << "CREATE MARKER " << fileName << lineNumber;
}

BreakpointMarker::~BreakpointMarker()
{
    //qDebug() << "REMOVE MARKER ";
    m_data = 0;
}

QIcon BreakpointMarker::icon() const
{
    if (!m_data->enabled)
        return m_handler->disabledBreakpointIcon();
    if (!m_handler->isActive())
        return m_handler->emptyIcon();
    return m_pending ? m_handler->pendingBreakPointIcon() : m_handler->breakpointIcon();
}

void BreakpointMarker::setPending(bool pending)
{
    if (pending == m_pending)
        return;
    m_pending = pending;
    updateMarker();
}

void BreakpointMarker::updateBlock(const QTextBlock &)
{
    //qDebug() << "BREAKPOINT MARKER UPDATE BLOCK";
}

void BreakpointMarker::removedFromEditor()
{
    if (!m_data)
        return;
    m_handler->removeBreakpoint(m_data);
    //handler->saveBreakpoints();
    m_handler->updateMarkers();
}

void BreakpointMarker::updateLineNumber(int lineNumber)
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
        m_handler->updateMarkers();
    }
}

} // namespace Internal
} // namespace Debugger


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

#include "qmloutputformatter.h"

#include <texteditor/basetexteditor.h>

#include <QtGui/QPlainTextEdit>

using namespace ProjectExplorer;
using namespace QmlProjectManager::Internal;

QmlOutputFormatter::QmlOutputFormatter(QObject *parent)
    : OutputFormatter(parent)
    , m_qmlError(QLatin1String("(file:///[^:]+:\\d+:\\d+):"))
    , m_linksActive(true)
    , m_mousePressed(false)
{
}

void QmlOutputFormatter::appendApplicationOutput(const QString &text, bool onStdErr)
{
    QTextCharFormat linkFormat;
    linkFormat.setForeground(plainTextEdit()->palette().link().color());
    linkFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    linkFormat.setAnchor(true);

    // Create links from QML errors (anything of the form "file:///...:[line]:[column]:")
    int index = 0;
    while (m_qmlError.indexIn(text, index) != -1) {
        const int matchPos = m_qmlError.pos(1);
        const QString leader = text.mid(index, matchPos - index);
        append(leader, onStdErr ? StdErrFormat : StdOutFormat);

        const QString matched = m_qmlError.cap(1);
        linkFormat.setAnchorHref(matched);
        append(matched, linkFormat);

        index = matchPos + m_qmlError.matchedLength() - 1;
    }
    append(text.mid(index), onStdErr ? StdErrFormat : StdOutFormat);
}

void QmlOutputFormatter::mousePressEvent(QMouseEvent * /*e*/)
{
    m_mousePressed = true;
}

void QmlOutputFormatter::mouseReleaseEvent(QMouseEvent *e)
{
    m_mousePressed = false;

    if (!m_linksActive) {
        // Mouse was released, activate links again
        m_linksActive = true;
        return;
    }

    const QString href = plainTextEdit()->anchorAt(e->pos());
    if (!href.isEmpty()) {
        QRegExp qmlErrorLink(QLatin1String("^file://(/[^:]+):(\\d+):(\\d+)"));

        if (qmlErrorLink.indexIn(href) != -1) {
            const QString fileName = qmlErrorLink.cap(1);
            const int line = qmlErrorLink.cap(2).toInt();
            const int column = qmlErrorLink.cap(3).toInt();
            TextEditor::BaseTextEditor::openEditorAt(fileName, line, column - 1);
        }
    }
}

void QmlOutputFormatter::mouseMoveEvent(QMouseEvent *e)
{
    // Cursor was dragged to make a selection, deactivate links
    if (m_mousePressed && plainTextEdit()->textCursor().hasSelection())
        m_linksActive = false;

    if (!m_linksActive || plainTextEdit()->anchorAt(e->pos()).isEmpty())
        plainTextEdit()->viewport()->setCursor(Qt::IBeamCursor);
    else
        plainTextEdit()->viewport()->setCursor(Qt::PointingHandCursor);
}

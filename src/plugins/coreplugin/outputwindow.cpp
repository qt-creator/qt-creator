/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "outputwindow.h"

#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"
#include "coreconstants.h"
#include "icore.h"

#include <utils/qtcassert.h>
#include <utils/outputformatter.h>

#include <QAction>
#include <QScrollBar>

using namespace Utils;

namespace Core {

/*******************/

OutputWindow::OutputWindow(Core::Context context, QWidget *parent)
    : QPlainTextEdit(parent)
    , m_formatter(0)
    , m_enforceNewline(false)
    , m_scrollToBottom(false)
    , m_linksActive(true)
    , m_mousePressed(false)
    , m_maxLineCount(100000)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //setCenterOnScroll(false);
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);
    setUndoRedoEnabled(false);

    m_outputWindowContext = new Core::IContext;
    m_outputWindowContext->setContext(context);
    m_outputWindowContext->setWidget(this);
    ICore::addContextObject(m_outputWindowContext);

    QAction *undoAction = new QAction(this);
    QAction *redoAction = new QAction(this);
    QAction *cutAction = new QAction(this);
    QAction *copyAction = new QAction(this);
    QAction *pasteAction = new QAction(this);
    QAction *selectAllAction = new QAction(this);

    ActionManager::registerAction(undoAction, Core::Constants::UNDO, context);
    ActionManager::registerAction(redoAction, Core::Constants::REDO, context);
    ActionManager::registerAction(cutAction, Core::Constants::CUT, context);
    ActionManager::registerAction(copyAction, Core::Constants::COPY, context);
    ActionManager::registerAction(pasteAction, Core::Constants::PASTE, context);
    ActionManager::registerAction(selectAllAction, Core::Constants::SELECTALL, context);

    connect(undoAction, SIGNAL(triggered()), this, SLOT(undo()));
    connect(redoAction, SIGNAL(triggered()), this, SLOT(redo()));
    connect(cutAction, SIGNAL(triggered()), this, SLOT(cut()));
    connect(copyAction, SIGNAL(triggered()), this, SLOT(copy()));
    connect(pasteAction, SIGNAL(triggered()), this, SLOT(paste()));
    connect(selectAllAction, SIGNAL(triggered()), this, SLOT(selectAll()));

    connect(this, SIGNAL(undoAvailable(bool)), undoAction, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(redoAvailable(bool)), redoAction, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(copyAvailable(bool)), cutAction, SLOT(setEnabled(bool)));  // OutputWindow never read-only
    connect(this, SIGNAL(copyAvailable(bool)), copyAction, SLOT(setEnabled(bool)));

    undoAction->setEnabled(false);
    redoAction->setEnabled(false);
    cutAction->setEnabled(false);
    copyAction->setEnabled(false);
}

OutputWindow::~OutputWindow()
{
    Core::ICore::removeContextObject(m_outputWindowContext);
    delete m_outputWindowContext;
}

void OutputWindow::mousePressEvent(QMouseEvent * e)
{
    m_mousePressed = true;
    QPlainTextEdit::mousePressEvent(e);
}

void OutputWindow::mouseReleaseEvent(QMouseEvent *e)
{
    m_mousePressed = false;

    if (m_linksActive) {
        const QString href = anchorAt(e->pos());
        if (m_formatter)
            m_formatter->handleLink(href);
    }

    // Mouse was released, activate links again
    m_linksActive = true;

    QPlainTextEdit::mouseReleaseEvent(e);
}

void OutputWindow::mouseMoveEvent(QMouseEvent *e)
{
    // Cursor was dragged to make a selection, deactivate links
    if (m_mousePressed && textCursor().hasSelection())
        m_linksActive = false;

    if (!m_linksActive || anchorAt(e->pos()).isEmpty())
        viewport()->setCursor(Qt::IBeamCursor);
    else
        viewport()->setCursor(Qt::PointingHandCursor);
    QPlainTextEdit::mouseMoveEvent(e);
}

void OutputWindow::resizeEvent(QResizeEvent *e)
{
    //Keep scrollbar at bottom of window while resizing, to ensure we keep scrolling
    //This can happen if window is resized while building, or if the horizontal scrollbar appears
    bool atBottom = isScrollbarAtBottom();
    QPlainTextEdit::resizeEvent(e);
    if (atBottom)
        scrollToBottom();
}

void OutputWindow::keyPressEvent(QKeyEvent *ev)
{
    QPlainTextEdit::keyPressEvent(ev);

    //Ensure we scroll also on Ctrl+Home or Ctrl+End
    if (ev->matches(QKeySequence::MoveToStartOfDocument))
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMinimum);
    else if (ev->matches(QKeySequence::MoveToEndOfDocument))
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
}

OutputFormatter *OutputWindow::formatter() const
{
    return m_formatter;
}

void OutputWindow::setFormatter(OutputFormatter *formatter)
{
    m_formatter = formatter;
    m_formatter->setPlainTextEdit(this);
}

void OutputWindow::showEvent(QShowEvent *e)
{
    QPlainTextEdit::showEvent(e);
    if (m_scrollToBottom)
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    m_scrollToBottom = false;
}

QString OutputWindow::doNewlineEnfocement(const QString &out)
{
    m_scrollToBottom = true;
    QString s = out;
    if (m_enforceNewline) {
        s.prepend(QLatin1Char('\n'));
        m_enforceNewline = false;
    }

    if (s.endsWith(QLatin1Char('\n'))) {
        m_enforceNewline = true; // make appendOutputInline put in a newline next time
        s.chop(1);
    }

    return s;
}

void OutputWindow::setMaxLineCount(int count)
{
    m_maxLineCount = count;
    setMaximumBlockCount(m_maxLineCount);
}

void OutputWindow::appendMessage(const QString &output, OutputFormat format)
{
    QString out = output;
    out.remove(QLatin1Char('\r'));
    setMaximumBlockCount(m_maxLineCount);
    const bool atBottom = isScrollbarAtBottom();

    if (format == ErrorMessageFormat || format == NormalMessageFormat) {

        m_formatter->appendMessage(doNewlineEnfocement(out), format);

    } else {

        bool sameLine = format == StdOutFormatSameLine
                     || format == StdErrFormatSameLine;

        if (sameLine) {
            m_scrollToBottom = true;

            int newline = -1;
            bool enforceNewline = m_enforceNewline;
            m_enforceNewline = false;

            if (!enforceNewline) {
                newline = out.indexOf(QLatin1Char('\n'));
                moveCursor(QTextCursor::End);
                if (newline != -1)
                    m_formatter->appendMessage(out.left(newline), format);// doesn't enforce new paragraph like appendPlainText
            }

            QString s = out.mid(newline+1);
            if (s.isEmpty()) {
                m_enforceNewline = true;
            } else {
                if (s.endsWith(QLatin1Char('\n'))) {
                    m_enforceNewline = true;
                    s.chop(1);
                }
                m_formatter->appendMessage(QLatin1Char('\n') + s, format);
            }
        } else {
            m_formatter->appendMessage(doNewlineEnfocement(out), format);
        }
    }

    if (atBottom)
        scrollToBottom();
    enableUndoRedo();
}

// TODO rename
void OutputWindow::appendText(const QString &textIn, const QTextCharFormat &format)
{
    QString text = textIn;
    text.remove(QLatin1Char('\r'));
    if (m_maxLineCount > 0 && document()->blockCount() >= m_maxLineCount)
        return;
    const bool atBottom = isScrollbarAtBottom();
    QTextCursor cursor = QTextCursor(document());
    cursor.movePosition(QTextCursor::End);
    cursor.beginEditBlock();
    cursor.insertText(doNewlineEnfocement(text), format);

    if (m_maxLineCount > 0 && document()->blockCount() >= m_maxLineCount) {
        QTextCharFormat tmp;
        tmp.setFontWeight(QFont::Bold);
        cursor.insertText(doNewlineEnfocement(tr("Additional output omitted\n")), tmp);
    }

    cursor.endEditBlock();
    if (atBottom)
        scrollToBottom();
}

bool OutputWindow::isScrollbarAtBottom() const
{
    return verticalScrollBar()->value() == verticalScrollBar()->maximum();
}

void OutputWindow::clear()
{
    m_enforceNewline = false;
    QPlainTextEdit::clear();
}

void OutputWindow::scrollToBottom()
{
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    // QPlainTextEdit destroys the first calls value in case of multiline
    // text, so make sure that the scroll bar actually gets the value set.
    // Is a noop if the first call succeeded.
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void OutputWindow::grayOutOldContent()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat endFormat = cursor.charFormat();

    cursor.select(QTextCursor::Document);

    QTextCharFormat format;
    const QColor bkgColor = palette().base().color();
    const QColor fgdColor = palette().text().color();
    double bkgFactor = 0.50;
    double fgdFactor = 1.-bkgFactor;
    format.setForeground(QColor((bkgFactor * bkgColor.red() + fgdFactor * fgdColor.red()),
                             (bkgFactor * bkgColor.green() + fgdFactor * fgdColor.green()),
                             (bkgFactor * bkgColor.blue() + fgdFactor * fgdColor.blue()) ));
    cursor.mergeCharFormat(format);

    cursor.movePosition(QTextCursor::End);
    cursor.setCharFormat(endFormat);
    cursor.insertBlock(QTextBlockFormat());
}

void OutputWindow::enableUndoRedo()
{
    setMaximumBlockCount(0);
    setUndoRedoEnabled(true);
}

void OutputWindow::setWordWrapEnabled(bool wrap)
{
    if (wrap)
        setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    else
        setWordWrapMode(QTextOption::NoWrap);
}

} // namespace Core

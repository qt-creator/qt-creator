/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "outputwindow.h"

#include "actionmanager/actionmanager.h"
#include "coreconstants.h"
#include "icore.h"

#include <utils/outputformatter.h>
#include <utils/synchronousprocess.h>

#include <QAction>
#include <QScrollBar>

using namespace Utils;

namespace Core {

namespace Internal {

class OutputWindowPrivate
{
public:
    OutputWindowPrivate(QTextDocument *document)
        : outputWindowContext(0)
        , formatter(0)
        , enforceNewline(false)
        , scrollToBottom(false)
        , linksActive(true)
        , mousePressed(false)
        , maxLineCount(100000)
        , cursor(document)
    {
    }

    ~OutputWindowPrivate()
    {
        ICore::removeContextObject(outputWindowContext);
        delete outputWindowContext;
    }

    IContext *outputWindowContext;
    Utils::OutputFormatter *formatter;

    bool enforceNewline;
    bool scrollToBottom;
    bool linksActive;
    bool mousePressed;
    int maxLineCount;
    QTextCursor cursor;
};

} // namespace Internal

/*******************/

OutputWindow::OutputWindow(Context context, QWidget *parent)
    : QPlainTextEdit(parent)
    , d(new Internal::OutputWindowPrivate(document()))
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //setCenterOnScroll(false);
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);
    setUndoRedoEnabled(false);

    d->outputWindowContext = new IContext;
    d->outputWindowContext->setContext(context);
    d->outputWindowContext->setWidget(this);
    ICore::addContextObject(d->outputWindowContext);

    QAction *undoAction = new QAction(this);
    QAction *redoAction = new QAction(this);
    QAction *cutAction = new QAction(this);
    QAction *copyAction = new QAction(this);
    QAction *pasteAction = new QAction(this);
    QAction *selectAllAction = new QAction(this);

    ActionManager::registerAction(undoAction, Constants::UNDO, context);
    ActionManager::registerAction(redoAction, Constants::REDO, context);
    ActionManager::registerAction(cutAction, Constants::CUT, context);
    ActionManager::registerAction(copyAction, Constants::COPY, context);
    ActionManager::registerAction(pasteAction, Constants::PASTE, context);
    ActionManager::registerAction(selectAllAction, Constants::SELECTALL, context);

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
    delete d;
}

void OutputWindow::mousePressEvent(QMouseEvent * e)
{
    d->mousePressed = true;
    QPlainTextEdit::mousePressEvent(e);
}

void OutputWindow::mouseReleaseEvent(QMouseEvent *e)
{
    d->mousePressed = false;

    if (d->linksActive) {
        const QString href = anchorAt(e->pos());
        if (d->formatter)
            d->formatter->handleLink(href);
    }

    // Mouse was released, activate links again
    d->linksActive = true;

    QPlainTextEdit::mouseReleaseEvent(e);
}

void OutputWindow::mouseMoveEvent(QMouseEvent *e)
{
    // Cursor was dragged to make a selection, deactivate links
    if (d->mousePressed && textCursor().hasSelection())
        d->linksActive = false;

    if (!d->linksActive || anchorAt(e->pos()).isEmpty())
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
    return d->formatter;
}

void OutputWindow::setFormatter(OutputFormatter *formatter)
{
    d->formatter = formatter;
    d->formatter->setPlainTextEdit(this);
}

void OutputWindow::showEvent(QShowEvent *e)
{
    QPlainTextEdit::showEvent(e);
    if (d->scrollToBottom)
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    d->scrollToBottom = false;
}

QString OutputWindow::doNewlineEnforcement(const QString &out)
{
    d->scrollToBottom = true;
    QString s = out;
    if (d->enforceNewline) {
        s.prepend(QLatin1Char('\n'));
        d->enforceNewline = false;
    }

    if (s.endsWith(QLatin1Char('\n'))) {
        d->enforceNewline = true; // make appendOutputInline put in a newline next time
        s.chop(1);
    }

    return s;
}

void OutputWindow::setMaxLineCount(int count)
{
    d->maxLineCount = count;
    setMaximumBlockCount(d->maxLineCount);
}

int OutputWindow::maxLineCount() const
{
    return d->maxLineCount;
}

void OutputWindow::appendMessage(const QString &output, OutputFormat format)
{
    const QString out = SynchronousProcess::normalizeNewlines(output);
    setMaximumBlockCount(d->maxLineCount);
    const bool atBottom = isScrollbarAtBottom();

    if (format == ErrorMessageFormat || format == NormalMessageFormat) {

        d->formatter->appendMessage(doNewlineEnforcement(out), format);

    } else {

        bool sameLine = format == StdOutFormatSameLine
                     || format == StdErrFormatSameLine;

        if (sameLine) {
            d->scrollToBottom = true;

            int newline = -1;
            bool enforceNewline = d->enforceNewline;
            d->enforceNewline = false;

            if (!enforceNewline) {
                newline = out.indexOf(QLatin1Char('\n'));
                moveCursor(QTextCursor::End);
                if (newline != -1)
                    d->formatter->appendMessage(out.left(newline), format);// doesn't enforce new paragraph like appendPlainText
            }

            QString s = out.mid(newline+1);
            if (s.isEmpty()) {
                d->enforceNewline = true;
            } else {
                if (s.endsWith(QLatin1Char('\n'))) {
                    d->enforceNewline = true;
                    s.chop(1);
                }
                d->formatter->appendMessage(QLatin1Char('\n') + s, format);
            }
        } else {
            d->formatter->appendMessage(doNewlineEnforcement(out), format);
        }
    }

    if (atBottom)
        scrollToBottom();
    enableUndoRedo();
}

// TODO rename
void OutputWindow::appendText(const QString &textIn, const QTextCharFormat &format)
{
    const QString text = SynchronousProcess::normalizeNewlines(textIn);
    if (d->maxLineCount > 0 && document()->blockCount() >= d->maxLineCount)
        return;
    const bool atBottom = isScrollbarAtBottom();
    if (!d->cursor.atEnd())
        d->cursor.movePosition(QTextCursor::End);
    d->cursor.beginEditBlock();
    d->cursor.insertText(doNewlineEnforcement(text), format);

    if (d->maxLineCount > 0 && document()->blockCount() >= d->maxLineCount) {
        QTextCharFormat tmp;
        tmp.setFontWeight(QFont::Bold);
        d->cursor.insertText(doNewlineEnforcement(tr("Additional output omitted") + QLatin1Char('\n')), tmp);
    }

    d->cursor.endEditBlock();
    if (atBottom)
        scrollToBottom();
}

bool OutputWindow::isScrollbarAtBottom() const
{
    return verticalScrollBar()->value() == verticalScrollBar()->maximum();
}

void OutputWindow::clear()
{
    d->enforceNewline = false;
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
    if (!d->cursor.atEnd())
        d->cursor.movePosition(QTextCursor::End);
    QTextCharFormat endFormat = d->cursor.charFormat();

    d->cursor.select(QTextCursor::Document);

    QTextCharFormat format;
    const QColor bkgColor = palette().base().color();
    const QColor fgdColor = palette().text().color();
    double bkgFactor = 0.50;
    double fgdFactor = 1.-bkgFactor;
    format.setForeground(QColor((bkgFactor * bkgColor.red() + fgdFactor * fgdColor.red()),
                             (bkgFactor * bkgColor.green() + fgdFactor * fgdColor.green()),
                             (bkgFactor * bkgColor.blue() + fgdFactor * fgdColor.blue()) ));
    d->cursor.mergeCharFormat(format);

    d->cursor.movePosition(QTextCursor::End);
    d->cursor.setCharFormat(endFormat);
    d->cursor.insertBlock(QTextBlockFormat());
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

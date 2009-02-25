/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "outputwindow.h"
#include "projectexplorerconstants.h"
#include "runconfiguration.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <find/basetextfind.h>
#include <aggregation/aggregate.h>

#include <QtGui/QIcon>
#include <QtGui/QScrollBar>
#include <QtGui/QTextLayout>
#include <QtGui/QPainter>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QTabWidget>

using namespace ProjectExplorer::Internal;
using namespace ProjectExplorer;

OutputPane::OutputPane()
    : m_mainWidget(new QWidget)
{
//     m_insertLineButton = new QToolButton;
//     m_insertLineButton->setIcon(QIcon(ProjectExplorer::Constants::ICON_INSERT_LINE));
//     m_insertLineButton->setText(tr("Insert line"));
//     m_insertLineButton->setToolTip(tr("Insert line"));
//     m_insertLineButton->setAutoRaise(true);
//     connect(m_insertLineButton, SIGNAL(clicked()), this, SLOT(insertLine()));

    QIcon runIcon(Constants::ICON_RUN);
    runIcon.addFile(Constants::ICON_RUN_SMALL);

    // Rerun
    m_reRunButton = new QToolButton;
    m_reRunButton->setIcon(runIcon);
    m_reRunButton->setToolTip(tr("Rerun this runconfiguration"));
    m_reRunButton->setAutoRaise(true);
    m_reRunButton->setEnabled(false);
    connect(m_reRunButton, SIGNAL(clicked()),
            this, SLOT(reRunRunControl()));

    // Stop
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    QList<int> globalcontext;
    globalcontext.append(Core::Constants::C_GLOBAL_ID);

    m_stopAction = new QAction(QIcon(Constants::ICON_STOP), tr("Stop"), this);
    m_stopAction->setToolTip(tr("Stop"));
    m_stopAction->setEnabled(false);

    Core::Command *cmd = am->registerAction(m_stopAction, Constants::STOP, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+R")));

    m_stopButton = new QToolButton;
    m_stopButton->setDefaultAction(cmd->action());
    m_stopButton->setAutoRaise(true);

    connect(m_stopAction, SIGNAL(triggered()),
            this, SLOT(stopRunControl()));

    // Spacer (?)

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    m_tabWidget = new QTabWidget;
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    layout->addWidget(m_tabWidget);

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

    m_mainWidget->setLayout(layout);

    connect(Core::ICore::instance(), SIGNAL(coreAboutToClose()),
            this, SLOT(coreAboutToClose()));
}

void OutputPane::coreAboutToClose()
{
    while (m_tabWidget->count()) {
        RunControl *rc = runControlForTab(0);
        if (rc->isRunning())
            rc->stop();
        closeTab(0);
    }
}

OutputPane::~OutputPane()
{
    delete m_mainWidget;
}

QWidget *OutputPane::outputWidget(QWidget *)
{
    return m_mainWidget;
}

QList<QWidget*> OutputPane::toolBarWidgets() const
{
    return QList<QWidget*>() << m_reRunButton << m_stopButton
            ; // << m_insertLineButton;
}

QString OutputPane::name() const
{
    return tr("Application Output");
}

int OutputPane::priorityInStatusBar() const
{
    return 60;
}

void OutputPane::clearContents()
{
    OutputWindow *currentWindow = qobject_cast<OutputWindow *>(m_tabWidget->currentWidget());
    if (currentWindow)
        currentWindow->clear();
}

void OutputPane::visibilityChanged(bool /* b */)
{
}

bool OutputPane::hasFocus()
{
    return m_tabWidget->currentWidget() && m_tabWidget->currentWidget()->hasFocus();
}

bool OutputPane::canFocus()
{
    return m_tabWidget->currentWidget();
}

void OutputPane::setFocus()
{
    if (m_tabWidget->currentWidget())
        m_tabWidget->currentWidget()->setFocus();
}

void OutputPane::appendOutput(const QString &/*out*/)
{
    // This function is in the interface, since we can't do anything sensible here, we don't do anything here.
}

void OutputPane::createNewOutputWindow(RunControl *rc)
{
    connect(rc, SIGNAL(started()),
            this, SLOT(runControlStarted()));
    connect(rc, SIGNAL(finished()),
            this, SLOT(runControlFinished()));

    // First look if we can reuse a tab
    bool found = false;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        RunControl *old = runControlForTab(i);
        if (old->runConfiguration() == rc->runConfiguration() && !old->isRunning()) {
            // Reuse this tab
            delete old;
            m_outputWindows.remove(old);
            OutputWindow *ow = static_cast<OutputWindow *>(m_tabWidget->widget(i));
            ow->appendOutput("");//New line
            m_outputWindows.insert(rc, ow);
            found = true;
            break;
        }
    }
    if (!found) {
        OutputWindow *ow = new OutputWindow(m_tabWidget);
        Aggregation::Aggregate *agg = new Aggregation::Aggregate;
        agg->add(ow);
        agg->add(new Find::BaseTextFind(ow));
        m_outputWindows.insert(rc, ow);
        m_tabWidget->addTab(ow, rc->runConfiguration()->name());
    }
}

void OutputPane::appendOutput(RunControl *rc, const QString &out)
{
    OutputWindow *ow = m_outputWindows.value(rc);
    ow->appendOutput(out);
}

void OutputPane::appendOutputInline(RunControl *rc, const QString &out)
{
    OutputWindow *ow = m_outputWindows.value(rc);
    ow->appendOutputInline(out);
}

void OutputPane::showTabFor(RunControl *rc)
{
    OutputWindow *ow = m_outputWindows.value(rc);
    m_tabWidget->setCurrentWidget(ow);
}

void OutputPane::insertLine()
{
    OutputWindow *currentWindow = qobject_cast<OutputWindow *>(m_tabWidget->currentWidget());
    if (currentWindow)
        currentWindow->clear();
}

void OutputPane::reRunRunControl()
{
    RunControl *rc = runControlForTab(m_tabWidget->currentIndex());
    if (rc->runConfiguration()->project() != 0)
        rc->start();
}

void OutputPane::stopRunControl()
{
    RunControl *rc = runControlForTab(m_tabWidget->currentIndex());
    rc->stop();
}

void OutputPane::closeTab(int index)
{
    OutputWindow *ow = static_cast<OutputWindow *>(m_tabWidget->widget(index));
    RunControl *rc = m_outputWindows.key(ow);

    if (rc->isRunning()) {
        QString msg = tr("The application is still running. Close it first.");
        QMessageBox::critical(0, tr("Unable to close"), msg);
        return;
    }

    m_tabWidget->removeTab(index);
    delete ow;
    delete rc;
}

void OutputPane::projectRemoved()
{
    tabChanged(m_tabWidget->currentIndex());
}

void OutputPane::tabChanged(int i)
{
    if (i == -1) {
        m_stopAction->setEnabled(false);
        m_reRunButton->setEnabled(false);
    } else {
        RunControl *rc = runControlForTab(i);
        m_stopAction->setEnabled(rc->isRunning());
        m_reRunButton->setEnabled(!rc->isRunning() && rc->runConfiguration()->project());
    }
}

void OutputPane::runControlStarted()
{
    RunControl *rc = runControlForTab(m_tabWidget->currentIndex());
    if (rc == qobject_cast<RunControl *>(sender())) {
        m_reRunButton->setEnabled(false);
        m_stopAction->setEnabled(true);
    }
}

void OutputPane::runControlFinished()
{
    RunControl *rc = runControlForTab(m_tabWidget->currentIndex());
    if (rc == qobject_cast<RunControl *>(sender())) {
        m_reRunButton->setEnabled(rc->runConfiguration()->project());
        m_stopAction->setEnabled(false);
    }
}

RunControl* OutputPane::runControlForTab(int index) const
{
    return m_outputWindows.key(qobject_cast<OutputWindow *>(m_tabWidget->widget(index)));
}


/*******************/

OutputWindow::OutputWindow(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //setCenterOnScroll(false);
    //setMaximumBlockCount(10000);
    setWindowTitle(tr("Application Output Window"));
    setWindowIcon(QIcon(":/qt4projectmanager/images/window.png"));
    setFrameShape(QFrame::NoFrame);
}


OutputWindow::~OutputWindow()
{
}

void OutputWindow::appendOutput(const QString &out)
{
    if (out.endsWith('\n'))
        appendPlainText(out);
    else
        appendPlainText(out + '\n');
}

void OutputWindow::appendOutputInline(const QString &out)
{
    moveCursor(QTextCursor::End);
    insertPlainText(out);
}

void OutputWindow::insertLine()
{
    appendPlainText(QString());
}

#if 0
OutputWindow::OutputWindow(QWidget *parent)
        : QAbstractScrollArea(parent)
{
    max_lines = 1000;
    width_used = 0;
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    same_height = true;
    block_scroll = false;
    setWindowTitle(tr("Application Output Window"));
    setWindowIcon(QIcon(":/qt4projectmanager/images/window.png"));
}

void OutputWindow::changed() {
    int remove = lines.size() - max_lines;
    if (remove > 0) {
        selection_start.line -= remove;
        selection_end.line -= remove;
        selection_start = qMax(selection_start, Selection());
        selection_end = qMax(selection_end, Selection());
        if (remove > verticalScrollBar()->value()) {
            if (same_height)
                viewport()->scroll(0, -remove * fontMetrics().lineSpacing());
            else
                viewport()->update();
        } else {
            block_scroll = true;
            verticalScrollBar()->setValue(verticalScrollBar()->value() - remove);
            block_scroll = false;
        }
        while (remove--)
            lines.removeFirst();
    }

    verticalScrollBar()->setRange(0, lines.size() - 1);

}


bool OutputWindow::getCursorPos(int *lineNumber, int *position, const QPoint &pos) {
    if (lines.isEmpty())
        return false;
    *lineNumber = verticalScrollBar()->value();

    int x = 4 - horizontalScrollBar()->value();

    int spacing = fontMetrics().lineSpacing();
    int leading = fontMetrics().leading();
    int height = 0;

    QTextLayout textLayout;
    textLayout.setFont(font());

    if (same_height && pos.y() > 0) {
        int skipLines = pos.y() / spacing;
        height += skipLines * spacing;
        *lineNumber = qMin(*lineNumber + skipLines, lines.size() - 1);
    }

    same_height = true;

    while ( *lineNumber < lines.size()) {
        textLayout.setText(lines.at(*lineNumber));

        textLayout.beginLayout();
        while (1) {
            QTextLine line = textLayout.createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(INT_MAX/256);
            height += leading;
            line.setPosition(QPoint(x, height));
            height += static_cast<int>(line.height());
        }
        textLayout.endLayout();
        if (height > pos.y()) {
            *position = textLayout.lineAt(0).xToCursor(pos.x());
            break;
        }
        ++*lineNumber;
    }
    return true;
}

void OutputWindow::setNumberOfLines(int max)
{
    max_lines = qMax(1, max);
    while (lines.size() > max_lines)
        lines.removeLast();
    changed();
}

int OutputWindow::numberOfLines() const
{
    return max_lines;
}

bool OutputWindow::hasSelectedText() const
{
    return selection_start != selection_end;
}

void OutputWindow::clearSelection()
{
    bool hadSelectedText = hasSelectedText();
    selection_start = selection_end = Selection();
    if (hadSelectedText)
        viewport()->update();
}

QString OutputWindow::selectedText() const
{
    Selection sel_start = qMin(selection_start, selection_end);
    Selection sel_end = qMax(selection_start, selection_end);
    QString text;

    if (sel_start.line == sel_end.line) {
        text += lines.at(sel_start.line).mid(sel_start.pos, sel_end.pos - sel_start.pos);
    } else {
        int line = sel_start.line;
        text += lines.at(line++).mid(sel_start.pos);
        text += QLatin1Char('\n');
        while (line < sel_end.line) {
            text += lines.at(line++);
            text += QLatin1Char('\n');
        }
        text += lines.at(sel_end.line).left(sel_end.pos);
    }
    return text;
}

void OutputWindow::appendOutput(const QString &text)
{
    lines.append(text);
    if (same_height)
        viewport()->update(
            QRect(0, (lines.size() - verticalScrollBar()->value() - 1) * fontMetrics().lineSpacing(),
                  viewport()->width(), viewport()->height()));
    else
        viewport()->update();

    changed();
    int top = lines.size() - (viewport()->height() / fontMetrics().lineSpacing());
    if (verticalScrollBar()->value() == top - 1)
        verticalScrollBar()->setValue(top);
}

void OutputWindow::clear()
{
    clearSelection();
    lines.clear();
    viewport()->update();
}

void OutputWindow::copy()
{
    if (hasSelectedText())
        QApplication::clipboard()->setText(selectedText());
}

void OutputWindow::selectAll()
{
    selection_start = Selection();
    selection_end.line = lines.size() - 1;
    selection_end.pos = lines.last().length() - 1;
    viewport()->update();
}

void OutputWindow::scrollContentsBy(int dx, int dy)
{
    if (block_scroll)
        return;
    if (dx && dy) {
        viewport()->update();
    } else if (dx && !dy) {
        viewport()->scroll(dx, 0);
    } else {
        if (same_height) {
            viewport()->scroll(0, fontMetrics().lineSpacing() * dy);
        } else {
            viewport()->update();
        }
    }
}

void OutputWindow::keyPressEvent(QKeyEvent *e)
{
    bool accept = true;
    if (e == QKeySequence::Copy) {
        copy();
    } else if (e == QKeySequence::SelectAll) {
        selectAll();
    } else if (e->key() == Qt::Key_Enter
               || e->key() == Qt::Key_Return) {
        insertLine();
    } else {
        accept = false;
    }

    if (accept)
        e->accept();
    else
        QAbstractScrollArea::keyPressEvent(e);
}

void OutputWindow::paintEvent(QPaintEvent *e)
{
    int lineNumber = verticalScrollBar()->value();

    int x = 4 - horizontalScrollBar()->value();
    QPainter p(viewport());

    int spacing = fontMetrics().lineSpacing();
    int leading = fontMetrics().leading();
    int height = 0;

    QTextLayout textLayout;
    textLayout.setFont(font());

    QTextCharFormat selectionFormat;
    selectionFormat.setBackground(palette().highlight());
    selectionFormat.setForeground(palette().highlightedText());

    if (e->rect().top() <= 0 && e->rect().bottom() >= viewport()->rect().bottom())
        width_used = 0; // recalculate

    if (same_height) {
        int skipLines = e->rect().top() / spacing;
        height += skipLines * spacing;
        lineNumber += skipLines;
    }

    same_height = true;

    Selection sel_start = qMin(selection_start, selection_end);
    Selection sel_end = qMax(selection_start, selection_end);

    while ( lineNumber < lines.size() && height <= e->rect().bottom()) {

        QString line = lines.at(lineNumber);

        if (line.size() == 1 && line.at(0) == QChar::ParagraphSeparator) {
            int y = height + spacing/2;
            p.drawLine(e->rect().left(), y, e->rect().right(), y);
            height += spacing;

        } else {
            textLayout.setText(line);
            textLayout.beginLayout();
            while (1) {
                QTextLine line = textLayout.createLine();
                if (!line.isValid())
                    break;
                line.setLineWidth(INT_MAX/256);
                height += leading;
                line.setPosition(QPoint(x, height));
                height += static_cast<int>(line.height());

                same_height = same_height && (line.height() + leading) == spacing;
                width_used = qMax(width_used, 8 + static_cast<int>(line.naturalTextWidth()));
            }
            textLayout.endLayout();

            if (lineNumber >= sel_start.line && lineNumber <= sel_end.line) {
                QVector<QTextLayout::FormatRange> selection(1);
                selection[0].start = (lineNumber == sel_start.line)? sel_start.pos : 0;
                selection[0].length = ((lineNumber == sel_end.line) ? sel_end.pos : lines.at(lineNumber).size()) - selection[0].start;
                selection[0].format = selectionFormat;

                textLayout.draw(&p, QPoint(0, 0), selection);
            } else {
                textLayout.draw(&p, QPoint(0, 0));
            }
        }


        ++lineNumber;
    }

    horizontalScrollBar()->setRange(0, qMax(0, width_used - viewport()->width()));
    if (horizontalScrollBar()->pageStep() != viewport()->width())
        horizontalScrollBar()->setPageStep(viewport()->width());
    if (height > viewport()->height())
        verticalScrollBar()->setPageStep(lineNumber - verticalScrollBar()->value());
    else if (verticalScrollBar()->pageStep() != viewport()->height() / fontMetrics().lineSpacing())
        verticalScrollBar()->setPageStep(viewport()->height() / fontMetrics().lineSpacing());
}

void OutputWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        clearSelection();
        if (getCursorPos(&selection_start.line, &selection_start.pos, e->pos())) {
            selection_end = selection_start;
            autoscroll = 0;
        }
    }
}

void OutputWindow::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == autoscroll_timer.timerId()) {
        int autoscroll = 0;
        if (lastMouseMove.y() < 0)
            autoscroll = -1;
        else if (lastMouseMove.y() > viewport()->height())
            autoscroll = 1;
        if (autoscroll) {
            verticalScrollBar()->setValue(verticalScrollBar()->value() + autoscroll);
            OutputWindow::mouseMoveEvent(0);
        }
    }
    QAbstractScrollArea::timerEvent(e);
}

void OutputWindow::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        autoscroll_timer.stop();
        if (hasSelectedText() &&  QApplication::clipboard()->supportsSelection())
            QApplication::clipboard()->setText(selectedText(), QClipboard::Selection);
    }
}

void OutputWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (e) {
        lastMouseMove = e->pos();
        if (viewport()->rect().contains(e->pos()))
            autoscroll_timer.stop();
        else
            autoscroll_timer.start(20, this);
    }


    Selection old = selection_end;
    if (!getCursorPos(&selection_end.line, &selection_end.pos, lastMouseMove))
        return;
    if (same_height) {
        Selection from = qMin(old, selection_end);
        Selection to = qMax(old, selection_end);
        viewport()->update(QRect(0, -1 + (from.line - verticalScrollBar()->value()) * fontMetrics().lineSpacing(),
                                 viewport()->width(), 2 + (to.line - from.line + 1) * fontMetrics().lineSpacing()));
    } else {
        viewport()->update();
    }
}

void OutputWindow::contextMenuEvent(QContextMenuEvent * e)
{
    QMenu menu(this);
    QAction *clearAction = menu.addAction("Clear", this, SLOT(clear()));
    QAction *copyAction = menu.addAction("Copy", this, SLOT(copy()), QKeySequence::Copy);
    QAction *selectAllAction = menu.addAction("Select All", this, SLOT(selectAll()), QKeySequence::SelectAll);
    if (lines.empty()) {
        clearAction->setDisabled(true);
        selectAllAction->setDisabled(true);
    }
    if (!hasSelectedText())
        copyAction->setDisabled(true);

    menu.exec(e->globalPos());
}

#endif // 0

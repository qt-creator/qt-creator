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

#include "outputwindow.h"
#include "projectexplorerconstants.h"
#include "runconfiguration.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/icontext.h>
#include <find/basetextfind.h>
#include <aggregation/aggregate.h>
#include <texteditor/basetexteditor.h>

#include <QtGui/QIcon>
#include <QtGui/QScrollBar>
#include <QtGui/QTextLayout>
#include <QtGui/QTextBlock>
#include <QtGui/QPainter>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QTabWidget>
#include <QtGui/QToolButton>

using namespace ProjectExplorer::Internal;
using namespace ProjectExplorer;

static const int MaxBlockCount = 100000;

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
    m_reRunButton->setToolTip(tr("Re-run this run-configuration"));
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
        if (old->sameRunConfiguration(rc) && !old->isRunning()) {
            // Reuse this tab
            delete old;
            m_outputWindows.remove(old);
            OutputWindow *ow = static_cast<OutputWindow *>(m_tabWidget->widget(i));
            ow->grayOutOldContent();
            ow->verticalScrollBar()->setValue(ow->verticalScrollBar()->maximum());
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
        m_tabWidget->addTab(ow, rc->displayName());
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
    int index = m_tabWidget->currentIndex();
    RunControl *rc = runControlForTab(index);
    OutputWindow *ow = static_cast<OutputWindow *>(m_tabWidget->widget(index));
    ow->grayOutOldContent();
    ow->verticalScrollBar()->setValue(ow->verticalScrollBar()->maximum());
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
        m_reRunButton->setEnabled(!rc->isRunning());
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
        m_reRunButton->setEnabled(rc);
        m_stopAction->setEnabled(false);
    }
}

RunControl* OutputPane::runControlForTab(int index) const
{
    return m_outputWindows.key(qobject_cast<OutputWindow *>(m_tabWidget->widget(index)));
}

bool OutputPane::canNext()
{
    return false;
}

bool OutputPane::canPrevious()
{
    return false;
}

void OutputPane::goToNext()
{

}

void OutputPane::goToPrev()
{

}

bool OutputPane::canNavigate()
{
    return false;
}

/*******************/

OutputWindow::OutputWindow(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_qmlError(QLatin1String("(file:///[^:]+:\\d+:\\d+):"))
    , m_enforceNewline(false)
    , m_scrollToBottom(false)
    , m_linksActive(true)
    , m_mousePressed(false)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //setCenterOnScroll(false);
    setWindowTitle(tr("Application Output Window"));
    setWindowIcon(QIcon(":/qt4projectmanager/images/window.png"));
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);

    static uint usedIds = 0;
    Core::ICore *core = Core::ICore::instance();
    QList<int> context;
    context << core->uniqueIDManager()->uniqueIdentifier(QString(Constants::C_APP_OUTPUT) + QString().setNum(usedIds++));
    m_outputWindowContext = new Core::BaseContext(this, context);
    core->addContextObject(m_outputWindowContext);

    QAction *undoAction = new QAction(this);
    QAction *redoAction = new QAction(this);
    QAction *cutAction = new QAction(this);
    QAction *copyAction = new QAction(this);
    QAction *pasteAction = new QAction(this);
    QAction *selectAllAction = new QAction(this);

    Core::ActionManager *am = core->actionManager();
    am->registerAction(undoAction, Core::Constants::UNDO, context);
    am->registerAction(redoAction, Core::Constants::REDO, context);
    am->registerAction(cutAction, Core::Constants::CUT, context);
    am->registerAction(copyAction, Core::Constants::COPY, context);
    am->registerAction(pasteAction, Core::Constants::PASTE, context);
    am->registerAction(selectAllAction, Core::Constants::SELECTALL, context);

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
    Core::ICore::instance()->removeContextObject(m_outputWindowContext);
    delete m_outputWindowContext;
}

void OutputWindow::showEvent(QShowEvent *e)
{
    QPlainTextEdit::showEvent(e);
    if (m_scrollToBottom) {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }
    m_scrollToBottom = false;
}

void OutputWindow::appendOutput(const QString &out)
{
    m_scrollToBottom = true;
    QString s = out;
    m_enforceNewline = true; // make appendOutputInline put in a newline next time
    if (s.endsWith(QLatin1Char('\n'))) {
        s.chop(1);
    }
    setMaximumBlockCount(MaxBlockCount);

    QTextCharFormat format;
    format.setForeground(palette().text().color());
    setCurrentCharFormat(format);
    appendPlainText(out);
    enableUndoRedo();
}

void OutputWindow::appendOutputInline(const QString &out)
{
    m_scrollToBottom = true;
    setMaximumBlockCount(MaxBlockCount);

    int newline = -1;
    bool enforceNewline = m_enforceNewline;
    m_enforceNewline = false;

    if (!enforceNewline) {
        newline = out.indexOf(QLatin1Char('\n'));
        moveCursor(QTextCursor::End);
        bool atBottom = (blockBoundingRect(document()->lastBlock()).bottom() + contentOffset().y()
                         <= viewport()->rect().bottom());
        insertPlainText(newline < 0 ? out : out.left(newline)); // doesn't enforce new paragraph like appendPlainText
        if (atBottom)
            verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }

    QString s = out.mid(newline+1);
    if (s.isEmpty()) {
        m_enforceNewline = true;
    } else {
        if (s.endsWith(QLatin1Char('\n'))) {
            m_enforceNewline = true;
            s.chop(1);
        }
        QTextCharFormat format;
        format.setForeground(palette().text().color());
        setCurrentCharFormat(format);

        // (This feature depends on the availability of QPlainTextEdit::anchorAt)
        // Convert to HTML, preserving newlines and whitespace
        s = Qt::convertFromPlainText(s);

        // Create links from QML errors (anything of the form "file:///...:[line]:[column]:")
        int index = 0;
        while ((index = m_qmlError.indexIn(s, index)) != -1) {
            const QString captured = m_qmlError.cap(1);
            const QString link = QString(QLatin1String("<a href=\"%1\">%2</a>")).arg(captured, captured);
            s.replace(index, captured.length(), link);
            index += link.length();
        }
        appendHtml(s);
    }

    enableUndoRedo();
}

void OutputWindow::insertLine()
{
    m_scrollToBottom = true;
    setMaximumBlockCount(MaxBlockCount);
    appendPlainText(QString());
    enableUndoRedo();
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

void OutputWindow::mousePressEvent(QMouseEvent *e)
{
    QPlainTextEdit::mousePressEvent(e);
    m_mousePressed = true;
}

void OutputWindow::mouseReleaseEvent(QMouseEvent *e)
{
    QPlainTextEdit::mouseReleaseEvent(e);
    m_mousePressed = false;

    if (!m_linksActive) {
        // Mouse was released, activate links again
        m_linksActive = true;
        return;
    }

    const QString href = anchorAt(e->pos());
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

void OutputWindow::mouseMoveEvent(QMouseEvent *e)
{
    QPlainTextEdit::mouseMoveEvent(e);

    // Cursor was dragged to make a selection, deactivate links
    if (m_mousePressed && textCursor().hasSelection())
        m_linksActive = false;

    if (!m_linksActive || anchorAt(e->pos()).isEmpty())
        viewport()->setCursor(Qt::IBeamCursor);
    else
        viewport()->setCursor(Qt::PointingHandCursor);
}

/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "outputwindow.h"
#include "projectexplorerconstants.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "runconfiguration.h"
#include "session.h"
#include "outputformatter.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/icontext.h>
#include <find/basetextfind.h>
#include <aggregation/aggregate.h>
#include <texteditor/basetexteditor.h>
#include <projectexplorer/project.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <utils/qtcassert.h>

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
#include <QtGui/QShowEvent>

#include <QtCore/QDebug>

static const int MaxBlockCount = 100000;

enum { debug = 0 };

namespace ProjectExplorer {
namespace Internal {

OutputPane::RunControlTab::RunControlTab(RunControl *rc, OutputWindow *w) :
    runControl(rc), window(w), asyncClosing(false)
{
}

OutputPane::OutputPane() :
    m_mainWidget(new QWidget),
    m_tabWidget(new QTabWidget),
    m_stopAction(new QAction(QIcon(QLatin1String(Constants::ICON_STOP)), tr("Stop"), this)),
    m_reRunButton(new QToolButton),
    m_stopButton(new QToolButton)
{
    m_runIcon.addFile(Constants::ICON_RUN);
    m_runIcon.addFile(Constants::ICON_RUN_SMALL);

    // Rerun
    m_reRunButton->setIcon(m_runIcon);
    m_reRunButton->setToolTip(tr("Re-run this run-configuration"));
    m_reRunButton->setAutoRaise(true);
    m_reRunButton->setEnabled(false);
    connect(m_reRunButton, SIGNAL(clicked()),
            this, SLOT(reRunRunControl()));

    // Stop
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    Core::Context globalcontext(Core::Constants::C_GLOBAL);

    m_stopAction->setToolTip(tr("Stop"));
    m_stopAction->setEnabled(false);

    Core::Command *cmd = am->registerAction(m_stopAction, Constants::STOP, globalcontext);

    m_stopButton->setDefaultAction(cmd->action());
    m_stopButton->setAutoRaise(true);

    connect(m_stopAction, SIGNAL(triggered()),
            this, SLOT(stopRunControl()));

    // Spacer (?)

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    layout->addWidget(m_tabWidget);

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

    m_mainWidget->setLayout(layout);

    connect(ProjectExplorer::ProjectExplorerPlugin::instance()->session(), SIGNAL(aboutToUnloadSession()),
            this, SLOT(aboutToUnloadSession()));
}

OutputPane::~OutputPane()
{
    if (debug)
        qDebug() << "OutputPane::~OutputPane: Entries left" << m_runControlTabs.size();

    foreach(const RunControlTab &rt, m_runControlTabs)
        delete rt.runControl;
    delete m_mainWidget;
}

int OutputPane::currentIndex() const
{
    if (const QWidget *w = m_tabWidget->currentWidget())
        return indexOf(w);
    return -1;
}

RunControl *OutputPane::currentRunControl() const
{
    const int index = currentIndex();
    if (index != -1)
        return m_runControlTabs.at(index).runControl;
    return 0;
}

int OutputPane::indexOf(const RunControl *rc) const
{
    for (int i = m_runControlTabs.size() - 1; i >= 0; i--)
        if (m_runControlTabs.at(i).runControl == rc)
            return i;
    return -1;
}

int OutputPane::indexOf(const QWidget *outputWindow) const
{
    for (int i = m_runControlTabs.size() - 1; i >= 0; i--)
        if (m_runControlTabs.at(i).window == outputWindow)
            return i;
    return -1;
}

int OutputPane::tabWidgetIndexOf(int runControlIndex) const
{
    if (runControlIndex >= 0 && runControlIndex < m_runControlTabs.size())
        return m_tabWidget->indexOf(m_runControlTabs.at(runControlIndex).window);
    return -1;
}

bool OutputPane::aboutToClose() const
{
    foreach(const RunControlTab &rt, m_runControlTabs)
        if (rt.runControl->isRunning() && !rt.runControl->promptToStop())
            return false;
    return true;
}

void OutputPane::aboutToUnloadSession()
{
    closeTabs(CloseTabWithPrompt);
}

QWidget *OutputPane::outputWidget(QWidget *)
{
    return m_mainWidget;
}

QList<QWidget*> OutputPane::toolBarWidgets() const
{
    return QList<QWidget*>() << m_reRunButton << m_stopButton;
}

QString OutputPane::displayName() const
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

void OutputPane::createNewOutputWindow(RunControl *rc)
{
    connect(rc, SIGNAL(started()),
            this, SLOT(runControlStarted()));
    connect(rc, SIGNAL(finished()),
            this, SLOT(runControlFinished()));
    connect(rc, SIGNAL(appendMessage(ProjectExplorer::RunControl*,QString,ProjectExplorer::OutputFormat)),
            this, SLOT(appendMessage(ProjectExplorer::RunControl*,QString,ProjectExplorer::OutputFormat)));

    // First look if we can reuse a tab
    const int size = m_runControlTabs.size();
    for (int i = 0; i < size; i++) {
        RunControlTab &tab =m_runControlTabs[i];
        if (tab.runControl->sameRunConfiguration(rc) && !tab.runControl->isRunning()) {
            // Reuse this tab
            delete tab.runControl;
            tab.runControl = rc;
            tab.window->handleOldOutput();
            tab.window->scrollToBottom();
            tab.window->setFormatter(rc->outputFormatter());
            if (debug)
                qDebug() << "OutputPane::createNewOutputWindow: Reusing tab" << i << " for " << rc;
            return;
        }
    }
    // Create new
    OutputWindow *ow = new OutputWindow(m_tabWidget);
    ow->setWindowTitle(tr("Application Output Window"));
    ow->setWindowIcon(QIcon(QLatin1String(Qt4ProjectManager::Constants::ICON_WINDOW)));
    ow->setFormatter(rc->outputFormatter());
    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(ow);
    agg->add(new Find::BaseTextFind(ow));
    m_runControlTabs.push_back(RunControlTab(rc, ow));
    m_tabWidget->addTab(ow, rc->displayName());
    if (debug)
        qDebug() << "OutputPane::createNewOutputWindow: Adding tab for " << rc;
}

void OutputPane::appendMessage(RunControl *rc, const QString &out, OutputFormat format)
{
    const int index = indexOf(rc);
    if (index != -1)
        m_runControlTabs.at(index).window->appendMessage(out, format);
}

void OutputPane::showTabFor(RunControl *rc)
{
    m_tabWidget->setCurrentIndex(tabWidgetIndexOf(indexOf(rc)));
}

void OutputPane::reRunRunControl()
{
    const int index = currentIndex();
    QTC_ASSERT(index != -1 && !m_runControlTabs.at(index).runControl->isRunning(), return;)

    RunControlTab &tab = m_runControlTabs[index];

    tab.window->handleOldOutput();
    tab.window->scrollToBottom();
    tab.runControl->start();
}

void OutputPane::stopRunControl()
{
    const int index = currentIndex();
    QTC_ASSERT(index != -1 && m_runControlTabs.at(index).runControl->isRunning(), return;)

    RunControl *rc = m_runControlTabs.at(index).runControl;
    if (rc->isRunning() && optionallyPromptToStop(rc))
        rc->stop();

    if (debug)
        qDebug() << "OutputPane::stopRunControl " << rc;
}

bool OutputPane::closeTabs(CloseTabMode mode)
{
    bool allClosed = true;
    for (int t = m_tabWidget->count() - 1; t >= 0; t--)
        if (!closeTab(t, mode))
            allClosed = false;
    if (debug)
        qDebug() << "OutputPane::closeTabs() returns " << allClosed;
    return allClosed;
}

bool OutputPane::closeTab(int index)
{
    return closeTab(index, CloseTabWithPrompt);
}

bool OutputPane::closeTab(int tabIndex, CloseTabMode closeTabMode)
{
    const int index = indexOf(m_tabWidget->widget(tabIndex));
    QTC_ASSERT(index != -1, return true;)

    RunControlTab &tab = m_runControlTabs[index];

    if (debug)
            qDebug() << "OutputPane::closeTab tab " << tabIndex << tab.runControl
                        << tab.window << tab.asyncClosing;
    // Prompt user to stop
    if (tab.runControl->isRunning()) {
        switch (closeTabMode) {
        case CloseTabNoPrompt:
            break;
        case CloseTabWithPrompt:
            if (!tab.runControl->promptToStop())
                return false;
            break;
        }
        if (tab.runControl->stop() == RunControl::AsynchronousStop) {
            tab.asyncClosing = true;
            return false;
        }
    }

    m_tabWidget->removeTab(tabIndex);
    if (tab.asyncClosing) { // We were invoked from its finished() signal.
        tab.runControl->deleteLater();
    } else {
        delete tab.runControl;
    }
    delete tab.window;
    m_runControlTabs.removeAt(index);
    return true;
}

bool OutputPane::optionallyPromptToStop(RunControl *runControl)
{
    ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();
    ProjectExplorerSettings settings = pe->projectExplorerSettings();
    if (!runControl->promptToStop(&settings.prompToStopRunControl))
        return false;
    pe->setProjectExplorerSettings(settings);
    return true;
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
        const int index = indexOf(m_tabWidget->widget(i));
        QTC_ASSERT(index != -1, return; )

        RunControl *rc = m_runControlTabs.at(index).runControl;
        m_stopAction->setEnabled(rc->isRunning());
        m_reRunButton->setEnabled(!rc->isRunning());
        m_reRunButton->setIcon(m_runIcon);
    }
}

void OutputPane::runControlStarted()
{
    RunControl *current = currentRunControl();
    if (current && current == sender()) {
        m_reRunButton->setEnabled(false);
        m_stopAction->setEnabled(true);
        m_reRunButton->setIcon(m_runIcon);
    }
}

void OutputPane::runControlFinished()
{
    RunControl *senderRunControl = qobject_cast<RunControl *>(sender());
    const int senderIndex = indexOf(senderRunControl);

    QTC_ASSERT(senderIndex != -1, return; )

    // Enable buttons for current
    RunControl *current = currentRunControl();

    if (debug)
        qDebug() << "OutputPane::runControlFinished"  << senderRunControl << senderIndex
                    << " current " << current << m_runControlTabs.size();

    if (current && current == sender()) {
        m_reRunButton->setEnabled(true);
        m_stopAction->setEnabled(false);
        m_reRunButton->setIcon(m_runIcon);
    }
    // Check for asynchronous close. Close the tab.
    if (m_runControlTabs.at(senderIndex).asyncClosing)
        closeTab(tabWidgetIndexOf(senderIndex), CloseTabNoPrompt);

    if (!isRunning())
        emit allRunControlsFinished();
}

bool OutputPane::isRunning() const
{
    foreach(const RunControlTab &rt, m_runControlTabs)
        if (rt.runControl->isRunning())
            return true;
    return false;
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
    , m_formatter(0)
    , m_enforceNewline(false)
    , m_scrollToBottom(false)
    , m_linksActive(true)
    , m_mousePressed(false)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //setCenterOnScroll(false);
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);
    if (!ProjectExplorerPlugin::instance()->projectExplorerSettings().wrapAppOutput)
        setWordWrapMode(QTextOption::NoWrap);

    static uint usedIds = 0;
    Core::ICore *core = Core::ICore::instance();
    Core::Context context(Constants::C_APP_OUTPUT, usedIds++);
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

    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateWordWrapMode()));
}

OutputWindow::~OutputWindow()
{
    Core::ICore::instance()->removeContextObject(m_outputWindowContext);
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
    if (m_scrollToBottom) {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }
    m_scrollToBottom = false;
}

QString OutputWindow::doNewlineEnfocement(const QString &out)
{
    m_scrollToBottom = true;
    QString s = out;
    if (m_enforceNewline)
        s.prepend(QLatin1Char('\n'));

    m_enforceNewline = true; // make appendOutputInline put in a newline next time

    if (s.endsWith(QLatin1Char('\n')))
        s.chop(1);

    return s;
}

void OutputWindow::appendMessage(const QString &output, OutputFormat format)
{
    QString out = output;
    out.remove(QLatin1Char('\r'));
    setMaximumBlockCount(MaxBlockCount);
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
void OutputWindow::appendText(const QString &textIn, const QTextCharFormat &format, int maxLineCount)
{
    QString text = textIn;
    text.remove(QLatin1Char('\r'));
    if (document()->blockCount() > maxLineCount)
        return;
    const bool atBottom = isScrollbarAtBottom();
    QTextCursor cursor = QTextCursor(document());
    cursor.movePosition(QTextCursor::End);
    cursor.beginEditBlock();
    cursor.insertText(doNewlineEnfocement(text), format);

    if (document()->blockCount() > maxLineCount) {
        QTextCharFormat tmp;
        tmp.setFontWeight(QFont::Bold);
        cursor.insertText(tr("Additional output omitted\n"), tmp);
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

void OutputWindow::handleOldOutput()
{
    if (ProjectExplorerPlugin::instance()->projectExplorerSettings().cleanOldAppOutput)
        clear();
    else
        grayOutOldContent();
}

void OutputWindow::scrollToBottom()
{
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

void OutputWindow::updateWordWrapMode()
{
    if (ProjectExplorerPlugin::instance()->projectExplorerSettings().wrapAppOutput)
        setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    else
        setWordWrapMode(QTextOption::NoWrap);
}

} // namespace Internal
} // namespace ProjectExplorer

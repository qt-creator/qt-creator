/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "resultsview.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>

#include <QHeaderView>
#include <QResizeEvent>
#include <QTimer>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QToolButton>
#include <QAction>
#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include <QDebug>

enum
{
    ResultSize = 100,
    ResultPosition = 0,
    DetailsPosition = 1,
    ReasonPosition  = 2,
    ScreenPosition = 3
};

// Role for data which holds the identifier for use with m_pendingScreenshots
static const int ScreenshotIdRole = Qt::UserRole + 10;
// Role for the link to the .png file of a failure screenshot
static const int ScreenshotLinkRole = Qt::UserRole + 100;


/*
    Constructs a screenshot ID for the test failure at the given \a file and
    \a line.
*/
static QString screenshotId(const QString &file, int line)
{
    return QString::fromLatin1("%1 %2").arg(QFileInfo(file).canonicalFilePath()).arg(line);
}

ResultsView::ResultsView(QWidget *parent) :
    QTableWidget(parent),
    m_lastRow(-1),
    m_ignoreEvent(false),
    m_userLock(false),
    m_passBrush(QColor("lightgreen")),
    m_failBrush(QColor("orangered")),
    m_unexpectedBrush(QColor("orange"))
{
    setColumnCount(3);
    setGridStyle(Qt::NoPen);

    m_showPassing = m_testSettings.showPassedResults();
    m_showDebug = m_testSettings.showDebugResults();
    m_showSkipped = m_testSettings.showSkippedResults();

    setHorizontalHeaderLabels(QStringList() << tr("Result")
        << tr("Test Details") << tr("Description"));
    resize(width());

    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSortingEnabled(false);
    setAlternatingRowColors(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    verticalHeader()->hide();
    horizontalHeader()->show();

    connect(this, SIGNAL(currentCellChanged(int,int,int,int)),
        this, SLOT(onChanged()), Qt::DirectConnection);
    connect(this, SIGNAL(itemClicked(QTableWidgetItem*)),
        this, SLOT(onItemClicked(QTableWidgetItem*)), Qt::DirectConnection);

    setWordWrap(true);
}

ResultsView::~ResultsView()
{
}

void ResultsView::resize(int width)
{
    resizeColumnToContents(ResultPosition);
    if (columnWidth(ResultPosition) < ResultSize)
        setColumnWidth(ResultPosition, ResultSize);
    resizeColumnToContents(DetailsPosition);
    if (columnWidth(DetailsPosition) < ResultSize)
        setColumnWidth(DetailsPosition, ResultSize);
    setColumnWidth(DetailsPosition, columnWidth(DetailsPosition) + 20);
    setColumnWidth(ReasonPosition, width - columnWidth(ResultPosition) - columnWidth(DetailsPosition));
}

void ResultsView::resizeEvent(QResizeEvent *event)
{
    resize(event->size().width());
}

void ResultsView::clear()
{
    setUpdatesEnabled(false);
    disconnect(this, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(onChanged()));
    clearContents();
    setRowCount(0);
    connect(this, SIGNAL(currentCellChanged(int,int,int,int)),
        this, SLOT(onChanged()), Qt::DirectConnection);
    setUpdatesEnabled(true);

    m_ignoreEvent = false;
    m_userLock = false;
    m_failedTests.clear();
}

/*
    Called when the test runner tells us it has taken a screenshot due to
    failure.

    Note that, since we determine test failures by parsing output, this could happen
    before or after we see the actual test failure.
*/
void ResultsView::addScreenshot(const QString &screenshot, const QString &testfunction,
    const QString& file, int line)
{
    Q_UNUSED(testfunction);

    QString id = screenshotId(file, line);
    // We may have received the screenshot before the test result it matches.
    m_pendingScreenshots[id] = screenshot;
    updateScreenshots();
}

/*
    Iterates through the test results and generates links to screenshots
    for any that have a screenshot available.
*/
void ResultsView::updateScreenshots()
{
    for (int row = 0; row < rowCount(); ++row) {
        QTableWidgetItem *result = item(row, ResultPosition);
        if (!result)
            continue;
        // If there is a screenshot for this result, put a link to it in the table.
        QString id = result->data(ScreenshotIdRole).toString();
        if (!m_pendingScreenshots.contains(id))
            continue;

        QString screenshot = m_pendingScreenshots.take(id);
        QTableWidgetItem* shot = new QTableWidgetItem(QIcon(QPixmap(QLatin1String(":/testrun.png"))), QString());
        shot->setData(ScreenshotLinkRole, screenshot);
        shot->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        setItem(row, ScreenPosition, shot);
    }
}

/*
    Opens any screenshot linked to from \a item.
    Called when \a item is clicked.
*/
void ResultsView::onItemClicked(QTableWidgetItem* item)
{
    QString shot = item->data(ScreenshotLinkRole).toString();
    if (shot.isEmpty())
        return;
    // Open the screenshot using the preferred image viewer.
    QDesktopServices::openUrl(QUrl::fromLocalFile(shot));
}

void ResultsView::append(const QString &res, const QString &test, const QString &reason, const QString &dataTag, const QString &file, const QString &line)
{
    int row = rowCount();
    insertRow(row);

    QTableWidgetItem* result = new QTableWidgetItem(res);
    result->setTextAlignment(Qt::AlignCenter);
    if (res.startsWith(QLatin1String("PASS"))) {
        result->setBackground(m_passBrush);
        setRowHidden(row, !m_showPassing);
    } else if (res.startsWith(QLatin1String("FAIL"))) {
        result->setBackground(m_failBrush);
    } else if (res.startsWith(QLatin1String("QDEBUG"))) {
        setRowHidden(row, !m_showDebug);
    } else if (res.startsWith(QLatin1String("SKIP"))) {
        setRowHidden(row, !m_showSkipped);
    } else if (res.startsWith(QLatin1String("XFAIL")) || res.startsWith(QLatin1String("XPASS"))) {
        result->setBackground(m_unexpectedBrush);
    }

    if ((res.contains(QLatin1String("FAIL")) || res.startsWith(QLatin1String("XPASS"))) && !m_failedTests.contains(test))
        m_failedTests.append(test);

    // Construct a result id for use in mapping test failures to screenshots.
    QString resultId = screenshotId(file, line.toInt());
    result->setData(ScreenshotIdRole, resultId);

    QTableWidgetItem *testDetails = new QTableWidgetItem(formatTestDetails(test, dataTag));
    testDetails->setToolTip(formatLocation(file, line));
    setItem(row, ResultPosition, result);
    setItem(row, DetailsPosition, testDetails);
    setItem(row, ReasonPosition, new QTableWidgetItem(reason));
    resize(width());
    resizeRowToContents(row);

    if (currentRow() == -1)
        scrollToItem(item(row, ReasonPosition));

    m_resultsWindow->navigateStateChanged();
    updateScreenshots();
}

QString ResultsView::xmlDequote(const QString &input)
{
    QString result = input;
    return result.replace(QLatin1String("&gt;"), QLatin1String(">"))
        .replace(QLatin1String("&lt;"), QLatin1String("<"))
        .replace(QLatin1String("&apos;"), QLatin1String("'"))
        .replace(QLatin1String("&quot;"), QLatin1String("\""))
        .replace(QLatin1String("&amp;"), QLatin1String("&"))
        .replace(QLatin1String("&#x002D;"), QLatin1String("-"));
}

QString ResultsView::htmlQuote(const QString &input)
{
    QString result=input;
    return result.replace(QLatin1String("&"), QLatin1String("&amp;"))
        .replace(QLatin1String(">"), QLatin1String("&gt;"))
        .replace(QLatin1String("<"), QLatin1String("&lt;"))
        .replace(QLatin1String("\""), QLatin1String("&quot;"))
        .replace(QLatin1String("\n"), QLatin1String("<br />\n"));
}

QString ResultsView::formatTestDetails(const QString &test, const QString &dataTag)
{
    QString ret = test;
    if (!dataTag.isEmpty() && dataTag != QLatin1String("...")) {
        ret += QLatin1String(" (");
        ret += dataTag;
        ret += QLatin1Char(')');
    }
    return ret;
}

QString ResultsView::formatLocation(const QString &file, const QString &line)
{
    QString description;
    QString _file = file;
    if (_file.startsWith(QLatin1Char('[')))
        _file = _file.mid(1);
    if (!_file.isEmpty() || !line.isEmpty()) {
        description += xmlDequote(file);
        if (line !=  QLatin1String("-1")) {
            description += QLatin1Char(':');
            description += line;
        }
    }

    return description;
}

QString ResultsView::result(int row)
{
    if (row >= rowCount())
        return QString();

    return item(row, ResultPosition)->text().simplified();
}

QString ResultsView::reason(int row)
{
    if (row >= rowCount())
        return QString();

    QString txt = item(row, ReasonPosition)->text();
    const int pos = txt.indexOf(QLatin1Char('\n'));
    if (pos > 0)
        txt.truncate(pos);
    return txt.simplified();
}

QString ResultsView::location(int row)
{
    if (row >= rowCount())
        return QString();

    return item(row, DetailsPosition)->toolTip();
}

QString ResultsView::file(int row)
{
    QString txt = location(row);
    if (!txt.isEmpty()) {
        const int pos = txt.indexOf(QLatin1Char(':'));
        if (pos >= 0)
            txt.truncate(pos);
        return txt.simplified();
    }
    return QString();
}

QString ResultsView::line(int row)
{
    QString txt = location(row);
    if (!txt.isEmpty()) {
        const int pos = txt.indexOf(QLatin1Char(':'));
        if (pos >= 0) {
            txt = txt.mid(pos + 1);
        } else {
            txt.clear();
        }
        return txt.simplified();
    }
    return QString();
}

int ResultsView::intLine(int row)
{
    if (row >= rowCount())
        return false;

    return line(row).toInt();
}

void ResultsView::setResult(const QString &result, const QString &test, const QString &reason, const QString &dataTag, const QString &file, int line)
{
    QString lineStr(QString::number(line));
    append(result, test, reason, dataTag, file, lineStr);
}

void ResultsView::onChanged()
{
    if (!m_ignoreEvent) {
        if (currentColumn() == 0)
            m_userLock = false;
        else
            m_userLock = true;
    }

    int row = currentRow();
    m_lastRow = row;

    if (m_ignoreEvent || row >= rowCount())
        return;

    m_resultsWindow->navigateStateChanged();
    QTimer::singleShot(0, this, SLOT(emitCurSelection()));
}

void ResultsView::emitCurSelection()
{
    emitSelection(m_lastRow);
}

void ResultsView::emitSelection(int row)
{
    if (row < 0)
        return;

    TestCaseRec rec;
    rec.m_testFunction = "";
    rec.m_line = intLine(row);
    TestCode *tmp = m_testCollection.findCode(file(row), "", "");
    rec.m_code = tmp;

    emit defectSelected(rec);
}

void ResultsView::setResultsWindow(TestResultsWindow *window)
{
    m_resultsWindow = window;
}

TestResultsWindow* ResultsView::resultsWindow() const
{
    return m_resultsWindow;
}

void ResultsView::showPassing(bool show)
{
    m_showPassing = show;
    updateHidden(QLatin1String("PASS"), show);
    m_testSettings.setShowPassedResults(show);
}

void ResultsView::showDebugMessages(bool show)
{
    m_showDebug = show;
    updateHidden(QLatin1String("QDEBUG"), show);
    m_testSettings.setShowDebugResults(show);
}

void ResultsView::showSkipped(bool show)
{
    m_showSkipped = show;
    updateHidden(QLatin1String("SKIP"), show);
    m_testSettings.setShowSkippedResults(show);
}

void ResultsView::updateHidden(const QString &result, bool show)
{
    for (int row = 0; row < rowCount(); ++row) {
        QTableWidgetItem *resultItem = item(row, ResultPosition);
        if (resultItem && resultItem->text().startsWith(result))
            setRowHidden(row, !show);
    }
    resize(width());
    m_resultsWindow->navigateStateChanged();
}

// **********************************************************

ResultsView *testResultsPane()
{
    return TestResultsWindow::instance()->resultsView();
}

TestResultsWindow *_testResultsInstance = 0;

TestResultsWindow *TestResultsWindow::instance()
{
    if (!_testResultsInstance) {
        _testResultsInstance = new TestResultsWindow();
    }
    return _testResultsInstance;
}

TestResultsWindow::TestResultsWindow() :
    m_stopAction(new QAction(QIcon(QLatin1String(ProjectExplorer::Constants::ICON_STOP)),
        tr("Stop Testing"), this)),
    m_retryAction(new QAction(QIcon(QLatin1String(":/reload.png")),
        tr("Retry Failed Tests"), this)),
    m_copyAction(new QAction(QIcon(QLatin1String(Core::Constants::ICON_COPY)),
        tr("Copy Results"), this)),
    m_stopButton(new QToolButton),
    m_retryButton(new QToolButton),
    m_copyButton(new QToolButton),
    m_filterButton(new QToolButton)
{
    m_resultsView = new ResultsView;
    m_resultsView->setResultsWindow(this);
    m_resultsView->setFrameStyle(QFrame::NoFrame);

    connect(&m_testSettings, SIGNAL(changed()), this, SLOT(onSettingsChanged()));

    m_stopAction->setToolTip(tr("Stop Testing"));
    m_stopAction->setEnabled(false);

    m_retryAction->setToolTip(tr("Retry Failed Tests"));
    m_retryAction->setEnabled(false);

    m_copyAction->setToolTip(tr("Copy Results"));
    m_copyAction->setEnabled(false);

    m_filterButton->setIcon(QIcon(Core::Constants::ICON_FILTER));
    m_filterButton->setToolTip(tr("Filter Results"));
    m_filterButton->setPopupMode(QToolButton::InstantPopup);

    m_showPassingAction = new QAction(tr("Show Passing Tests"), this);
    m_showPassingAction->setCheckable(true);
    m_showPassingAction->setChecked(m_testSettings.showPassedResults());

    connect(m_showPassingAction, SIGNAL(toggled(bool)), m_resultsView, SLOT(showPassing(bool)));

    m_showDebugAction = new QAction(tr("Show Debug Messages"), this);
    m_showDebugAction->setCheckable(true);
    m_showDebugAction->setChecked(m_testSettings.showDebugResults());
    connect(m_showDebugAction, SIGNAL(toggled(bool)), m_resultsView, SLOT(showDebugMessages(bool)));

    m_showSkipAction = new QAction(tr("Show Skipped Tests"), this);
    m_showSkipAction->setCheckable(true);
    m_showSkipAction->setChecked(m_testSettings.showSkippedResults());
    connect(m_showSkipAction, SIGNAL(toggled(bool)), m_resultsView, SLOT(showSkipped(bool)));

    QMenu *filterMenu = new QMenu(m_filterButton);
    filterMenu->addAction(m_showPassingAction);
    filterMenu->addAction(m_showDebugAction);
    filterMenu->addAction(m_showSkipAction);
    m_filterButton->setMenu(filterMenu);

    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    Core::Context globalcontext(Core::Constants::C_GLOBAL);
    Core::Command *stopCmd = am->registerAction(m_stopAction,
        Core::Id("Qt4Test.StopTest"), globalcontext);
    Core::Command *retryCmd = am->registerAction(m_retryAction,
        Core::Id("Qt4Test.RetryFailed"), globalcontext);
    Core::Command *copyCmd = am->registerAction(m_copyAction,
        Core::Id("Qt4Test.CopyResults"), globalcontext);

    m_stopButton->setDefaultAction(stopCmd->action());
    m_stopButton->setAutoRaise(true);
    m_retryButton->setDefaultAction(retryCmd->action());
    m_retryButton->setAutoRaise(true);
    m_copyButton->setDefaultAction(copyCmd->action());
    m_copyButton->setAutoRaise(true);

    connect(m_stopAction, SIGNAL(triggered()), this, SIGNAL(stopTest()));
    connect(m_retryAction, SIGNAL(triggered()), this, SLOT(retryFailed()));
    connect(m_copyAction, SIGNAL(triggered()), this, SLOT(copyResults()));
}

TestResultsWindow::~TestResultsWindow()
{
    delete m_resultsView;
    _testResultsInstance = 0;
}

bool TestResultsWindow::hasFocus()
{
    return m_resultsView->hasFocus();
}

bool TestResultsWindow::canFocus()
{
    return true;
}

void TestResultsWindow::setFocus()
{
    m_resultsView->setFocus();
}

void TestResultsWindow::clearContents()
{
    m_resultsView->clear();
    m_retryAction->setEnabled(false);
    m_copyAction->setEnabled(false);
    navigateStateChanged();
}

QWidget *TestResultsWindow::outputWidget(QWidget *parent)
{
    m_resultsView->setParent(parent);
    return m_resultsView;
}

QString TestResultsWindow::name() const
{
    return tr("Test Results");
}

void TestResultsWindow::visibilityChanged(bool /*b*/)
{
}

int TestResultsWindow::priorityInStatusBar() const
{
    return 50;
}

bool TestResultsWindow::canNext()
{
    for (int i = m_resultsView->currentRow() + 1; i < m_resultsView->rowCount(); ++i) {
        if (!m_resultsView->isRowHidden(i))
            return true;
    }
    return false;
}

bool TestResultsWindow::canPrevious()
{
    for (int i = m_resultsView->currentRow() - 1; i >= 0; --i) {
        if (!m_resultsView->isRowHidden(i))
            return true;
    }
    return false;
}

void TestResultsWindow::goToNext()
{
    for (int i = m_resultsView->currentRow() + 1; i < m_resultsView->rowCount(); ++i) {
        if (!m_resultsView->isRowHidden(i)) {
            m_resultsView->setCurrentCell(i, 0);
            return;
        }
    }
}

void TestResultsWindow::goToPrev()
{
    for (int i = m_resultsView->currentRow() - 1; i >= 0; --i) {
      if (!m_resultsView->isRowHidden(i)) {
            m_resultsView->setCurrentCell(i, 0);
            return;
        }
    }
}

bool TestResultsWindow::canNavigate()
{
    return true;
}

QList<QWidget*> TestResultsWindow::toolBarWidgets() const
{
    return QList<QWidget*>() << m_filterButton << m_stopButton << m_retryButton << m_copyButton;
}

void TestResultsWindow::addResult(const QString &result, const QString &test, const QString &reason,
    const QString &dataTag, const QString &file, int line)
{
    m_resultsView->setResult(result, test, reason, dataTag, file, line);
}

void TestResultsWindow::onTestStarted()
{
    m_stopAction->setEnabled(true);
    m_retryAction->setEnabled(false);
    m_copyAction->setEnabled(false);
}

void TestResultsWindow::onTestStopped()
{
    m_stopAction->setEnabled(false);
    m_retryAction->setEnabled(false);
    m_copyAction->setEnabled(false);
}

void TestResultsWindow::onTestFinished()
{
    m_stopAction->setEnabled(false);
    m_retryAction->setEnabled(!m_resultsView->failedTests().isEmpty());
    m_copyAction->setEnabled(true);
}

void TestResultsWindow::retryFailed()
{
    emit retryFailedTests(m_resultsView->failedTests());
}

void TestResultsWindow::copyResults()
{
    m_resultsView->copyResults();
}

void ResultsView::copyResults()
{
    QMimeData *md = new QMimeData();
    QString html = QLatin1String("<html><table>");
    QString text;
    for (int row = 0; row < rowCount(); ++row) {
        QString result = item(row, ResultPosition)->text().trimmed();
        QString detail = item(row, DetailsPosition)->text();
        QString location = item(row, DetailsPosition)->toolTip();
        QString reason = item(row, ReasonPosition)->text();
        html += QString::fromLatin1("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>")
            .arg(result).arg(detail).arg(location).arg(htmlQuote(reason));
        text += QString::fromLatin1("%1\n%2\n%3\n%4\n").arg(result).arg(detail).arg(location).arg(reason);
    }
    html += QLatin1String("</table></html>");
    md->setHtml(html);
    md->setText(text);
    QApplication::clipboard()->setMimeData(md);
}

void TestResultsWindow::onSettingsChanged()
{
    if (m_showPassingAction->isChecked() != m_testSettings.showPassedResults())
        m_showPassingAction->setChecked(m_testSettings.showPassedResults());
    if (m_showDebugAction->isChecked() != m_testSettings.showDebugResults())
        m_showDebugAction->setChecked(m_testSettings.showDebugResults());
    if (m_showSkipAction->isChecked() != m_testSettings.showSkippedResults())
        m_showSkipAction->setChecked(m_testSettings.showSkippedResults());
}

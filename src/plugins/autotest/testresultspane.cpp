/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "autotestplugin.h"
#include "autotesticons.h"
#include "testresultspane.h"
#include "testresultmodel.h"
#include "testresultdelegate.h"
#include "testrunner.h"
#include "testsettings.h"
#include "testtreemodel.h"
#include "testcodeparser.h"
#include "testeditormark.h"
#include "testoutputreader.h"

#include <aggregation/aggregate.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/basetextfind.h>
#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

using namespace Core;

namespace Autotest {
namespace Internal {

ResultsTreeView::ResultsTreeView(QWidget *parent)
    : Utils::TreeView(parent)
{
    setAttribute(Qt::WA_MacShowFocusRect, false);
}

void ResultsTreeView::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Copy)) {
        emit copyShortcutTriggered();
        event->accept();
    }
    TreeView::keyPressEvent(event);
}

TestResultsPane::TestResultsPane(QObject *parent) :
    IOutputPane(parent),
    m_context(new IContext(this))
{
    m_outputWidget = new QStackedWidget;
    QWidget *visualOutputWidget = new QWidget;
    m_outputWidget->addWidget(visualOutputWidget);
    QVBoxLayout *outputLayout = new QVBoxLayout;
    outputLayout->setContentsMargins(0, 0, 0, 0);
    outputLayout->setSpacing(0);
    visualOutputWidget->setLayout(outputLayout);

    QPalette pal;
    pal.setColor(QPalette::Window,
                 Utils::creatorTheme()->color(Utils::Theme::InfoBarBackground));
    pal.setColor(QPalette::WindowText,
                 Utils::creatorTheme()->color(Utils::Theme::InfoBarText));
    m_summaryWidget = new QFrame;
    m_summaryWidget->setPalette(pal);
    m_summaryWidget->setAutoFillBackground(true);
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(6, 6, 6, 6);
    m_summaryWidget->setLayout(layout);
    m_summaryLabel = new QLabel;
    m_summaryLabel->setPalette(pal);
    layout->addWidget(m_summaryLabel);
    m_summaryWidget->setVisible(false);

    outputLayout->addWidget(m_summaryWidget);

    m_treeView = new ResultsTreeView(visualOutputWidget);
    m_treeView->setHeaderHidden(true);
    m_treeView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    pal = m_treeView->palette();
    pal.setColor(QPalette::Base, pal.window().color());
    m_treeView->setPalette(pal);
    m_model = new TestResultModel(this);
    m_filterModel = new TestResultFilterModel(m_model, this);
    m_filterModel->setDynamicSortFilter(true);
    m_treeView->setModel(m_filterModel);
    TestResultDelegate *trd = new TestResultDelegate(this);
    m_treeView->setItemDelegate(trd);

    outputLayout->addWidget(ItemViewFind::createSearchableWrapper(m_treeView));

    m_textOutput = new QPlainTextEdit;
    m_textOutput->setPalette(pal);
    m_textOutput->setFont(TextEditor::TextEditorSettings::fontSettings().font());
    m_textOutput->setWordWrapMode(QTextOption::WordWrap);
    m_textOutput->setReadOnly(true);
    m_outputWidget->addWidget(m_textOutput);

    auto agg = new Aggregation::Aggregate;
    agg->add(m_textOutput);
    agg->add(new BaseTextFind(m_textOutput));

    createToolButtons();

    connect(m_treeView, &Utils::TreeView::activated, this, &TestResultsPane::onItemActivated);
    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged,
            trd, &TestResultDelegate::currentChanged);
    connect(m_treeView, &Utils::TreeView::customContextMenuRequested,
            this, &TestResultsPane::onCustomContextMenuRequested);
    connect(m_treeView, &ResultsTreeView::copyShortcutTriggered, [this] () {
        onCopyItemTriggered(getTestResult(m_treeView->currentIndex()));
    });
    connect(m_model, &TestResultModel::requestExpansion, [this] (QModelIndex idx) {
        m_treeView->expand(m_filterModel->mapFromSource(idx));
    });
    connect(TestRunner::instance(), &TestRunner::testRunStarted,
            this, &TestResultsPane::onTestRunStarted);
    connect(TestRunner::instance(), &TestRunner::testRunFinished,
            this, &TestResultsPane::onTestRunFinished);
    connect(TestRunner::instance(), &TestRunner::testResultReady,
            this, &TestResultsPane::addTestResult);
    connect(TestRunner::instance(), &TestRunner::hadDisabledTests,
            m_model, &TestResultModel::raiseDisabledTests);
}

void TestResultsPane::createToolButtons()
{
    m_expandCollapse = new QToolButton(m_treeView);
    m_expandCollapse->setIcon(Utils::Icons::EXPAND_ALL_TOOLBAR.icon());
    m_expandCollapse->setToolTip(tr("Expand All"));
    m_expandCollapse->setCheckable(true);
    m_expandCollapse->setChecked(false);
    connect(m_expandCollapse, &QToolButton::clicked, [this] (bool checked) {
        if (checked)
            m_treeView->expandAll();
        else
            m_treeView->collapseAll();
    });

    m_runAll = new QToolButton(m_treeView);
    m_runAll->setDefaultAction(ActionManager::command(Constants::ACTION_RUN_ALL_ID)->action());

    m_runSelected = new QToolButton(m_treeView);
    m_runSelected->setDefaultAction(ActionManager::command(Constants::ACTION_RUN_SELECTED_ID)->action());

    m_runFile = new QToolButton(m_treeView);
    m_runFile->setDefaultAction(ActionManager::command(Constants::ACTION_RUN_FILE_ID)->action());

    m_stopTestRun = new QToolButton(m_treeView);
    m_stopTestRun->setIcon(Utils::Icons::STOP_SMALL_TOOLBAR.icon());
    m_stopTestRun->setToolTip(tr("Stop Test Run"));
    m_stopTestRun->setEnabled(false);
    connect(m_stopTestRun, &QToolButton::clicked, TestRunner::instance(), &TestRunner::requestStopTestRun);

    m_filterButton = new QToolButton(m_treeView);
    m_filterButton->setIcon(Utils::Icons::FILTER.icon());
    m_filterButton->setToolTip(tr("Filter Test Results"));
    m_filterButton->setProperty("noArrow", true);
    m_filterButton->setPopupMode(QToolButton::InstantPopup);
    m_filterMenu = new QMenu(m_filterButton);
    initializeFilterMenu();
    connect(m_filterMenu, &QMenu::triggered, this, &TestResultsPane::filterMenuTriggered);
    m_filterButton->setMenu(m_filterMenu);
    m_outputToggleButton = new QToolButton(m_treeView);
    m_outputToggleButton->setIcon(Icons::TEXT_DISPLAY.icon());
    m_outputToggleButton->setToolTip(tr("Switch Between Visual and Text Display"));
    m_outputToggleButton->setEnabled(true);
    connect(m_outputToggleButton, &QToolButton::clicked, this, &TestResultsPane::toggleOutputStyle);
}

static TestResultsPane *s_instance = nullptr;

TestResultsPane *TestResultsPane::instance()
{
    if (!s_instance)
        s_instance = new TestResultsPane;
    return s_instance;
}

TestResultsPane::~TestResultsPane()
{
    delete m_treeView;
    if (!m_outputWidget->parent())
        delete m_outputWidget;
    s_instance = nullptr;
}

void TestResultsPane::addTestResult(const TestResultPtr &result)
{
    const QScrollBar *scrollBar = m_treeView->verticalScrollBar();
    m_atEnd = scrollBar ? scrollBar->value() == scrollBar->maximum() : true;

    m_model->addTestResult(result, m_expandCollapse->isChecked());
    setIconBadgeNumber(m_model->resultTypeCount(ResultType::Fail)
                       + m_model->resultTypeCount(ResultType::MessageFatal)
                       + m_model->resultTypeCount(ResultType::UnexpectedPass));
    flash();
    navigateStateChanged();
}

static void checkAndFineTuneColors(QTextCharFormat *format)
{
    QTC_ASSERT(format, return);
    const QColor bgColor = format->background().color();
    QColor fgColor = format->foreground().color();

    if (Utils::StyleHelper::isReadableOn(bgColor, fgColor))
        return;

    int h, s, v;
    fgColor.getHsv(&h, &s, &v);
    // adjust the color value to ensure better readability
    if (Utils::StyleHelper::luminance(bgColor) < .5)
        v = v + 64;
    else
        v = v - 64;

    fgColor.setHsv(h, s, v);
    if (!Utils::StyleHelper::isReadableOn(bgColor, fgColor)) {
        s = (s + 128) % 255;    // adjust the saturation to ensure better readability
        fgColor.setHsv(h, s, v);
        if (!Utils::StyleHelper::isReadableOn(bgColor, fgColor))
            return;
    }

    format->setForeground(fgColor);
}

void TestResultsPane::addOutputLine(const QByteArray &outputLine, OutputChannel channel)
{
    if (!QTC_GUARD(!outputLine.contains('\n'))) {
        for (const auto &line : outputLine.split('\n'))
            addOutputLine(line, channel);
        return;
    }

    const Utils::FormattedText formattedText
            = Utils::FormattedText{QString::fromUtf8(outputLine), m_defaultFormat};
    QList<Utils::FormattedText> formatted = channel == OutputChannel::StdOut
            ? m_stdOutHandler.parseText(formattedText)
            : m_stdErrHandler.parseText(formattedText);

    QTextCursor cursor = m_textOutput->textCursor();
    cursor.beginEditBlock();
    for (auto formattedText : formatted) {
        checkAndFineTuneColors(&formattedText.format);
        cursor.insertText(formattedText.text, formattedText.format);
    }
    cursor.insertText("\n");
    cursor.endEditBlock();
}

QWidget *TestResultsPane::outputWidget(QWidget *parent)
{
    if (m_outputWidget) {
        m_outputWidget->setParent(parent);
    } else {
        qDebug() << "This should not happen...";
    }
    return m_outputWidget;
}

QList<QWidget *> TestResultsPane::toolBarWidgets() const
{
    return {m_expandCollapse, m_runAll, m_runSelected, m_runFile, m_stopTestRun,
            m_outputToggleButton, m_filterButton};
}

QString TestResultsPane::displayName() const
{
    return tr("Test Results");
}

int TestResultsPane::priorityInStatusBar() const
{
    return -666;
}

void TestResultsPane::clearContents()
{
    m_filterModel->clearTestResults();
    if (auto delegate = qobject_cast<TestResultDelegate *>(m_treeView->itemDelegate()))
        delegate->clearCache();
    setIconBadgeNumber(0);
    navigateStateChanged();
    m_summaryWidget->setVisible(false);
    m_autoScroll = AutotestPlugin::settings()->autoScroll;
    connect(m_treeView->verticalScrollBar(), &QScrollBar::rangeChanged,
            this, &TestResultsPane::onScrollBarRangeChanged, Qt::UniqueConnection);
    m_textOutput->clear();
    m_defaultFormat.setBackground(Utils::creatorTheme()->palette().color(
                                      m_textOutput->backgroundRole()));
    m_defaultFormat.setForeground(Utils::creatorTheme()->palette().color(
                                      m_textOutput->foregroundRole()));

    // in case they had been forgotten to reset
    m_stdErrHandler.endFormatScope();
    m_stdOutHandler.endFormatScope();
    clearMarks();
}

void TestResultsPane::visibilityChanged(bool /*visible*/)
{
}

void TestResultsPane::setFocus()
{
}

bool TestResultsPane::hasFocus() const
{
    return m_treeView->hasFocus();
}

bool TestResultsPane::canFocus() const
{
    return true;
}

bool TestResultsPane::canNavigate() const
{
    return true;
}

bool TestResultsPane::canNext() const
{
    return m_filterModel->hasResults();
}

bool TestResultsPane::canPrevious() const
{
    return m_filterModel->hasResults();
}

void TestResultsPane::goToNext()
{
    if (!canNext())
        return;

    const QModelIndex currentIndex = m_treeView->currentIndex();
    QModelIndex nextCurrentIndex;

    if (currentIndex.isValid()) {
        // try to set next to first child or next sibling
        if (m_filterModel->rowCount(currentIndex)) {
            nextCurrentIndex = m_filterModel->index(0, 0, currentIndex);
        } else {
            nextCurrentIndex = currentIndex.sibling(currentIndex.row() + 1, 0);
            // if it had no sibling check siblings of parent (and grandparents if necessary)
            if (!nextCurrentIndex.isValid()) {

                QModelIndex parent = currentIndex.parent();
                do {
                    if (!parent.isValid())
                        break;
                    nextCurrentIndex = parent.sibling(parent.row() + 1, 0);
                    parent = parent.parent();
                } while (!nextCurrentIndex.isValid());
            }
        }
    }

    // if we have no current or could not find a next one, use the first item of the whole tree
    if (!nextCurrentIndex.isValid()) {
        Utils::TreeItem *rootItem = m_model->itemForIndex(QModelIndex());
        // if the tree does not contain any item - don't do anything
        if (!rootItem || !rootItem->childCount())
            return;

        nextCurrentIndex = m_filterModel->mapFromSource(m_model->indexForItem(rootItem->childAt(0)));
    }

    m_treeView->setCurrentIndex(nextCurrentIndex);
    onItemActivated(nextCurrentIndex);
}

void TestResultsPane::goToPrev()
{
    if (!canPrevious())
        return;

    const QModelIndex currentIndex = m_treeView->currentIndex();
    QModelIndex nextCurrentIndex;

    if (currentIndex.isValid()) {
        // try to set next to prior sibling or parent
        if (currentIndex.row() > 0) {
            nextCurrentIndex = currentIndex.sibling(currentIndex.row() - 1, 0);
            // if the sibling has children, use the last one
            while (int rowCount = m_filterModel->rowCount(nextCurrentIndex))
                nextCurrentIndex = m_filterModel->index(rowCount - 1, 0, nextCurrentIndex);
        } else {
            nextCurrentIndex = currentIndex.parent();
        }
    }

    // if we have no current or didn't find a sibling/parent use the last item of the whole tree
    if (!nextCurrentIndex.isValid()) {
        const QModelIndex rootIdx = m_filterModel->index(0, 0);
        // if the tree does not contain any item - don't do anything
        if (!rootIdx.isValid())
            return;

        // get the last (visible) top level index
        nextCurrentIndex = m_filterModel->index(m_filterModel->rowCount(QModelIndex()) - 1, 0);
        // step through until end
        while (int rowCount = m_filterModel->rowCount(nextCurrentIndex))
            nextCurrentIndex = m_filterModel->index(rowCount - 1, 0, nextCurrentIndex);
    }

    m_treeView->setCurrentIndex(nextCurrentIndex);
    onItemActivated(nextCurrentIndex);
}

void TestResultsPane::onItemActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    const TestResult *testResult = m_filterModel->testResult(index);
    if (testResult && !testResult->fileName().isEmpty())
        EditorManager::openEditorAt(testResult->fileName(), testResult->line(), 0);
}

void TestResultsPane::onRunAllTriggered()
{
    TestRunner *runner = TestRunner::instance();
    runner->setSelectedTests(TestTreeModel::instance()->getAllTestCases());
    runner->prepareToRunTests(TestRunMode::Run);
}

void TestResultsPane::onRunSelectedTriggered()
{
    TestRunner *runner = TestRunner::instance();
    runner->setSelectedTests(TestTreeModel::instance()->getSelectedTests());
    runner->prepareToRunTests(TestRunMode::Run);
}

void TestResultsPane::initializeFilterMenu()
{
    const bool omitIntern = AutotestPlugin::settings()->omitInternalMssg;
    // FilterModel has all messages enabled by default
    if (omitIntern)
        m_filterModel->toggleTestResultType(ResultType::MessageInternal);

    QMap<ResultType, QString> textAndType;
    textAndType.insert(ResultType::Pass, tr("Pass"));
    textAndType.insert(ResultType::Fail, tr("Fail"));
    textAndType.insert(ResultType::ExpectedFail, tr("Expected Fail"));
    textAndType.insert(ResultType::UnexpectedPass, tr("Unexpected Pass"));
    textAndType.insert(ResultType::Skip, tr("Skip"));
    textAndType.insert(ResultType::Benchmark, tr("Benchmarks"));
    textAndType.insert(ResultType::MessageDebug, tr("Debug Messages"));
    textAndType.insert(ResultType::MessageWarn, tr("Warning Messages"));
    textAndType.insert(ResultType::MessageInternal, tr("Internal Messages"));
    for (ResultType result : textAndType.keys()) {
        QAction *action = new QAction(m_filterMenu);
        action->setText(textAndType.value(result));
        action->setCheckable(true);
        action->setChecked(result != ResultType::MessageInternal || !omitIntern);
        action->setData(int(result));
        m_filterMenu->addAction(action);
    }
    m_filterMenu->addSeparator();
    QAction *action = new QAction(tr("Check All Filters"), m_filterMenu);
    m_filterMenu->addAction(action);
    connect(action, &QAction::triggered, this, [this]() { TestResultsPane::checkAllFilter(true); });
    action = new QAction(tr("Uncheck All Filters"), m_filterMenu);
    m_filterMenu->addAction(action);
    connect(action, &QAction::triggered, this, [this]() { TestResultsPane::checkAllFilter(false); });
}

void TestResultsPane::updateSummaryLabel()
{
    QString labelText = QString("<p>");
    labelText.append(tr("Test summary"));
    labelText.append(":&nbsp;&nbsp; ");
    int count = m_model->resultTypeCount(ResultType::Pass);
    labelText += QString::number(count) + ' ' + tr("passes");
    count = m_model->resultTypeCount(ResultType::Fail);
    labelText += ", " + QString::number(count) + ' ' + tr("fails");
    count = m_model->resultTypeCount(ResultType::UnexpectedPass);
    if (count)
        labelText += ", " + QString::number(count) + ' ' + tr("unexpected passes");
    count = m_model->resultTypeCount(ResultType::ExpectedFail);
    if (count)
        labelText += ", " + QString::number(count) + ' ' + tr("expected fails");
    count = m_model->resultTypeCount(ResultType::MessageFatal);
    if (count)
        labelText += ", " + QString::number(count) + ' ' + tr("fatals");
    count = m_model->resultTypeCount(ResultType::BlacklistedFail)
            + m_model->resultTypeCount(ResultType::BlacklistedXFail)
            + m_model->resultTypeCount(ResultType::BlacklistedPass)
            + m_model->resultTypeCount(ResultType::BlacklistedXPass);
    if (count)
        labelText += ", " + QString::number(count) + ' ' + tr("blacklisted");
    count = m_model->resultTypeCount(ResultType::Skip);
    if (count)
        labelText += ", " + QString::number(count) + ' ' + tr("skipped");
    count = m_model->disabledTests();
    if (count)
        labelText += ", " + QString::number(count) + ' ' + tr("disabled");
    labelText.append(".</p>");
    m_summaryLabel->setText(labelText);
}

void TestResultsPane::checkAllFilter(bool checked)
{
    for (QAction *action : m_filterMenu->actions()) {
        if (action->isCheckable())
            action->setChecked(checked);
    }
    m_filterModel->enableAllResultTypes(checked);
}

void TestResultsPane::filterMenuTriggered(QAction *action)
{
    m_filterModel->toggleTestResultType(TestResult::toResultType(action->data().value<int>()));
    navigateStateChanged();
}

void TestResultsPane::onTestRunStarted()
{
    m_testRunning = true;
    m_stopTestRun->setEnabled(true);
    AutotestPlugin::updateMenuItemsEnabledState();
    m_summaryWidget->setVisible(false);
}

static bool hasFailedTests(const TestResultModel *model)
{
    return (model->resultTypeCount(ResultType::Fail) > 0
            || model->resultTypeCount(ResultType::MessageFatal) > 0
            || model->resultTypeCount(ResultType::UnexpectedPass) > 0);
}

void TestResultsPane::onTestRunFinished()
{
    m_testRunning = false;
    m_stopTestRun->setEnabled(false);

    AutotestPlugin::updateMenuItemsEnabledState();
    updateSummaryLabel();
    m_summaryWidget->setVisible(true);
    m_model->removeCurrentTestMessage();
    disconnect(m_treeView->verticalScrollBar(), &QScrollBar::rangeChanged,
               this, &TestResultsPane::onScrollBarRangeChanged);
    if (AutotestPlugin::settings()->popupOnFinish
            && (!AutotestPlugin::settings()->popupOnFail || hasFailedTests(m_model))) {
        popup(IOutputPane::NoModeSwitch);
    }
    createMarks();
}

void TestResultsPane::onScrollBarRangeChanged(int, int max)
{
    if (m_autoScroll && m_atEnd)
        m_treeView->verticalScrollBar()->setValue(max);
}

void TestResultsPane::onCustomContextMenuRequested(const QPoint &pos)
{
    const bool resultsAvailable = m_filterModel->hasResults();
    const bool enabled = !m_testRunning && resultsAvailable;
    const TestResult *clicked = getTestResult(m_treeView->indexAt(pos));
    QMenu menu;

    QAction *action = new QAction(tr("Copy"), &menu);
    action->setShortcut(QKeySequence(QKeySequence::Copy));
    action->setEnabled(resultsAvailable && clicked);
    connect(action, &QAction::triggered, [this, clicked] () {
       onCopyItemTriggered(clicked);
    });
    menu.addAction(action);

    action = new QAction(tr("Copy All"), &menu);
    action->setEnabled(enabled);
    connect(action, &QAction::triggered, this, &TestResultsPane::onCopyWholeTriggered);
    menu.addAction(action);

    action = new QAction(tr("Save Output to File..."), &menu);
    action->setEnabled(enabled);
    connect(action, &QAction::triggered, this, &TestResultsPane::onSaveWholeTriggered);
    menu.addAction(action);

    const auto correlatingItem = (enabled && clicked) ? clicked->findTestTreeItem() : nullptr;
    action = new QAction(tr("Run This Test"), &menu);
    action->setEnabled(correlatingItem && correlatingItem->canProvideTestConfiguration());
    connect(action, &QAction::triggered, this, [this, clicked] {
        onRunThisTestTriggered(TestRunMode::Run, clicked);
    });
    menu.addAction(action);

    action = new QAction(tr("Debug This Test"), &menu);
    action->setEnabled(correlatingItem && correlatingItem->canProvideDebugConfiguration());
    connect(action, &QAction::triggered, this, [this, clicked] {
        onRunThisTestTriggered(TestRunMode::Debug, clicked);
    });
    menu.addAction(action);

    menu.exec(m_treeView->mapToGlobal(pos));
}

const TestResult *TestResultsPane::getTestResult(const QModelIndex &idx)
{
    if (!idx.isValid())
        return nullptr;

    const TestResult *result = m_filterModel->testResult(idx);
    QTC_CHECK(result);

    return result;
}

void TestResultsPane::onCopyItemTriggered(const TestResult *result)
{
    QTC_ASSERT(result, return);
    QApplication::clipboard()->setText(result->outputString(true));
}

void TestResultsPane::onCopyWholeTriggered()
{
    QApplication::clipboard()->setText(getWholeOutput());
}

void TestResultsPane::onSaveWholeTriggered()
{
    const QString fileName = QFileDialog::getSaveFileName(ICore::dialogParent(),
                                                          tr("Save Output To"));
    if (fileName.isEmpty())
        return;

    Utils::FileSaver saver(fileName, QIODevice::Text);
    if (!saver.write(getWholeOutput().toUtf8()) || !saver.finalize()) {
        QMessageBox::critical(ICore::dialogParent(), tr("Error"),
                              tr("Failed to write \"%1\".\n\n%2").arg(fileName)
                              .arg(saver.errorString()));
    }
}

void TestResultsPane::onRunThisTestTriggered(TestRunMode runMode, const TestResult *result)
{
    QTC_ASSERT(result, return);

    const TestTreeItem *item = result->findTestTreeItem();

    if (item)
        TestRunner::instance()->runTest(runMode, item);
}

void TestResultsPane::toggleOutputStyle()
{
    const bool displayText = m_outputWidget->currentIndex() == 0;
    m_outputWidget->setCurrentIndex(displayText ? 1 : 0);
    m_outputToggleButton->setIcon(displayText ? Icons::VISUAL_DISPLAY.icon()
                                              : Icons::TEXT_DISPLAY.icon());
}

// helper for onCopyWholeTriggered() and onSaveWholeTriggered()
QString TestResultsPane::getWholeOutput(const QModelIndex &parent)
{
    QString output;
    for (int row = 0, count = m_model->rowCount(parent); row < count; ++row) {
        QModelIndex current = m_model->index(row, 0, parent);
        const TestResult *result = m_model->testResult(current);
        QTC_ASSERT(result, continue);
        if (auto item = m_model->itemForIndex(current))
            output.append(item->resultString()).append('\t');
        output.append(result->outputString(true)).append('\n');
        output.append(getWholeOutput(current));
    }
    return output;
}

void TestResultsPane::createMarks(const QModelIndex &parent)
{
    const TestResult *parentResult = m_model->testResult(parent);
    ResultType parentType = parentResult ? parentResult->result() : ResultType::Invalid;
    const QVector<ResultType> interested{ResultType::Fail, ResultType::UnexpectedPass};
    for (int row = 0, count = m_model->rowCount(parent); row < count; ++row) {
        const QModelIndex index = m_model->index(row, 0, parent);
        const TestResult *result = m_model->testResult(index);
        QTC_ASSERT(result, continue);

        if (m_model->hasChildren(index))
            createMarks(index);

        bool isLocationItem = result->result() == ResultType::MessageLocation;
        if (interested.contains(result->result())
                || (isLocationItem && interested.contains(parentType))) {
            const Utils::FilePath fileName = Utils::FilePath::fromString(result->fileName());
            TestEditorMark *mark = new TestEditorMark(index, fileName, result->line());
            mark->setIcon(index.data(Qt::DecorationRole).value<QIcon>());
            mark->setColor(Utils::Theme::OutputPanes_TestFailTextColor);
            mark->setPriority(TextEditor::TextMark::NormalPriority);
            mark->setToolTip(result->description());
            m_marks << mark;
        }
    }
}

void TestResultsPane::clearMarks()
{
    qDeleteAll(m_marks);
    m_marks.clear();
}

void TestResultsPane::showTestResult(const QModelIndex &index)
{
    QModelIndex mapped = m_filterModel->mapFromSource(index);
    if (mapped.isValid()) {
        popup(IOutputPane::NoModeSwitch);
        m_treeView->setCurrentIndex(mapped);
    }
}

} // namespace Internal
} // namespace Autotest

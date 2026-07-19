// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "autotestconstants.h"
#include "testresult.h"

#include <coreplugin/ioutputpane.h>

#include <utils/filepath.h>
#include <utils/itemviews.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QMultiHash>
#include <QQueue>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QAction;
class QFrame;
class QKeyEvent;
class QLabel;
class QModelIndex;
class QMenu;
class QPlainTextEdit;
class QStackedWidget;
class QToolButton;
QT_END_NAMESPACE

namespace Core {
class IContext;
class OutputWindow;
}

namespace Autotest {

enum class OutputChannel;
class TestResult;

namespace Internal {

class TestResultModel;
class TestResultFilterModel;
class TestEditorMark;

class ResultsTreeView : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit ResultsTreeView(QWidget *parent = nullptr);

signals:
    void copyShortcutTriggered();

protected:
    void keyPressEvent(QKeyEvent *event) final;
};

class TestResultsPane : public Core::IOutputPane
{
    Q_OBJECT
public:
    ~TestResultsPane() override;
    static TestResultsPane *instance();

    // IOutputPane interface
    QWidget *outputWidget(QWidget *parent) override;
    QList<QWidget *> toolBarWidgets() const override;
    void clearContents() override;
    void setFocus() override;
    bool hasFocus() const override;
    bool canFocus() const override;
    bool canNavigate() const override;
    bool canNext() const override;
    bool canPrevious() const override;
    void goToNext() override;
    void goToPrev() override;
    void updateFilter() override;

    void scheduleTestResult(const TestResult &result);
    void addOutputLine(const QByteArray &outputLine, OutputChannel channel);
    void showTestResult(const QModelIndex &index);

    bool expandIntermediate() const;

    void aboutToShutdown();

private:
    void addTestResult(const TestResult &result);
    void handleNextBuffered();
    explicit TestResultsPane(QObject *parent = nullptr);

    void onItemActivated(const QModelIndex &index);
    void checkAllFilter(bool checked);
    void filterMenuTriggered(QAction *action);
    bool eventFilter(QObject *object, QEvent *event) override;

    void initializeFilterMenu();
    void updateSummaryLabel();
    void createToolButtons();
    void onTestRunStarted();
    void onTestRunFinished();
    void onScrollBarRangeChanged(int, int max);
    void onCustomContextMenuRequested(const QPoint &pos);
    TestResult getTestResult(const QModelIndex &idx);
    void onCopyItemTriggered(const TestResult &result);
    void onCopyWholeTriggered();
    void onSaveWholeTriggered();
    void onRunThisTestTriggered(TestRunMode runMode, const TestResult &result);
    void toggleOutputStyle();
    QString getWholeOutput(const QModelIndex &parent = QModelIndex());

    void createMarks(const QModelIndex &parent = QModelIndex());
    void clearMarks();

    void onSessionLoaded();
    void onAboutToSaveSession();

    void handlePendingResultsSilently();

    QStackedWidget *m_outputWidget = nullptr;
    QFrame *m_summaryWidget = nullptr;
    QLabel *m_summaryLabel = nullptr;
    ResultsTreeView *m_treeView = nullptr;
    TestResultModel *m_model = nullptr;
    TestResultFilterModel *m_filterModel = nullptr;
    Core::IContext *m_context = nullptr;
    QToolButton *m_expandCollapse = nullptr;
    QToolButton *m_runAll = nullptr;
    QToolButton *m_runSelected = nullptr;
    QToolButton *m_runFailed = nullptr;
    QToolButton *m_runFile = nullptr;
    QToolButton *m_stopTestRun = nullptr;
    QToolButton *m_filterButton = nullptr;
    QToolButton *m_outputToggleButton = nullptr;
    QToolButton *m_showDurationButton = nullptr;
    Core::OutputWindow *m_textOutput = nullptr;
    QMenu *m_filterMenu = nullptr;
    bool m_autoScroll = false;
    bool m_atEnd = false;
    bool m_testRunning = false;
    QMultiHash<QPair<Utils::FilePath, int>, TestEditorMark *> m_marks;
    QQueue<TestResult> m_buffered;
    std::optional<TestResult> m_lastCurrentMessage = std::nullopt;
    QTimer m_bufferTimer;
    QtTaskTree::QSingleTaskTreeRunner m_pendingRunner;
};

} // namespace Internal
} // namespace Autotest

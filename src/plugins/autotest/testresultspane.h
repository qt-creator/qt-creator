// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "testresult.h"

#include <coreplugin/ioutputpane.h>

#include <utils/itemviews.h>

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

    void addTestResult(const TestResult &result);
    void addOutputLine(const QByteArray &outputLine, OutputChannel channel);
    void showTestResult(const QModelIndex &index);
private:
    explicit TestResultsPane(QObject *parent = nullptr);

    void onItemActivated(const QModelIndex &index);
    void onRunAllTriggered();
    void onRunSelectedTriggered();
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

    QStackedWidget *m_outputWidget;
    QFrame *m_summaryWidget;
    QLabel *m_summaryLabel;
    ResultsTreeView *m_treeView;
    TestResultModel *m_model;
    TestResultFilterModel *m_filterModel;
    Core::IContext *m_context;
    QToolButton *m_expandCollapse;
    QToolButton *m_runAll;
    QToolButton *m_runSelected;
    QToolButton *m_runFailed;
    QToolButton *m_runFile;
    QToolButton *m_stopTestRun;
    QToolButton *m_filterButton;
    QToolButton *m_outputToggleButton;
    Core::OutputWindow *m_textOutput;
    QMenu *m_filterMenu;
    bool m_autoScroll = false;
    bool m_atEnd = false;
    bool m_testRunning = false;
    QVector<TestEditorMark *> m_marks;
};

} // namespace Internal
} // namespace Autotest

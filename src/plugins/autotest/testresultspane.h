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

#ifndef TESTRESULTSPANE_H
#define TESTRESULTSPANE_H

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
class QToolButton;
QT_END_NAMESPACE

namespace Core {
class IContext;
}

namespace Autotest {
namespace Internal {

class TestResultModel;
class TestResultFilterModel;

class ResultsTreeView : public Utils::TreeView
{
    Q_OBJECT
public:
    ResultsTreeView(QWidget *parent = 0);

signals:
    void copyShortcutTriggered();

protected:
    void keyPressEvent(QKeyEvent *event);
};

class TestResultsPane : public Core::IOutputPane
{
    Q_OBJECT
public:
    virtual ~TestResultsPane();
    static TestResultsPane *instance();

    // IOutputPane interface
    QWidget *outputWidget(QWidget *parent);
    QList<QWidget *> toolBarWidgets() const;
    QString displayName() const;
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);
    void setFocus();
    bool hasFocus() const;
    bool canFocus() const;
    bool canNavigate() const;
    bool canNext() const;
    bool canPrevious() const;
    void goToNext();
    void goToPrev();

signals:

public slots:
    void addTestResult(const TestResultPtr &result);

private slots:
    void onItemActivated(const QModelIndex &index);
    void onRunAllTriggered();
    void onRunSelectedTriggered();
    void enableAllFilter();
    void filterMenuTriggered(QAction *action);

private:
    explicit TestResultsPane(QObject *parent = 0);
    void initializeFilterMenu();
    void updateSummaryLabel();
    void createToolButtons();
    void onTestRunStarted();
    void onTestRunFinished();
    void onScrollBarRangeChanged(int, int max);
    void updateRunActions();
    void onCustomContextMenuRequested(const QPoint &pos);
    void onCopyItemTriggered(const QModelIndex &idx);
    void onCopyWholeTriggered();
    void onSaveWholeTriggered();
    QString getWholeOutput(const QModelIndex &parent = QModelIndex());

    QWidget *m_outputWidget;
    QFrame *m_summaryWidget;
    QLabel *m_summaryLabel;
    ResultsTreeView *m_treeView;
    TestResultModel *m_model;
    TestResultFilterModel *m_filterModel;
    Core::IContext *m_context;
    QToolButton *m_expandCollapse;
    QToolButton *m_runAll;
    QToolButton *m_runSelected;
    QToolButton *m_stopTestRun;
    QToolButton *m_filterButton;
    QMenu *m_filterMenu;
    bool m_wasVisibleBefore;
    bool m_autoScroll;
    bool m_atEnd;
    bool m_testRunning;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTRESULTSPANE_H

// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QAction;
class QFrame;
class QLabel;
class QMenu;
class QModelIndex;
class QPlainTextEdit;
class QTabWidget;
class QToolButton;
QT_END_NAMESPACE

namespace Utils { class TreeView; }

namespace Squish::Internal {

class TestResult;
class SquishResultItem;
class SquishResultModel;
class SquishResultFilterModel;

class SquishOutputPane final : public Core::IOutputPane
{
public:
    SquishOutputPane();

    static SquishOutputPane *instance();

    // IOutputPane interface
    QWidget *outputWidget(QWidget *parent) override;
    QList<QWidget *> toolBarWidgets() const override;
    void clearContents() override;
    void visibilityChanged(bool visible) override;
    void setFocus() override;
    bool hasFocus() const override;
    bool canFocus() const override;
    bool canNavigate() const override;
    bool canNext() const override;
    bool canPrevious() const override;
    void goToNext() override;
    void goToPrev() override;

    void addResultItem(SquishResultItem *item);
    void addLogOutput(const QString &output);
    void onTestRunFinished();
    void clearOldResults();

private:
    void createToolButtons();
    void initializeFilterMenu();
    void onItemActivated(const QModelIndex &idx);
    void onSectionResized(int logicalIndex, int oldSize, int newSize);
    void onFilterMenuTriggered(QAction *action);
    void enableAllFiltersTriggered();
    void updateSummaryLabel();

    QTabWidget *m_outputPane;
    QWidget *m_outputWidget;
    QFrame *m_summaryWidget;
    QLabel *m_summaryLabel;
    Utils::TreeView *m_treeView;
    SquishResultModel *m_model;
    SquishResultFilterModel *m_filterModel;
    QPlainTextEdit *m_runnerServerLog;
    QToolButton *m_expandAll;
    QToolButton *m_collapseAll;
    QToolButton *m_filterButton;
    QMenu *m_filterMenu;
};

} // namespace Squish::Internal

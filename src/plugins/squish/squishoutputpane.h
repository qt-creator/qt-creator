/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

namespace Core { class IContext; }

namespace Utils { class TreeView; }

namespace Squish {
namespace Internal {

class TestResult;
class SquishResultItem;
class SquishResultModel;
class SquishResultFilterModel;

class SquishOutputPane : public Core::IOutputPane
{
    Q_OBJECT
public:
    static SquishOutputPane *instance();

    // IOutputPane interface
    QWidget *outputWidget(QWidget *parent) override;
    QList<QWidget *> toolBarWidgets() const override;
    QString displayName() const override;
    int priorityInStatusBar() const override;
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

public slots:
    void addResultItem(SquishResultItem *item);
    void addLogOutput(const QString &output);
    void onTestRunFinished();
    void clearOldResults();

private:
    SquishOutputPane(QObject *parent = nullptr);
    void createToolButtons();
    void initializeFilterMenu();
    void onItemActivated(const QModelIndex &idx);
    void onSectionResized(int logicalIndex, int oldSize, int newSize);
    void onFilterMenuTriggered(QAction *action);
    void enableAllFiltersTriggered();
    void updateSummaryLabel();

    QTabWidget *m_outputPane;
    Core::IContext *m_context;
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

} // namespace Internal
} // namespace Squish

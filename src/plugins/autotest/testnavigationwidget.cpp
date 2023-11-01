// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testnavigationwidget.h"

#include "autotestconstants.h"
#include "autotesticons.h"
#include "autotesttr.h"
#include "itemdatacache.h"
#include "testcodeparser.h"
#include "testframeworkmanager.h"
#include "testrunner.h"
#include "testtreeitem.h"
#include "testtreeitemdelegate.h"
#include "testtreemodel.h"
#include "testtreeview.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/itemviewfind.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/link.h>
#include <utils/navigationtreeview.h>
#include <utils/progressindicator.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QMenu>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

using namespace Core;
using namespace Utils;

namespace Autotest::Internal {

class TestNavigationWidget : public QWidget
{
public:
    TestNavigationWidget();

    void contextMenuEvent(QContextMenuEvent *event) override;
    QList<QToolButton *> createToolButtons();

    void updateExpandedStateCache();

private:
    void onItemActivated(const QModelIndex &index);
    void onSortClicked();
    void onFilterMenuTriggered(QAction *action);
    void onParsingStarted();
    void onParsingFinished();
    void initializeFilterMenu();
    void onRunThisTestTriggered(TestRunMode runMode);
    void reapplyCachedExpandedState();

    TestTreeModel *m_model;
    TestTreeSortFilterModel *m_sortFilterModel;
    TestTreeView *m_view;
    QToolButton *m_sort;
    QToolButton *m_filterButton;
    QMenu *m_filterMenu;
    bool m_sortAlphabetically;
    Utils::ProgressIndicator *m_progressIndicator;
    QTimer *m_progressTimer;
    QFrame *m_missingFrameworksWidget;
    ItemDataCache<bool> m_expandedStateCache;
};

TestNavigationWidget::TestNavigationWidget()
{
    setWindowTitle(Tr::tr("Tests"));
    m_model = TestTreeModel::instance();
    m_sortFilterModel = new TestTreeSortFilterModel(m_model, m_model);
    m_sortFilterModel->setDynamicSortFilter(true);
    m_view = new TestTreeView(this);
    m_view->setModel(m_sortFilterModel);
    m_view->setSortingEnabled(true);
    m_view->setItemDelegate(new TestTreeItemDelegate(this));

    QPalette pal;
    pal.setColor(QPalette::Window, creatorTheme()->color(Theme::InfoBarBackground));
    pal.setColor(QPalette::WindowText, creatorTheme()->color(Theme::InfoBarText));
    m_missingFrameworksWidget = new QFrame;
    m_missingFrameworksWidget->setPalette(pal);
    m_missingFrameworksWidget->setAutoFillBackground(true);
    QHBoxLayout *hLayout = new QHBoxLayout;
    m_missingFrameworksWidget->setLayout(hLayout);
    hLayout->addWidget(new QLabel(Tr::tr("No active test frameworks.")));
    const bool hasActiveFrameworks = Utils::anyOf(TestFrameworkManager::registeredFrameworks(),
                                                  &ITestFramework::active);
    m_missingFrameworksWidget->setVisible(!hasActiveFrameworks);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_missingFrameworksWidget);
    layout->addWidget(ItemViewFind::createSearchableWrapper(m_view));
    setLayout(layout);

    connect(m_view, &TestTreeView::activated, this, &TestNavigationWidget::onItemActivated);

    m_progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Medium, this);
    m_progressIndicator->attachToWidget(m_view);
    m_progressIndicator->hide();

    m_progressTimer = new QTimer(this);
    m_progressTimer->setSingleShot(true);
    m_progressTimer->setInterval(1000); // don't display indicator if progress takes less than 1s

    connect(m_model->parser(), &TestCodeParser::parsingStarted,
            this, &TestNavigationWidget::onParsingStarted);
    connect(m_model->parser(), &TestCodeParser::parsingFinished,
            this, &TestNavigationWidget::onParsingFinished);
    connect(m_model->parser(), &TestCodeParser::parsingFailed,
            this, &TestNavigationWidget::onParsingFinished);
    connect(m_model, &TestTreeModel::updatedActiveFrameworks, this, [this](int numberOfActive) {
        m_missingFrameworksWidget->setVisible(numberOfActive == 0);
    });
    ProjectExplorer::ProjectManager *sm = ProjectExplorer::ProjectManager::instance();
    connect(sm, &ProjectExplorer::ProjectManager::startupProjectChanged,
            this, [this](ProjectExplorer::Project * /*project*/) {
        m_expandedStateCache.clear();
    });
    connect(m_model, &TestTreeModel::testTreeModelChanged,
            this, &TestNavigationWidget::reapplyCachedExpandedState);
    connect(m_progressTimer, &QTimer::timeout, m_progressIndicator, &ProgressIndicator::show);
    connect(m_view, &TestTreeView::expanded, this, &TestNavigationWidget::updateExpandedStateCache);
    connect(m_view, &TestTreeView::collapsed, this, &TestNavigationWidget::updateExpandedStateCache);
}

void TestNavigationWidget::contextMenuEvent(QContextMenuEvent *event)
{
    const bool enabled = !ProjectExplorer::BuildManager::isBuilding()
            && !TestRunner::instance()->isTestRunning()
            && m_model->parser()->state() == TestCodeParser::Idle;

    QMenu menu;
    QAction *runThisTest = nullptr;
    QAction *runWithoutDeploy = nullptr;
    QAction *debugThisTest = nullptr;
    QAction *debugWithoutDeploy = nullptr;
    const QModelIndexList list = m_view->selectionModel()->selectedIndexes();
    if (list.size() == 1) {
        const QModelIndex index = list.first();
        QRect rect(m_view->visualRect(index));
        if (rect.contains(event->pos())) {
            ITestTreeItem *item = static_cast<ITestTreeItem *>(
                        m_model->itemForIndex(m_sortFilterModel->mapToSource(index)));
            if (item->canProvideTestConfiguration()) {
                runThisTest = new QAction(Tr::tr("Run This Test"), &menu);
                runThisTest->setEnabled(enabled);
                connect(runThisTest, &QAction::triggered, this, [this] {
                    onRunThisTestTriggered(TestRunMode::Run);
                });
                runWithoutDeploy = new QAction(Tr::tr("Run Without Deployment"), &menu);
                runWithoutDeploy->setEnabled(enabled);
                connect(runWithoutDeploy, &QAction::triggered, this, [this] {
                    onRunThisTestTriggered(TestRunMode::RunWithoutDeploy);
                });
            }
            auto ttitem = item->testBase()->type() == ITestBase::Framework
                              ? static_cast<TestTreeItem *>(item)
                              : nullptr;
            if (ttitem && ttitem->canProvideDebugConfiguration()) {
                debugThisTest = new QAction(Tr::tr("Debug This Test"), &menu);
                debugThisTest->setEnabled(enabled);
                connect(debugThisTest, &QAction::triggered, this, [this] {
                    onRunThisTestTriggered(TestRunMode::Debug);
                });
                debugWithoutDeploy = new QAction(Tr::tr("Debug Without Deployment"), &menu);
                debugWithoutDeploy->setEnabled(enabled);
                connect(debugWithoutDeploy, &QAction::triggered, this, [this] {
                    onRunThisTestTriggered(TestRunMode::DebugWithoutDeploy);
                });
            }
        }
    }

    QAction *runAll = ActionManager::command(Constants::ACTION_RUN_ALL_ID)->action();
    QAction *runSelected = ActionManager::command(Constants::ACTION_RUN_SELECTED_ID)->action();
    QAction *runAllNoDeploy = ActionManager::command(Constants::ACTION_RUN_ALL_NODEPLOY_ID)->action();
    QAction *runSelectedNoDeploy = ActionManager::command(Constants::ACTION_RUN_SELECTED_NODEPLOY_ID)->action();
    QAction *selectAll = new QAction(Tr::tr("Select All"), &menu);
    QAction *deselectAll = new QAction(Tr::tr("Deselect All"), &menu);
    QAction *rescan = ActionManager::command(Constants::ACTION_SCAN_ID)->action();
    QAction *disable = ActionManager::command(Constants::ACTION_DISABLE_TMP)->action();

    connect(selectAll, &QAction::triggered, m_view, &TestTreeView::selectAll);
    connect(deselectAll, &QAction::triggered, m_view, &TestTreeView::deselectAll);

    if (runThisTest) {
        menu.addAction(runThisTest);
        menu.addAction(runWithoutDeploy);
    }
    if (debugThisTest) {
        menu.addAction(debugThisTest);
        menu.addAction(debugWithoutDeploy);
    }
    if (runThisTest || debugThisTest)
        menu.addSeparator();

    menu.addAction(runAll);
    menu.addAction(runAllNoDeploy);
    menu.addAction(runSelected);
    menu.addAction(runSelectedNoDeploy);
    menu.addSeparator();
    menu.addAction(selectAll);
    menu.addAction(deselectAll);
    menu.addSeparator();
    menu.addAction(rescan);
    menu.addSeparator();
    menu.addAction(disable);

    menu.exec(mapToGlobal(event->pos()));
}

QList<QToolButton *> TestNavigationWidget::createToolButtons()
{
    QList<QToolButton *> list;

    m_filterButton = new QToolButton(m_view);
    m_filterButton->setIcon(Utils::Icons::FILTER.icon());
    m_filterButton->setToolTip(Tr::tr("Filter Test Tree"));
    m_filterButton->setProperty(StyleHelper::C_NO_ARROW, true);
    m_filterButton->setPopupMode(QToolButton::InstantPopup);
    m_filterMenu = new QMenu(m_filterButton);
    initializeFilterMenu();
    connect(m_filterMenu, &QMenu::triggered, this, &TestNavigationWidget::onFilterMenuTriggered);
    m_filterButton->setMenu(m_filterMenu);

    m_sortAlphabetically = true;
    m_sort = new QToolButton(this);
    m_sort->setIcon(Icons::SORT_NATURALLY.icon());
    m_sort->setToolTip(Tr::tr("Sort Naturally"));

    QToolButton *expand = new QToolButton(this);
    expand->setIcon(Utils::Icons::EXPAND_TOOLBAR.icon());
    expand->setToolTip(Tr::tr("Expand All"));

    QToolButton *collapse = new QToolButton(this);
    collapse->setIcon(Utils::Icons::COLLAPSE_TOOLBAR.icon());
    collapse->setToolTip(Tr::tr("Collapse All"));

    connect(expand, &QToolButton::clicked, m_view, [this] {
        m_view->blockSignals(true);
        m_view->expandAll();
        m_view->blockSignals(false);
        updateExpandedStateCache();
    });
    connect(collapse, &QToolButton::clicked, m_view, [this] {
        m_view->blockSignals(true);
        m_view->collapseAll();
        m_view->blockSignals(false);
        updateExpandedStateCache();
    });
    connect(m_sort, &QToolButton::clicked, this, &TestNavigationWidget::onSortClicked);

    list << m_filterButton << m_sort << expand << collapse;
    return list;
}

void TestNavigationWidget::updateExpandedStateCache()
{
    m_expandedStateCache.evolve(ITestBase::Framework);

    for (TreeItem *rootNode : *m_model->rootItem()) {
        rootNode->forAllChildren([this](TreeItem *child) {
            m_expandedStateCache.insert(static_cast<ITestTreeItem *>(child),
                                        m_view->isExpanded(child->index()));
        });
    }
}

void TestNavigationWidget::onItemActivated(const QModelIndex &index)
{
    const Link link = index.data(LinkRole).value<Link>();
    if (link.hasValidTarget())
        EditorManager::openEditorAt(link);
}

void TestNavigationWidget::onSortClicked()
{
    if (m_sortAlphabetically) {
        m_sort->setIcon(Utils::Icons::SORT_ALPHABETICALLY_TOOLBAR.icon());
        m_sort->setToolTip(Tr::tr("Sort Alphabetically"));
        m_sortFilterModel->setSortMode(TestTreeItem::Naturally);
    } else {
        m_sort->setIcon(Icons::SORT_NATURALLY.icon());
        m_sort->setToolTip(Tr::tr("Sort Naturally"));
        m_sortFilterModel->setSortMode(TestTreeItem::Alphabetically);
    }
    m_sortAlphabetically = !m_sortAlphabetically;
}

void TestNavigationWidget::onFilterMenuTriggered(QAction *action)
{
    m_sortFilterModel->toggleFilter(
        TestTreeSortFilterModel::toFilterMode(action->data().value<int>()));
}

void TestNavigationWidget::onParsingStarted()
{
    m_progressTimer->start();
}

void TestNavigationWidget::onParsingFinished()
{
    m_progressTimer->stop();
    m_progressIndicator->hide();
}

void TestNavigationWidget::initializeFilterMenu()
{
    QAction *action = new QAction(m_filterMenu);
    action->setText(Tr::tr("Show Init and Cleanup Functions"));
    action->setCheckable(true);
    action->setChecked(false);
    action->setData(TestTreeSortFilterModel::ShowInitAndCleanup);
    m_filterMenu->addAction(action);
    action = new QAction(m_filterMenu);
    action->setText(Tr::tr("Show Data Functions"));
    action->setCheckable(true);
    action->setChecked(false);
    action->setData(TestTreeSortFilterModel::ShowTestData);
    m_filterMenu->addAction(action);
}

void TestNavigationWidget::onRunThisTestTriggered(TestRunMode runMode)
{
    const QModelIndexList selected = m_view->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
        return;
    const QModelIndex &sourceIndex = m_sortFilterModel->mapToSource(selected.first());
    if (!sourceIndex.isValid())
        return;

    ITestTreeItem *item = static_cast<ITestTreeItem *>(sourceIndex.internalPointer());
    TestRunner::instance()->runTest(runMode, item);
}

void TestNavigationWidget::reapplyCachedExpandedState()
{
    using namespace Utils;
    for (TreeItem *rootNode : *m_model->rootItem()) {
        rootNode->forAllChildren([this](TreeItem *child) {
            std::optional<bool> cached = m_expandedStateCache.get(
                static_cast<ITestTreeItem *>(child));
            if (cached.has_value()) {
                QModelIndex index = child->index();
                if (m_view->isExpanded(index) != cached.value())
                    m_view->setExpanded(index, cached.value());
            }
        });
    }
}

// TestNavigationWidgetFactory

TestNavigationWidgetFactory::TestNavigationWidgetFactory()
{
    setDisplayName(Tr::tr("Tests"));
    setId(Autotest::Constants::AUTOTEST_ID);
    setPriority(666);
}

NavigationView TestNavigationWidgetFactory::createWidget()
{
    TestNavigationWidget *treeViewWidget = new TestNavigationWidget;
    return {treeViewWidget, treeViewWidget->createToolButtons()};
}

} // Autotest::Internal

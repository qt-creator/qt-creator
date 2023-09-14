// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "squishoutputpane.h"

#include "squishresultmodel.h"
#include "squishtr.h"
#include "testresult.h"

#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <coreplugin/editormanager/editormanager.h>

#include <utils/itemviews.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

namespace Squish::Internal {

static SquishOutputPane *m_instance = nullptr;

SquishOutputPane::SquishOutputPane()
{
    setId("Squish");
    setDisplayName(Tr::tr("Squish"));
    setPriorityInStatusBar(-60);

    m_instance = this;

    m_outputPane = new QTabWidget;
    m_outputPane->setDocumentMode(true);

    m_outputWidget = new QWidget;
    QVBoxLayout *outputLayout = new QVBoxLayout;
    outputLayout->setContentsMargins(0, 0, 0, 0);
    outputLayout->setSpacing(0);
    m_outputWidget->setLayout(outputLayout);

    QPalette pal;
    pal.setColor(QPalette::Window, Utils::creatorTheme()->color(Utils::Theme::InfoBarBackground));
    pal.setColor(QPalette::WindowText, Utils::creatorTheme()->color(Utils::Theme::InfoBarText));

    m_summaryWidget = new QFrame;
    m_summaryWidget->setPalette(pal);
    m_summaryWidget->setAutoFillBackground(true);
    QHBoxLayout *summaryLayout = new QHBoxLayout;
    summaryLayout->setContentsMargins(6, 6, 6, 6);
    m_summaryWidget->setLayout(summaryLayout);
    m_summaryLabel = new QLabel;
    m_summaryLabel->setPalette(pal);
    summaryLayout->addWidget(m_summaryLabel);
    m_summaryWidget->setVisible(false);

    outputLayout->addWidget(m_summaryWidget);

    m_treeView = new Utils::TreeView(m_outputWidget);
    m_treeView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_treeView->setAlternatingRowColors(true);

    m_model = new SquishResultModel(this);
    m_filterModel = new SquishResultFilterModel(m_model, this);
    m_filterModel->setDynamicSortFilter(true);
    m_treeView->setModel(m_filterModel);

    QHeaderView *header = m_treeView->header();
    header->setSectionsMovable(false);
    header->setStretchLastSection(false);
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    m_treeView->setHeaderHidden(true);

    outputLayout->addWidget(m_treeView);

    createToolButtons();

    m_runnerServerLog = new QPlainTextEdit;
    m_runnerServerLog->setMaximumBlockCount(10000);
    m_runnerServerLog->setReadOnly(true);

    m_outputPane->addTab(m_outputWidget, Tr::tr("Test Results"));
    m_outputPane->addTab(m_runnerServerLog, Tr::tr("Runner/Server Log"));

    connect(m_outputPane, &QTabWidget::currentChanged, this, [this] { navigateStateChanged(); });
    connect(m_treeView, &Utils::TreeView::activated, this, &SquishOutputPane::onItemActivated);
    connect(header, &QHeaderView::sectionResized, this, &SquishOutputPane::onSectionResized);
    connect(m_model, &SquishResultModel::requestExpansion, this, [this](QModelIndex idx) {
        m_treeView->expand(m_filterModel->mapFromSource(idx));
    });
    connect(m_model,
            &SquishResultModel::resultTypeCountUpdated,
            this,
            &SquishOutputPane::updateSummaryLabel);
}

SquishOutputPane *SquishOutputPane::instance()
{
    return m_instance;
}

QWidget *SquishOutputPane::outputWidget(QWidget *parent)
{
    if (m_outputPane)
        m_outputPane->setParent(parent);
    else
        qWarning("This should not happen");
    return m_outputPane;
}

QList<QWidget *> SquishOutputPane::toolBarWidgets() const
{
    return {m_filterButton, m_expandAll, m_collapseAll};
}

void SquishOutputPane::clearContents()
{
    if (m_outputPane->currentIndex() == 0)
        clearOldResults();
    else if (m_outputPane->currentIndex() == 1)
        m_runnerServerLog->clear();
}

void SquishOutputPane::visibilityChanged(bool visible)
{
    Q_UNUSED(visible)
}

void SquishOutputPane::setFocus()
{
    if (m_outputPane->currentIndex() == 0)
        m_treeView->setFocus();
    else if (m_outputPane->currentIndex() == 1)
        m_runnerServerLog->setFocus();
}

bool SquishOutputPane::hasFocus() const
{
    return m_treeView->hasFocus() || m_runnerServerLog->hasFocus();
}

bool SquishOutputPane::canFocus() const
{
    return true;
}

bool SquishOutputPane::canNavigate() const
{
    return m_outputPane->currentIndex() == 0; // only support navigation for test results
}

bool SquishOutputPane::canNext() const
{
    return m_filterModel->hasResults();
}

bool SquishOutputPane::canPrevious() const
{
    return m_filterModel->hasResults();
}

void SquishOutputPane::goToNext()
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

void SquishOutputPane::goToPrev()
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

void SquishOutputPane::addResultItem(SquishResultItem *item)
{
    m_model->addResultItem(item);
    m_treeView->setHeaderHidden(false);
    if (!m_treeView->isVisible())
        popup(Core::IOutputPane::NoModeSwitch);
    flash();
    navigateStateChanged();
}

void SquishOutputPane::addLogOutput(const QString &output)
{
    m_runnerServerLog->appendPlainText(output);
}

void SquishOutputPane::onTestRunFinished()
{
    m_model->expandVisibleRootItems();
    m_summaryWidget->setVisible(true);
    updateSummaryLabel();
}

void SquishOutputPane::updateSummaryLabel()
{
    if (m_summaryWidget->isVisible()) {
        const int passes = m_model->resultTypeCount(Result::Pass)
                           + m_model->resultTypeCount(Result::ExpectedFail);
        const int fails = m_model->resultTypeCount(Result::Fail)
                          + m_model->resultTypeCount(Result::UnexpectedPass);
        const QString labelText =
                QString("<p>" + Tr::tr("<b>Test summary:</b>&nbsp;&nbsp; %1 passes, %2 fails, "
                                     "%3 fatals, %4 errors, %5 warnings.") + "</p>")
                                      .arg(passes)
                                      .arg(fails)
                                      .arg(m_model->resultTypeCount(Result::Fatal))
                                      .arg(m_model->resultTypeCount(Result::Error))
                                      .arg(m_model->resultTypeCount(Result::Warn));

        m_summaryLabel->setText(labelText);
    }
}

void SquishOutputPane::clearOldResults()
{
    m_treeView->setHeaderHidden(true);
    m_summaryWidget->setVisible(false);
    m_filterModel->clearResults();
    navigateStateChanged();
}

void SquishOutputPane::createToolButtons()
{
    m_expandAll = new QToolButton(m_treeView);
    Utils::StyleHelper::setPanelWidget(m_expandAll);
    m_expandAll->setIcon(Utils::Icons::EXPAND_TOOLBAR.icon());
    m_expandAll->setToolTip(Tr::tr("Expand All"));

    m_collapseAll = new QToolButton(m_treeView);
    Utils::StyleHelper::setPanelWidget(m_collapseAll);
    m_collapseAll->setIcon(Utils::Icons::COLLAPSE_TOOLBAR.icon());
    m_collapseAll->setToolTip(Tr::tr("Collapse All"));

    m_filterButton = new QToolButton(m_treeView);
    Utils::StyleHelper::setPanelWidget(m_filterButton);
    m_filterButton->setIcon(Utils::Icons::FILTER.icon());
    m_filterButton->setToolTip(Tr::tr("Filter Test Results"));
    m_filterButton->setProperty(Utils::StyleHelper::C_NO_ARROW, true);
    m_filterButton->setAutoRaise(true);
    m_filterButton->setPopupMode(QToolButton::InstantPopup);
    m_filterMenu = new QMenu(m_filterButton);
    initializeFilterMenu();
    m_filterButton->setMenu(m_filterMenu);

    connect(m_expandAll, &QToolButton::clicked, m_treeView, &Utils::TreeView::expandAll);
    connect(m_collapseAll, &QToolButton::clicked, m_treeView, &Utils::TreeView::collapseAll);
    connect(m_filterMenu, &QMenu::triggered, this, &SquishOutputPane::onFilterMenuTriggered);
}

void SquishOutputPane::initializeFilterMenu()
{
    QMap<Result::Type, QString> textAndType;
    textAndType.insert(Result::Pass, Tr::tr("Pass"));
    textAndType.insert(Result::Fail, Tr::tr("Fail"));
    textAndType.insert(Result::ExpectedFail, Tr::tr("Expected Fail"));
    textAndType.insert(Result::UnexpectedPass, Tr::tr("Unexpected Pass"));
    textAndType.insert(Result::Warn, Tr::tr("Warning Messages"));
    textAndType.insert(Result::Log, Tr::tr("Log Messages"));

    const QList<Result::Type> types = textAndType.keys();
    for (Result::Type type : types) {
        QAction *action = new QAction(m_filterMenu);
        action->setText(textAndType.value(type));
        action->setCheckable(true);
        action->setChecked(true);
        action->setData(type);
        m_filterMenu->addAction(action);
    }
    m_filterMenu->addSeparator();
    QAction *action = new QAction(m_filterMenu);
    action->setText(Tr::tr("Check All Filters"));
    action->setCheckable(false);
    m_filterMenu->addAction(action);
    connect(action, &QAction::triggered, this, &SquishOutputPane::enableAllFiltersTriggered);
}

void SquishOutputPane::onItemActivated(const QModelIndex &idx)
{
    if (!idx.isValid())
        return;

    const TestResult result = m_filterModel->testResult(idx);
    if (!result.file().isEmpty())
        Core::EditorManager::openEditorAt(
            Utils::Link(Utils::FilePath::fromString(result.file()), result.line(), 0));
}

// TODO: this is currently a workaround - might vanish if a item delegate will be implemented
void SquishOutputPane::onSectionResized(int logicalIndex, int /*oldSize*/, int /*newSize*/)
{
    // details column should have been modified by user, so no action, time stamp column is fixed
    if (logicalIndex != 1) {
        QHeaderView *header = m_treeView->header();
        const int minimum = m_outputPane->width() - header->sectionSize(0) - header->sectionSize(2);
        header->resizeSection(1, qMax(minimum, header->sectionSize(1)));
    }
}

void SquishOutputPane::onFilterMenuTriggered(QAction *action)
{
    m_filterModel->toggleResultType(Result::Type(action->data().toInt()));
    navigateStateChanged();
}

void SquishOutputPane::enableAllFiltersTriggered()
{
    const QList<QAction *> actions = m_filterMenu->actions();
    for (QAction *action : actions)
        action->setChecked(true);

    m_filterModel->enableAllResultTypes();
}

} // namespace Squish::Internal

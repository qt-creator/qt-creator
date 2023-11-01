// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "todooutputpane.h"

#include "constants.h"
#include "todoitemsmodel.h"
#include "todooutputtreeview.h"
#include "todotr.h"

#include <aggregation/aggregate.h>
#include <coreplugin/find/itemviewfind.h>

#include <QIcon>
#include <QHeaderView>
#include <QToolButton>
#include <QButtonGroup>
#include <QSortFilterProxyModel>

namespace Todo {
namespace Internal {

TodoOutputPane::TodoOutputPane(TodoItemsModel *todoItemsModel, const Settings *settings, QObject *parent) :
    IOutputPane(parent),
    m_todoItemsModel(todoItemsModel),
    m_settings(settings)
{
    setId("To-DoEntries");
    setDisplayName(Tr::tr("To-Do Entries"));
    setPriorityInStatusBar(10);

    createTreeView();
    createScopeButtons();
    setScanningScope(ScanningScopeCurrentFile); // default
    connect(m_todoTreeView->model(), &TodoItemsModel::layoutChanged,
            this, &TodoOutputPane::navigateStateUpdate);
    connect(m_todoTreeView->model(), &TodoItemsModel::layoutChanged,
            this, &TodoOutputPane::updateTodoCount);
}

TodoOutputPane::~TodoOutputPane()
{
    freeTreeView();
    freeScopeButtons();
}

QWidget *TodoOutputPane::outputWidget(QWidget *parent)
{
    Q_UNUSED(parent)
    return m_todoTreeView;
}

QList<QWidget*> TodoOutputPane::toolBarWidgets() const
{
    QWidgetList widgets;

    for (QToolButton *btn: m_filterButtons)
        widgets << btn;

    widgets << m_spacer << m_currentFileButton << m_wholeProjectButton << m_subProjectButton;

    return widgets;
}

void TodoOutputPane::clearContents()
{
    clearKeywordFilter();
}

void TodoOutputPane::setFocus()
{
    m_todoTreeView->setFocus();
}

bool TodoOutputPane::hasFocus() const
{
    return m_todoTreeView->window()->focusWidget() == m_todoTreeView;
}

bool TodoOutputPane::canFocus() const
{
    return true;
}

bool TodoOutputPane::canNavigate() const
{
    return true;
}

bool TodoOutputPane::canNext() const
{
    return m_todoTreeView->model()->rowCount() > 0;
}

bool TodoOutputPane::canPrevious() const
{
    return m_todoTreeView->model()->rowCount() > 0;
}

void TodoOutputPane::goToNext()
{
    const QModelIndex nextIndex = nextModelIndex();
    m_todoTreeView->selectionModel()->setCurrentIndex(nextIndex, QItemSelectionModel::SelectCurrent
                                                      | QItemSelectionModel::Rows | QItemSelectionModel::Clear);
    todoTreeViewClicked(nextIndex);
}

void TodoOutputPane::goToPrev()
{
    const QModelIndex prevIndex = previousModelIndex();
    m_todoTreeView->selectionModel()->setCurrentIndex(prevIndex, QItemSelectionModel::SelectCurrent
                                                      | QItemSelectionModel::Rows | QItemSelectionModel::Clear);
    todoTreeViewClicked(prevIndex);
}

void TodoOutputPane::setScanningScope(ScanningScope scanningScope)
{
    if (scanningScope == ScanningScopeCurrentFile)
        m_currentFileButton->setChecked(true);
    else if (scanningScope == ScanningScopeSubProject)
        m_subProjectButton->setChecked(true);
    else if (scanningScope == ScanningScopeProject)
        m_wholeProjectButton->setChecked(true);
    else
        Q_ASSERT_X(false, "Updating scanning scope buttons", "Unknown scanning scope enum value");
}

void TodoOutputPane::scopeButtonClicked(QAbstractButton *button)
{
    if (button == m_currentFileButton)
        emit scanningScopeChanged(ScanningScopeCurrentFile);
    else if (button == m_subProjectButton)
        emit scanningScopeChanged(ScanningScopeSubProject);
    else if (button == m_wholeProjectButton)
        emit scanningScopeChanged(ScanningScopeProject);
    emit setBadgeNumber(m_todoTreeView->model()->rowCount());
}

void TodoOutputPane::todoTreeViewClicked(const QModelIndex &index)
{
    // Create a to-do item and notify that it was clicked on

    int row = index.row();

    TodoItem item;
    item.text = index.sibling(row, Constants::OUTPUT_COLUMN_TEXT).data().toString();
    item.file = Utils::FilePath::fromUserInput(index.sibling(row, Constants::OUTPUT_COLUMN_FILE).data().toString());
    item.line = index.sibling(row, Constants::OUTPUT_COLUMN_LINE).data().toInt();
    item.color = index.data(Qt::ForegroundRole).value<QColor>();
    item.iconType = static_cast<IconType>(index.sibling(row, Constants::OUTPUT_COLUMN_TEXT)
                                          .data(Qt::UserRole).toInt());

    emit todoItemClicked(item);
}

void TodoOutputPane::updateTodoCount()
{
    emit setBadgeNumber(m_todoTreeView->model()->rowCount());
}

void TodoOutputPane::updateKeywordFilter()
{
    QStringList keywords;
    for (const QToolButton *btn: std::as_const(m_filterButtons)) {
        if (btn->isChecked())
            keywords.append(btn->property(Constants::FILTER_KEYWORD_NAME).toString());
    }

    QString pattern = keywords.isEmpty() ? QString() : QString("^(%1).*").arg(keywords.join('|'));
    int sortColumn = m_todoTreeView->header()->sortIndicatorSection();
    Qt::SortOrder sortOrder = m_todoTreeView->header()->sortIndicatorOrder();

    m_filteredTodoItemsModel->setFilterRegularExpression(pattern);
    m_filteredTodoItemsModel->sort(sortColumn, sortOrder);

    updateTodoCount();
}

void TodoOutputPane::clearKeywordFilter()
{
    for (QToolButton *btn: std::as_const(m_filterButtons))
        btn->setChecked(false);

    updateKeywordFilter();
}

void TodoOutputPane::createTreeView()
{
    m_filteredTodoItemsModel = new QSortFilterProxyModel();
    m_filteredTodoItemsModel->setSourceModel(m_todoItemsModel);
    m_filteredTodoItemsModel->setDynamicSortFilter(false);
    m_filteredTodoItemsModel->setFilterKeyColumn(Constants::OUTPUT_COLUMN_TEXT);

    m_todoTreeView = new TodoOutputTreeView();
    m_todoTreeView->setModel(m_filteredTodoItemsModel);
    auto agg = new Aggregation::Aggregate;
    agg->add(m_todoTreeView);
    agg->add(new Core::ItemViewFind(m_todoTreeView));

    connect(m_todoTreeView, &TodoOutputTreeView::activated, this, &TodoOutputPane::todoTreeViewClicked);
}

void TodoOutputPane::freeTreeView()
{
    delete m_todoTreeView;
    delete m_filteredTodoItemsModel;
}

QToolButton *TodoOutputPane::createCheckableToolButton(const QString &text, const QString &toolTip, const QIcon &icon)
{
    auto button = new QToolButton;

    button->setCheckable(true);
    button->setText(text);
    button->setToolTip(toolTip);
    button->setIcon(icon);

    return button;
}

void TodoOutputPane::createScopeButtons()
{
    m_currentFileButton = new QToolButton();
    m_currentFileButton->setCheckable(true);
    m_currentFileButton->setText(Tr::tr("Current Document"));
    m_currentFileButton->setToolTip(Tr::tr("Scan only the currently edited document."));

    m_wholeProjectButton = new QToolButton();
    m_wholeProjectButton->setCheckable(true);
    m_wholeProjectButton->setText(Tr::tr("Active Project"));
    m_wholeProjectButton->setToolTip(Tr::tr("Scan the whole active project."));

    m_subProjectButton = new QToolButton();
    m_subProjectButton->setCheckable(true);
    m_subProjectButton->setText(Tr::tr("Subproject"));
    m_subProjectButton->setToolTip(Tr::tr("Scan the current subproject."));

    m_scopeButtons = new QButtonGroup();
    m_scopeButtons->addButton(m_wholeProjectButton);
    m_scopeButtons->addButton(m_currentFileButton);
    m_scopeButtons->addButton(m_subProjectButton);
    connect(m_scopeButtons, &QButtonGroup::buttonClicked,
            this, &TodoOutputPane::scopeButtonClicked);

    m_spacer = new QWidget;
    m_spacer->setMinimumWidth(Constants::OUTPUT_TOOLBAR_SPACER_WIDTH);

    QString tooltip = Tr::tr("Show \"%1\" entries");
    for (const Keyword &keyword: m_settings->keywords) {
        QToolButton *button = createCheckableToolButton(keyword.name, tooltip.arg(keyword.name), toolBarIcon(keyword.iconType));
        button->setProperty(Constants::FILTER_KEYWORD_NAME, keyword.name);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        connect(button, &QToolButton::clicked, this, &TodoOutputPane::updateKeywordFilter);

        m_filterButtons.append(button);
    }
}

void TodoOutputPane::freeScopeButtons()
{
    delete m_currentFileButton;
    delete m_wholeProjectButton;
    delete m_subProjectButton;
    delete m_scopeButtons;
    delete m_spacer;

    qDeleteAll(m_filterButtons);
}

QModelIndex TodoOutputPane::selectedModelIndex()
{
    QModelIndexList selectedIndexes = m_todoTreeView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty())
        return QModelIndex();
    else
        // There is only one item selected
        return selectedIndexes.first();
}

QModelIndex TodoOutputPane::nextModelIndex()
{
    QModelIndex indexToBeSelected = m_todoTreeView->indexBelow(selectedModelIndex());
    if (!indexToBeSelected.isValid())
        return m_todoTreeView->model()->index(0, 0);
    else
        return indexToBeSelected;
}

QModelIndex TodoOutputPane::previousModelIndex()
{
    QModelIndex indexToBeSelected = m_todoTreeView->indexAbove(selectedModelIndex());
    if (!indexToBeSelected.isValid())
        return m_todoTreeView->model()->index(m_todoTreeView->model()->rowCount() - 1, 0);
    else
        return indexToBeSelected;
}

} // namespace Internal
} // namespace Todo

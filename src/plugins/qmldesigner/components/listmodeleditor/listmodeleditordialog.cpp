// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "listmodeleditordialog.h"
#include "listmodeleditormodel.h"

#include <theme.h>
#include <qmldesignericons.h>

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/stylehelper.h>

#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMainWindow>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>

namespace QmlDesigner {

namespace {
QIcon getIcon(Theme::Icon icon)
{
    const QString fontName = "qtds_propertyIconFont.ttf";

    return Utils::StyleHelper::getIconFromIconFont(fontName, Theme::getIconUnicode(icon), 30, 30);
}
} // namespace

ListModelEditorDialog::ListModelEditorDialog(QWidget *parent)
    : QDialog(parent)
{
    resize((Core::ICore::mainWindow()->size() * 8) / 10);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QToolBar *toolBar = new QToolBar();
    toolBar->setIconSize({30, 30});
    mainLayout->addWidget(toolBar);
    m_tableView = new QTableView;
    mainLayout->addWidget(m_tableView);

    m_addRowAboveAction = toolBar->addAction(getIcon(Theme::Icon::addRowBefore), tr("Add Row Above"));
    m_addRowBelowAction = toolBar->addAction(getIcon(Theme::Icon::addRowAfter), tr("Add Row Below"));
    m_removeRowsAction = toolBar->addAction(getIcon(Theme::Icon::deleteRow), tr("Remove Rows"));
    m_addColumnAction = toolBar->addAction(getIcon(Theme::Icon::addColumnAfter), tr("Add Column"));
    m_removeColumnsAction = toolBar->addAction(getIcon(Theme::Icon::deleteColumn),
                                               tr("Remove Columns"));
    m_moveDownAction = toolBar->addAction(Icons::ARROW_DOWN.icon(), tr("Move Down (Ctrl + Down)"));
    m_moveDownAction->setShortcut(QKeySequence(Qt::Key_Down | Qt::CTRL));
    m_moveUpAction = toolBar->addAction(Icons::ARROW_UP.icon(), tr("Move Up (Ctrl + Up)"));
    m_moveUpAction->setShortcut(QKeySequence(Qt::Key_Up | Qt::CTRL));
}

ListModelEditorDialog::~ListModelEditorDialog() = default;

void ListModelEditorDialog::setModel(ListModelEditorModel *model)
{
    m_model = model;

    connect(m_addRowAboveAction, &QAction::triggered, this, &ListModelEditorDialog::addRowAbove);
    connect(m_addRowBelowAction, &QAction::triggered, this, &ListModelEditorDialog::addRowBelow);
    connect(m_addColumnAction, &QAction::triggered, this, &ListModelEditorDialog::openColumnDialog);
    connect(m_removeRowsAction, &QAction::triggered, this, &ListModelEditorDialog::removeRows);
    connect(m_removeColumnsAction, &QAction::triggered, this, &ListModelEditorDialog::removeColumns);
    connect(m_moveDownAction, &QAction::triggered, this, &ListModelEditorDialog::moveRowsDown);
    connect(m_moveUpAction, &QAction::triggered, this, &ListModelEditorDialog::moveRowsUp);
    connect(m_tableView->horizontalHeader(),
            &QHeaderView::sectionDoubleClicked,
            this,
            &ListModelEditorDialog::changeHeader);

    m_tableView->setModel(model);
    m_tableView->horizontalHeader()->setMinimumSectionSize(60);
    m_tableView->verticalHeader()->setMinimumSectionSize(25);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    connect(model,
            &ListModelEditorModel::rowsInserted,
            this,
            &ListModelEditorDialog::onRowsInserted,
            Qt::ConnectionType::UniqueConnection);

    connect(model,
            &ListModelEditorModel::columnsInserted,
            this,
            &ListModelEditorDialog::onColumnsInserted,
            Qt::ConnectionType::UniqueConnection);

    connect(m_tableView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &ListModelEditorDialog::updateSelection,
            Qt::ConnectionType::UniqueConnection);
    updateSelection();
}

void ListModelEditorDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {
        const QModelIndexList selectedIndexes = m_tableView->selectionModel()->selectedIndexes();
        for (const QModelIndex index : selectedIndexes)
            m_model->setData(index, QVariant(), Qt::EditRole);
    }
}

void ListModelEditorDialog::addRowAbove()
{
    // If there's no items in the model, or nothing is selected, The item should be prepended.
    int curInteractionRow = ListModelEditorModel::currentInteractionRow(*m_tableView->selectionModel());
    m_model->addRow(std::max(0, curInteractionRow));
}

void ListModelEditorDialog::addRowBelow()
{
    m_model->addRow(ListModelEditorModel::nextInteractionRow(*m_tableView->selectionModel()));
}

void ListModelEditorDialog::openColumnDialog()
{
    bool ok;
    QString columnName = QInputDialog::getText(
        this, tr("Add Property"), tr("Property name:"), QLineEdit::Normal, "", &ok);
    if (ok && !columnName.isEmpty())
        m_model->addColumn(columnName);
}

void ListModelEditorDialog::removeRows()
{
    m_model->removeRows(m_tableView->selectionModel()->selectedRows());
}

void ListModelEditorDialog::removeColumns()
{
    m_model->removeColumns(m_tableView->selectionModel()->selectedColumns());
}

void ListModelEditorDialog::changeHeader(int column)
{
    if (column < 0)
        return;

    const QString propertyName = QString::fromUtf8(m_model->propertyNames()[column]);

    bool ok;
    QString newPropertyName = QInputDialog::getText(
        this, tr("Change Property"), tr("Column name:"), QLineEdit::Normal, propertyName, &ok);

    if (ok && !newPropertyName.isEmpty())
        m_model->renameColumn(column, newPropertyName);
}

void ListModelEditorDialog::moveRowsDown()
{
    QItemSelection selection = m_model->moveRowsDown(m_tableView->selectionModel()->selectedRows());
    m_tableView->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
    if (!selection.isEmpty()) {
        m_tableView->selectionModel()->setCurrentIndex(selection.first().topLeft(),
                                                       QItemSelectionModel::Current);
    }
}

void ListModelEditorDialog::moveRowsUp()
{
    QItemSelection selection = m_model->moveRowsUp(m_tableView->selectionModel()->selectedRows());
    m_tableView->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
    if (!selection.isEmpty()) {
        m_tableView->selectionModel()->setCurrentIndex(selection.first().topLeft(),
                                                       QItemSelectionModel::Current);
    }
}

void ListModelEditorDialog::updateSelection()
{
    QItemSelectionModel *selection = m_tableView->selectionModel();
    bool hasRowSelection = !selection->selectedRows().isEmpty();
    bool hasColumnSelection = !selection->selectedColumns().isEmpty();
    const int rows = m_tableView->model()->rowCount();

    m_moveUpAction->setEnabled(hasRowSelection && !selection->isRowSelected(0));
    m_moveDownAction->setEnabled(hasRowSelection && !selection->isRowSelected(rows - 1));
    m_removeRowsAction->setEnabled(hasRowSelection);
    m_removeColumnsAction->setEnabled(hasColumnSelection);
}

void ListModelEditorDialog::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    QItemSelectionModel *selection = m_tableView->selectionModel();
    auto model = selection->model();
    const int cols = model->columnCount(parent);
    auto topLeft = model->index(first, 0, parent);
    auto bottomRight = model->index(last, cols - 1, parent);
    QItemSelection rowsSelection{topLeft, bottomRight};

    selection->select(rowsSelection, QItemSelectionModel::ClearAndSelect);
}

void ListModelEditorDialog::onColumnsInserted(const QModelIndex &parent, int first, int last)
{
    QItemSelectionModel *selection = m_tableView->selectionModel();
    auto model = selection->model();
    const int rows = model->rowCount(parent);
    auto topLeft = model->index(0, first, parent);
    auto bottomRight = model->index(rows - 1, last, parent);
    QItemSelection columnSelection{topLeft, bottomRight};

    selection->select(columnSelection, QItemSelectionModel::ClearAndSelect);
}

} // namespace QmlDesigner

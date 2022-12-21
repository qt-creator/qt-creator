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

#include <vector>

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

    m_addRowAction = toolBar->addAction(getIcon(Theme::Icon::addRowAfter), tr("Add Row"));
    m_removeRowsAction = toolBar->addAction(getIcon(Theme::Icon::deleteRow), tr("Remove Columns"));
    m_addColumnAction = toolBar->addAction(getIcon(Theme::Icon::addColumnAfter), tr("Add Column"));
    m_removeColumnsAction = toolBar->addAction(getIcon(Theme::Icon::deleteColumn),
                                               tr("Remove Columns"));
    m_moveDownAction = toolBar->addAction(Icons::ARROW_DOWN.icon(), tr("Move Down (Ctrl + Down)"));
    m_moveDownAction->setShortcut(QKeySequence(Qt::Key_Down | Qt::CTRL));
    m_moveUpAction = toolBar->addAction(Icons::ARROW_UP.icon(), tr("Move Up (Ctrl + Up)"));
    m_moveDownAction->setShortcut(QKeySequence(Qt::Key_Up | Qt::CTRL));
}

ListModelEditorDialog::~ListModelEditorDialog() = default;

void ListModelEditorDialog::setModel(ListModelEditorModel *model)
{
    m_model = model;

    connect(m_addRowAction, &QAction::triggered, m_model, &ListModelEditorModel::addRow);
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
}

void ListModelEditorDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {
        for (const QModelIndex index : m_tableView->selectionModel()->selectedIndexes())
            m_model->setData(index, QVariant(), Qt::EditRole);
    }
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
    m_tableView->selectionModel()->select(selection, QItemSelectionModel::Select);
}

void ListModelEditorDialog::moveRowsUp()
{
    QItemSelection selection = m_model->moveRowsUp(m_tableView->selectionModel()->selectedRows());
    m_tableView->selectionModel()->select(selection, QItemSelectionModel::Select);
}

} // namespace QmlDesigner

/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "listmodeleditordialog.h"
#include "listmodeleditormodel.h"

#include <theme.h>

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/stylehelper.h>

#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
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
}

ListModelEditorDialog::~ListModelEditorDialog() = default;

void ListModelEditorDialog::setModel(ListModelEditorModel *model)
{
    m_model = model;

    connect(m_addRowAction, &QAction::triggered, m_model, &ListModelEditorModel::addRow);
    connect(m_addColumnAction, &QAction::triggered, this, &ListModelEditorDialog::openColumnDialog);
    connect(m_removeRowsAction, &QAction::triggered, this, &ListModelEditorDialog::removeRows);
    connect(m_removeColumnsAction, &QAction::triggered, this, &ListModelEditorDialog::removeColumns);
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
        this, tr("Add Property"), tr("Property Name:"), QLineEdit::Normal, "", &ok);
    if (ok && !columnName.isEmpty())
        m_model->addColumn(columnName);
}

void ListModelEditorDialog::removeRows()
{
    const QList<QModelIndex> indices = m_tableView->selectionModel()->selectedRows();
    std::vector<int> rows;
    rows.reserve(indices.size());

    for (QModelIndex index : indices)
        rows.push_back(index.row());

    std::sort(rows.begin(), rows.end());

    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());

    std::reverse(rows.begin(), rows.end());

    for (int row : rows)
        m_model->removeRow(row);
}

void ListModelEditorDialog::removeColumns()
{
    const QList<QModelIndex> indices = m_tableView->selectionModel()->selectedColumns();
    std::vector<int> columns;
    columns.reserve(indices.size());

    for (QModelIndex index : indices)
        columns.push_back(index.column());

    std::sort(columns.begin(), columns.end());

    columns.erase(std::unique(columns.begin(), columns.end()), columns.end());

    std::reverse(columns.begin(), columns.end());

    for (int row : columns)
        m_model->removeColumn(row);
}

void ListModelEditorDialog::changeHeader(int column)
{
    const QString propertyName = QString::fromUtf8(m_model->propertyNames()[column]);

    bool ok;
    QString newPropertyName = QInputDialog::getText(
        this, tr("Change Propertry"), tr("Column Name:"), QLineEdit::Normal, propertyName, &ok);

    if (ok && !newPropertyName.isEmpty())
        m_model->renameColumn(column, newPropertyName);
}

} // namespace QmlDesigner

// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modellistwidget.h"

#include <componentcore/theme.h>

#include <utils/fileutils.h>

#include <QHeaderView>
#include <QToolBar>
#include <QToolButton>

namespace QmlDesigner {

static QIcon toolButtonIcon(Theme::Icon type)
{
    const QIcon normalIcon = Theme::iconFromName(type, Theme::getColor(Theme::Color::DS_text_default));
    const QIcon disabledIcon = Theme::iconFromName(type, Theme::getColor(Theme::Color::DS_text_muted));
    QIcon icon;
    icon.addPixmap(normalIcon.pixmap(20), QIcon::Normal, QIcon::On);
    icon.addPixmap(disabledIcon.pixmap(20), QIcon::Disabled, QIcon::On);
    return icon;
}

ModelListWidget::ModelListWidget(QWidget *parent)
    : QTableWidget(0, 2, parent)
    , m_addButton(Utils::makeUniqueObjectPtr<QToolButton>())
    , m_removeButton(Utils::makeUniqueObjectPtr<QToolButton>())
    , m_moveUpButton(Utils::makeUniqueObjectPtr<QToolButton>())
    , m_moveDownButton(Utils::makeUniqueObjectPtr<QToolButton>())
    , m_resetButton(Utils::makeUniqueObjectPtr<QToolButton>())
    , m_toolBar(Utils::makeUniqueObjectPtr<QToolBar>())
{
    setHorizontalHeaderLabels({tr("Model ID"), tr("Model Name")});

    horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    verticalHeader()->setVisible(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    setMinimumHeight(110);

    // Stylesheet
    QString sheet = Utils::FileUtils::fetchQrc(":/qmldesigner/stylesheet.css");
    setStyleSheet(Theme::replaceCssColors(sheet));

    // Toolbar
    m_addButton->setIcon(toolButtonIcon(Theme::plus));
    m_removeButton->setIcon(toolButtonIcon(Theme::minus));
    m_moveUpButton->setIcon(toolButtonIcon(Theme::moveUp_medium));
    m_moveDownButton->setIcon(toolButtonIcon(Theme::moveDown_medium));
    m_resetButton->setIcon(toolButtonIcon(Theme::resetView_small));

    m_addButton->setToolTip(tr("Add a new model"));
    m_removeButton->setToolTip(tr("Remove selected model"));
    m_moveUpButton->setToolTip(tr("Move model up"));
    m_moveDownButton->setToolTip(tr("Move model down"));
    m_resetButton->setToolTip(tr("Reset to defaults"));

    m_toolBar->addWidget(m_addButton.get());
    m_toolBar->addWidget(m_removeButton.get());
    m_toolBar->addWidget(m_moveUpButton.get());
    m_toolBar->addWidget(m_moveDownButton.get());
    m_toolBar->addWidget(m_resetButton.get());

    // Connections
    connect(m_addButton.get(), &QToolButton::clicked, this, [this] { addRow(); });
    connect(m_removeButton.get(), &QToolButton::clicked, this, [this] { removeRow(currentRow()); });
    connect(m_resetButton.get(), &QToolButton::clicked, this, [this] { setWidgetItems(m_defaultItems); });
    connect(selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, [this](const QModelIndex &current, const QModelIndex &previous) {
                Q_UNUSED(previous)
                onRowChanged(current.row());
            });

    connect(m_moveUpButton.get(), &QToolButton::clicked, this, [this] {
        int row = currentRow();
        if (row <= 0)
            return;

        insertRow(row - 1);
        for (int i = 0; i < columnCount(); ++i)
            setItem(row - 1, i, takeItem(row + 1, i));
        removeRow(row + 1);
        setCurrentCell(row - 1, 0);
    });

    connect(m_moveDownButton.get(), &QToolButton::clicked, this, [this] {
        int row = currentRow();
        if (row < 0 || row >= rowCount() - 1)
            return;

        insertRow(row + 2);
        for (int i = 0; i < columnCount(); ++i)
            setItem(row + 2, i, takeItem(row, i));
        removeRow(row);
        setCurrentCell(row + 1, 0);
    });
}

ModelListWidget::~ModelListWidget() = default;

QSize ModelListWidget::sizeHint() const
{
    // Total height of all rows + the header height + the frames
    int h = verticalHeader()->length() + horizontalHeader()->height() + (frameWidth() * 2);
    return QSize(QTableWidget::sizeHint().width(), h);
}

void ModelListWidget::addRow(const AiModelData &rowData)
{
    int row = rowCount();
    insertRow(row);

    auto *idItem = new QTableWidgetItem(rowData.id);
    idItem->setFlags(idItem->flags() | Qt::ItemIsEditable);
    setItem(row, 0, idItem);

    auto *nameItem = new QTableWidgetItem(rowData.name);
    nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
    setItem(row, 1, nameItem);

    setCurrentCell(row, 0);
    editItem(item(row, 0));
}

QList<AiModelData> ModelListWidget::items() const
{
    QList<AiModelData> result;

    for (int r = 0; r < rowCount(); ++r)
        result.append({item(r, 0)->text(), item(r, 1)->text()});

    return result;
}

void ModelListWidget::setItems(const QList<AiModelData> &items, const QList<AiModelData> &defaultItems)
{
    m_defaultItems = defaultItems;
    setWidgetItems(items.isEmpty() ? defaultItems : items);
}

void ModelListWidget::setWidgetItems(const QList<AiModelData> &items)
{
    setRowCount(0);

    for (const auto &row : items)
        addRow(row);

    // Allow the user to resize columns manually
    horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);

    setCurrentIndex(QModelIndex());
}

QToolBar *ModelListWidget::toolBar() const { return m_toolBar.get(); }

void ModelListWidget::onRowChanged(int row)
{
    bool hasRow = row > -1;
    m_removeButton->setEnabled(hasRow);
    m_moveUpButton->setEnabled(hasRow && row != 0);
    m_moveDownButton->setEnabled(hasRow && row != rowCount() - 1);
}

void ModelListWidget::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    QTableWidget::closeEditor(editor, hint);
    int row = currentRow();
    if (row == -1)
        return;

    bool empty = true;
    for (int i = 0; i < columnCount(); ++i) {
        if (!item(row, i)->text().trimmed().isEmpty()) {
            empty = false;
            break;
        }
    }
    if (empty) {
        removeRow(row);
        setCurrentIndex(QModelIndex());
    }
}

} // namespace QmlDesigner

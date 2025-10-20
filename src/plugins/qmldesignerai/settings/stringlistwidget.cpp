// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stringlistwidget.h"

#include <componentcore/theme.h>
#include <utils/fileutils.h>
#include <utils/stylehelper.h>

#include <QToolBar>
#include <QToolButton>

namespace QmlDesigner {

static QIcon toolButtonIcon(Theme::Icon type)
{
    const QSize iconSize{24, 24};
    const QIcon normalIcon = Theme::iconFromName(type);
    const QIcon disabledIcon
        = Theme::iconFromName(type, Theme::getColor(Theme::Color::DStextSelectedTextColor));
    QIcon icon;
    icon.addPixmap(normalIcon.pixmap(iconSize), QIcon::Normal, QIcon::On);
    icon.addPixmap(disabledIcon.pixmap(iconSize), QIcon::Disabled, QIcon::On);
    return icon;
}

StringListWidget::StringListWidget(QWidget *parent)
    : QListWidget(parent)
    , m_addButton(Utils::makeUniqueObjectPtr<QToolButton>())
    , m_removeButton(Utils::makeUniqueObjectPtr<QToolButton>())
    , m_moveUpButton(Utils::makeUniqueObjectPtr<QToolButton>())
    , m_moveDownButton(Utils::makeUniqueObjectPtr<QToolButton>())
    , m_toolBar(Utils::makeUniqueObjectPtr<QToolBar>())
{
    setEditTriggers({
        QAbstractItemView::DoubleClicked,
        QAbstractItemView::EditKeyPressed,
    });

    QString sheet = Utils::FileUtils::fetchQrc(":/qmldesigner/stylesheet.css");
    setStyleSheet(Theme::replaceCssColors(sheet));

    m_addButton->setIcon(toolButtonIcon(Theme::plus));
    m_removeButton->setIcon(toolButtonIcon(Theme::minus));
    m_moveUpButton->setIcon(toolButtonIcon(Theme::moveUp_medium));
    m_moveDownButton->setIcon(toolButtonIcon(Theme::moveDown_medium));
    m_toolBar->addWidget(m_moveDownButton.get());
    m_toolBar->addWidget(m_moveUpButton.get());
    m_toolBar->addWidget(m_removeButton.get());
    m_toolBar->addWidget(m_addButton.get());

    connect(this, &QListWidget::currentRowChanged, this, &StringListWidget::onRowChanged);
    onRowChanged(currentRow());

    connect(m_addButton.get(), &QToolButton::clicked, this, [this] {
        addItem({});
        const int newRow = count() - 1;
        setCurrentRow(newRow);
        editItem(item(newRow));
    });

    connect(m_removeButton.get(), &QToolButton::clicked, this, [this] {
        delete takeItem(currentRow());
    });

    connect(m_moveUpButton.get(), &QToolButton::clicked, this, [this] {
        const int row = currentRow();
        const int newRow = row - 1;
        QListWidgetItem *rowItem = takeItem(row);
        insertItem(newRow, rowItem);
        setCurrentRow(newRow);
    });

    connect(m_moveDownButton.get(), &QToolButton::clicked, this, [this] {
        const int row = currentRow();
        const int newRow = row + 1;
        QListWidgetItem *rowItem = takeItem(row);
        insertItem(newRow, rowItem);
        setCurrentRow(newRow);
    });
}

StringListWidget::~StringListWidget() = default;

QStringList StringListWidget::items() const
{
    QStringList result;
    const int itemCount = count();
    result.reserve(itemCount);

    for (int i = 0; i < itemCount; ++i)
        result.append(item(i)->text());

    return result;
}

void StringListWidget::setItems(const QStringList &items)
{
    clear();

    for (const QString &itemText : items)
        addItem(itemText);
}

void StringListWidget::addItem(const QString &itemText)
{
    QListWidgetItem *item = new QListWidgetItem(itemText, this);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    QListWidget::addItem(item);
}

QToolBar *StringListWidget::toolBar() const
{
    return m_toolBar.get();
}

void StringListWidget::onRowChanged(int row)
{
    bool hasRowSelection = row > -1;
    m_removeButton->setEnabled(hasRowSelection);
    m_moveUpButton->setEnabled(hasRowSelection && row != 0);
    m_moveDownButton->setEnabled(hasRowSelection && row != count() - 1);
}

} // namespace QmlDesigner

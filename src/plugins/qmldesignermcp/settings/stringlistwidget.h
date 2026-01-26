// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QListWidget>

#include <utils/uniqueobjectptr.h>

Q_FORWARD_DECLARE_OBJC_CLASS(QToolBar);
Q_FORWARD_DECLARE_OBJC_CLASS(QToolButton);

namespace QmlDesigner {

class StringListWidget : public QListWidget
{
public:
    explicit StringListWidget(QWidget *parent = nullptr);
    ~StringListWidget();

    QStringList items() const;
    void setItems(const QStringList &items, const QStringList &defaultItems);
    void addItem(const QString &itemText);

    QToolBar *toolBar() const;

protected:
    void wheelEvent(QWheelEvent *event) override;
    void closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint) override;

private: // functions
    void onRowChanged(int row);
    void setWidgetItems(const QStringList &items);

private: // variables
    Utils::UniqueObjectPtr<QToolButton> m_addButton;
    Utils::UniqueObjectPtr<QToolButton> m_removeButton;
    Utils::UniqueObjectPtr<QToolButton> m_moveUpButton;
    Utils::UniqueObjectPtr<QToolButton> m_moveDownButton;
    Utils::UniqueObjectPtr<QToolButton> m_resetButton;
    Utils::UniqueObjectPtr<QToolBar> m_toolBar;
    QStringList m_defaultItems;
};

} // namespace QmlDesigner

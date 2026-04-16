// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "aidefaultsettings.h"

#include <utils/uniqueobjectptr.h>

#include <QList>
#include <QTableWidget>

Q_FORWARD_DECLARE_OBJC_CLASS(QToolBar);
Q_FORWARD_DECLARE_OBJC_CLASS(QToolButton);

namespace QmlDesigner {

class ModelListWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit ModelListWidget(QWidget *parent = nullptr);
    ~ModelListWidget() override;

    QSize sizeHint() const override;

    QList<AiModelData> items() const;
    void setItems(const QList<AiModelData> &items, const QList<AiModelData> &defaultItems);
    void addRow(const AiModelData &rowData = {});
    QToolBar *toolBar() const;

protected:
    void closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint) override;

private:
    void onRowChanged(int row);
    void setWidgetItems(const QList<AiModelData> &items);

    Utils::UniqueObjectPtr<QToolButton> m_addButton;
    Utils::UniqueObjectPtr<QToolButton> m_removeButton;
    Utils::UniqueObjectPtr<QToolButton> m_moveUpButton;
    Utils::UniqueObjectPtr<QToolButton> m_moveDownButton;
    Utils::UniqueObjectPtr<QToolButton> m_resetButton;
    Utils::UniqueObjectPtr<QToolBar> m_toolBar;

    QList<AiModelData> m_defaultItems;
};

} // namespace QmlDesigner

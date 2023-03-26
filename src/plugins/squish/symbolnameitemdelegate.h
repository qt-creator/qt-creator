// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fancylineedit.h>

#include <QStyledItemDelegate>

namespace Squish {
namespace Internal {

class SymbolNameItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    SymbolNameItemDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const override;
};

class ValidatingContainerNameLineEdit : public Utils::FancyLineEdit
{
    Q_OBJECT
public:
    ValidatingContainerNameLineEdit(const QStringList &forbidden, QWidget *parent = nullptr);

private:
    QStringList m_forbidden;
};

} // namespace Internal
} // namespace Squish

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "todooutputtreeviewdelegate.h"
#include "constants.h"

namespace Todo {
namespace Internal {

TodoOutputTreeViewDelegate::TodoOutputTreeViewDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

void TodoOutputTreeViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem newOption = option;
    newOption.textElideMode = index.column() == Constants::OUTPUT_COLUMN_FILE ? Qt::ElideLeft : Qt::ElideRight;
    QStyledItemDelegate::paint(painter, newOption, index);
}

} // namespace Internal
} // namespace Todo


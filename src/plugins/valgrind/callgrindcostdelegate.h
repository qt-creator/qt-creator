// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStyledItemDelegate>

namespace Valgrind::Internal {

class CostDelegate : public QStyledItemDelegate
{
public:
    explicit CostDelegate(QObject *parent = nullptr);
    ~CostDelegate() override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setModel(QAbstractItemModel *model);

    enum CostFormat {
        /// show absolute numbers
        FormatAbsolute,
        /// show percentages relative to the total inclusive cost
        FormatRelative,
        /// show percentages relative to the parent cost
        FormatRelativeToParent
    };

    void setFormat(CostFormat format);
    CostFormat format() const;

private:
    class Private;
    Private *d;
};

} // namespace Valgrind::Internal

Q_DECLARE_METATYPE(Valgrind::Internal::CostDelegate::CostFormat)

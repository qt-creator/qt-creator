// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testtreeitemdelegate.h"

#include "testtreeitem.h"

#include <QPainter>

namespace Autotest {
namespace Internal {

TestTreeItemDelegate::TestTreeItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void TestTreeItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    bool italic = index.data(ItalicRole).toBool();
    if (italic) {
        QFont font(option.font);
        font.setItalic(true);
        opt.font = font;

        // correct margin of items without a checkbox (except for root items)
        QStyleOptionButton styleOpt;
        styleOpt.initFrom(opt.widget);
        const QSize sz; // no text, no icon - we just need the size of the check box
        QSize cbSize = opt.widget->style()->sizeFromContents(QStyle::CT_CheckBox, &styleOpt, sz);
        // the 6 results from hard coded margins of the checkbox itself (2x2) and the item (1x2)
        opt.rect.setLeft(opt.rect.left() + cbSize.width() + 6);

        // HACK make sure the pixels that have been moved right are painted for selections
        if (opt.state & QStyle::State_Selected) {
            QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled
                    ? QPalette::Normal : QPalette::Disabled;
            if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
                cg = QPalette::Inactive;
            painter->fillRect(option.rect, opt.palette.brush(cg, QPalette::Highlight));
        }
    }

    // paint disabled items in gray
    if (!index.data(EnabledRole).toBool())
        opt.palette.setColor(QPalette::Text, QColor(0xa0, 0xa0, 0xa0));

    QStyledItemDelegate::paint(painter, opt, index);
}

} // namespace Internal
} // namespace Autotest

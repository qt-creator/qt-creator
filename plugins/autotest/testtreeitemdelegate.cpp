/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "testtreeitem.h"
#include "testtreeitemdelegate.h"
#include "testtreemodel.h"

#include <QPainter>

namespace Autotest {
namespace Internal {

TestTreeItemDelegate::TestTreeItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

TestTreeItemDelegate::~TestTreeItemDelegate()
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

    QStyledItemDelegate::paint(painter, opt, index);
}

} // namespace Internal
} // namespace Autotest

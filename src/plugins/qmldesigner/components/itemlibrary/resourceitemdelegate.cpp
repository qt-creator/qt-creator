/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "resourceitemdelegate.h"

#include "itemlibrarytreeview.h"

#include <QPainter>

namespace QmlDesigner {

ResourceItemDelegate::ResourceItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent),
      m_model(0) {}

void ResourceItemDelegate::paint(QPainter *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    if (option.state & QStyle::State_Selected)
        ItemLibraryTreeView::drawSelectionBackground(painter, option);

    painter->save();

    QIcon icon(m_model->fileIcon(index));
    QPixmap pixmap(icon.pixmap(icon.availableSizes().front()));
    painter->drawPixmap(option.rect.x(),option.rect.y()+2,pixmap);
    QString myString(m_model->fileName(index));

    // Check text length does not exceed available space
    int extraSpace=12+pixmap.width();
    QFontMetrics fm(option.font);
    myString = fm.elidedText(myString,Qt::ElideMiddle,option.rect.width()-extraSpace);

    painter->drawText(option.rect.bottomLeft()+QPoint(3+pixmap.width(),-8),myString);

    painter->restore();
}

QSize ResourceItemDelegate::sizeHint(const QStyleOptionViewItem &/*option*/,
                                     const QModelIndex &index) const
{
    QSize result = QSize(25, 4);
    const QIcon icon(m_model->fileIcon(index));
    if (!icon.isNull()) {
        const QList<QSize> sizes = icon.availableSizes();
        if (!sizes.isEmpty())
            result += sizes.front();
    }
    return result;
}

void ResourceItemDelegate::setModel(QFileSystemModel *model)
{
    m_model = model;
}

} // namespace QmlDesigner

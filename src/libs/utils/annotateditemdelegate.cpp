/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "annotateditemdelegate.h"

#include <QtCore/QSize>
#include <QtCore/QModelIndex>
#include <QtGui/QStandardItemModel>
#include <QtGui/QPainter>
#include <QtGui/QStyle>
#include <QtGui/QStyleOptionViewItemV4>
#include <QtGui/QApplication>

using namespace Utils;

AnnotatedItemDelegate::AnnotatedItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{}

AnnotatedItemDelegate::~AnnotatedItemDelegate()
{}

void AnnotatedItemDelegate::setAnnotationRole(int role)
{
    m_annotationRole = role;
}

int AnnotatedItemDelegate::annotationRole() const
{
    return m_annotationRole;
}

void AnnotatedItemDelegate::setDelimiter(const QString &delimiter)
{
    m_delimiter = delimiter;
}

const QString &AnnotatedItemDelegate::delimiter() const
{
    return m_delimiter;
}

void AnnotatedItemDelegate::paint(QPainter *painter,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);

    QStyle *style = QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    QString annotation = index.data(m_annotationRole).toString();
    if (!annotation.isEmpty()) {

        int newlinePos = annotation.indexOf(QLatin1Char('\n'));
        if (newlinePos != -1) {
            // print first line with '...' at end
            const QChar ellipsisChar(0x2026);
            annotation = annotation.left(newlinePos) + ellipsisChar;
        }

        QPalette disabled(opt.palette);
        disabled.setCurrentColorGroup(QPalette::Disabled);

        painter->save();
        painter->setPen(disabled.color(QPalette::WindowText));

        static int extra = opt.fontMetrics.width(m_delimiter) + 10;
        const QPixmap &pixmap = opt.icon.pixmap(opt.decorationSize);
        const QRect &iconRect = style->itemPixmapRect(opt.rect, opt.decorationAlignment, pixmap);
        const QRect &displayRect = style->itemTextRect(opt.fontMetrics, opt.rect,
            opt.displayAlignment, true, index.data(Qt::DisplayRole).toString());
        QRect annotationRect = style->itemTextRect(opt.fontMetrics, opt.rect,
            opt.displayAlignment, true, annotation);
        annotationRect.adjust(iconRect.width() + displayRect.width() + extra, 0,
                              iconRect.width() + displayRect.width() + extra, 0);

        QApplication::style()->drawItemText(painter, annotationRect,
            Qt::AlignLeft | Qt::AlignBottom, disabled, true, annotation);

        painter->restore();
    }
}

QSize AnnotatedItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);

    const QString &annotation = index.data(m_annotationRole).toString();
    if (!annotation.isEmpty())
        opt.text += m_delimiter + annotation;

    return QApplication::style()->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), 0);
}

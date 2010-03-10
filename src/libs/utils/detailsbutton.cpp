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

#include "detailsbutton.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QStyleOption>

#include <utils/stylehelper.h>

using namespace Utils;

DetailsButton::DetailsButton(QWidget *parent) : QAbstractButton(parent)
{
    setCheckable(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

QSize DetailsButton::sizeHint() const
{
    // TODO: Adjust this when icons become available!
    return QSize(80, 22);
}


void DetailsButton::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    QPainter p(this);
    if (isChecked()) {
        if (m_checkedPixmap.isNull() || m_checkedPixmap.size() != contentsRect().size())
            m_checkedPixmap = cacheRendering(contentsRect().size(), true);
        p.drawPixmap(contentsRect(), m_checkedPixmap);
    } else {
        if (m_uncheckedPixmap.isNull() || m_uncheckedPixmap.size() != contentsRect().size())
            m_uncheckedPixmap = cacheRendering(contentsRect().size(), false);
        p.drawPixmap(contentsRect(), m_uncheckedPixmap);
    }
}

QPixmap DetailsButton::cacheRendering(const QSize &size, bool checked)
{
    QLinearGradient lg;
    lg.setCoordinateMode(QGradient::ObjectBoundingMode);
    lg.setFinalStop(0, 1);

    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.translate(0.5, 0.5);
    p.setPen(Qt::NoPen);
    QColor color = palette().highlight().color();
    if(!checked) {
        lg.setColorAt(0, QColor(0, 0, 0, 10));
        lg.setColorAt(1, QColor(0, 0, 0, 16));
    } else {
        lg.setColorAt(0, QColor(255, 255, 255, 0));
        lg.setColorAt(1, QColor(255, 255, 255, 50));
    }

    p.setBrush(lg);
    p.setPen(QColor(255,255,255,140));
    p.drawRoundedRect(1, 1, size.width()-3, size.height()-3, 1, 1);
    p.setPen(QPen(QColor(0, 0, 0, 40)));
    p.drawLine(0, 1, 0, size.height() - 2);
    if(checked)
        p.drawLine(1, size.height() - 1, size.width() - 1, size.height() - 1);

    p.setPen(palette().color(QPalette::Text));

    QString text = tr("Details");
    QRect textRect = p.fontMetrics().boundingRect(text);
    textRect.setWidth(textRect.width() + 15);
    textRect.moveCenter(rect().center());

    p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);

    int arrowsize = 15;
    QStyleOption arrowOpt;
    arrowOpt.initFrom(this);
    QPalette pal = arrowOpt.palette;
    pal.setBrush(QPalette::All, QPalette::Text, QColor(0, 0, 0));
    arrowOpt.rect = QRect(size.width() - arrowsize - 6, height()/2-arrowsize/2, arrowsize, arrowsize);
    arrowOpt.palette = pal;
    style()->drawPrimitive(checked ? QStyle::PE_IndicatorArrowUp : QStyle::PE_IndicatorArrowDown, &arrowOpt, &p, this);
    return pixmap;
}

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

using namespace Utils;

DetailsButton::DetailsButton(QWidget *parent) : QAbstractButton(parent)
{
    setCheckable(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

QSize DetailsButton::sizeHint() const
{
    // TODO: Adjust this when icons become available!
    return QSize(40, 22);
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

    if (checked) {
        lg.setColorAt(0, palette().color(QPalette::Midlight));
        lg.setColorAt(1, palette().color(QPalette::Button));
    } else {
        lg.setColorAt(0, palette().color(QPalette::Button));
        lg.setColorAt(1, palette().color(QPalette::Midlight));
    }

    QPixmap pixmap(size);
    QPainter p(&pixmap);
    p.setBrush(lg);
    p.setPen(Qt::NoPen);

    p.drawRect(0, 0, size.width(), size.height());

    p.setPen(QPen(palette().color(QPalette::Mid)));
    p.drawLine(0, size.height() - 1, 0, 0);
    p.drawLine(0, 0,                 size.width() - 1, 0);
    p.drawLine(size.width() - 1, 0,  size.width() - 1, size.height() - 1);
    if (!checked)
        p.drawLine(size.width() - 1, size.height() - 1, 0, size.height() - 1);

    p.setPen(palette().color(QPalette::Text));

    // TODO: This should actually use some icons instead...
    if (checked) {
        p.drawText(0, 0, size.width(), size.height(), Qt::AlignCenter, tr("Less"));
    } else {
        p.drawText(0, 0, size.width(), size.height(), Qt::AlignCenter, tr("More"));
    }

    return pixmap;
}

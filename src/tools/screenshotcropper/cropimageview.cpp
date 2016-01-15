/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cropimageview.h"
#include <QPainter>
#include <QMouseEvent>

CropImageView::CropImageView(QWidget *parent)
    : QWidget(parent)
{
}

void CropImageView::mousePressEvent(QMouseEvent *event)
{
    setArea(QRect(event->pos(), m_area.bottomRight()));
    update();
}

void CropImageView::mouseMoveEvent(QMouseEvent *event)
{
    setArea(QRect(m_area.topLeft(), event->pos()));
    update();
}

void CropImageView::mouseReleaseEvent(QMouseEvent *event)
{
    mouseMoveEvent(event);
}

void CropImageView::setImage(const QImage &image)
{
    m_image = image;
    setMinimumSize(image.size());
    update();
}

void CropImageView::setArea(const QRect &area)
{
    m_area = m_image.rect().intersected(area);
    emit cropAreaChanged(m_area);
    update();
}

void CropImageView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);

    if (!m_image.isNull())
        p.drawImage(0, 0, m_image);

    if (!m_area.isNull()) {
        p.setPen(Qt::white);
        p.drawRect(m_area);
        QPen redDashes;
        redDashes.setColor(Qt::red);
        redDashes.setStyle(Qt::DashLine);
        p.setPen(redDashes);
        p.drawRect(m_area);
    }
}


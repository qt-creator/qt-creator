/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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


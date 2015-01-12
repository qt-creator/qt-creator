/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "progressindicator.h"
#include "stylehelper.h"

#include <QPainter>
#include <QPixmap>

using namespace Utils;

ProgressIndicator::ProgressIndicator(Size size, QWidget *parent)
    : QWidget(parent),
      m_rotation(0)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    m_size = size;
    m_rotationStep = size == Small ? 45 : 30;
    m_pixmap.load(StyleHelper::dpiSpecificImageFile(
                      size == Small ? QLatin1String(":/utils/images/progressindicator_small.png")
                                    : QLatin1String(":/utils/images/progressindicator_big.png")));
    m_timer.setInterval(size == Small ? 100 : 80);
    m_timer.setSingleShot(false);
    connect(&m_timer, &QTimer::timeout, this, &ProgressIndicator::step);
}

QSize ProgressIndicator::sizeHint() const
{
    return m_pixmap.size() / m_pixmap.devicePixelRatio();
}

void ProgressIndicator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    QPoint translate(rect().width() / 2, rect().height() / 2);
    QTransform t;
    t.translate(translate.x(), translate.y());
    t.rotate(m_rotation);
    t.translate(-translate.x(), -translate.y());
    p.setTransform(t);
    QSize pixmapUserSize(m_pixmap.size() / m_pixmap.devicePixelRatio());
    p.drawPixmap(QPoint((rect().width() - pixmapUserSize.width()) / 2,
                       (rect().height() - pixmapUserSize.height()) / 2),
                 m_pixmap);
}

void ProgressIndicator::showEvent(QShowEvent *)
{
    m_timer.start();
}

void ProgressIndicator::hideEvent(QHideEvent *)
{
    m_timer.stop();
}

void ProgressIndicator::step()
{
    m_rotation = (m_rotation + m_rotationStep + 360) % 360;
    update();
}


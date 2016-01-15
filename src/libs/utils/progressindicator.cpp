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

#include "progressindicator.h"

#include "qtcassert.h"
#include "stylehelper.h"

#include <QEvent>
#include <QPainter>
#include <QPixmap>

using namespace Utils;

ProgressIndicator::ProgressIndicator(IndicatorSize size, QWidget *parent)
    : QWidget(parent),
      m_rotation(0)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    m_timer.setSingleShot(false);
    connect(&m_timer, &QTimer::timeout, this, &ProgressIndicator::step);
    setIndicatorSize(size);
}

static QString imageFileNameForIndicatorSize(ProgressIndicator::IndicatorSize size)
{
    switch (size) {
        case ProgressIndicator::Large:
            return QLatin1String(":/utils/images/progressindicator_big.png");
        case ProgressIndicator::Medium:
            return QLatin1String(":/utils/images/progressindicator_medium.png");
        case ProgressIndicator::Small:
        default:
            return QLatin1String(":/utils/images/progressindicator_small.png");
    }
}

void ProgressIndicator::setIndicatorSize(ProgressIndicator::IndicatorSize size)
{
    m_size = size;
    m_rotationStep = size == Small ? 45 : 30;
    m_timer.setInterval(size == Small ? 100 : 80);
    m_pixmap.load(StyleHelper::dpiSpecificImageFile(
                      imageFileNameForIndicatorSize(size)));
    updateGeometry();
}

ProgressIndicator::IndicatorSize ProgressIndicator::indicatorSize() const
{
    return m_size;
}

QSize ProgressIndicator::sizeHint() const
{
    return m_pixmap.size() / m_pixmap.devicePixelRatio();
}

void ProgressIndicator::attachToWidget(QWidget *parent)
{
    if (parentWidget())
        parentWidget()->removeEventFilter(this);
    setParent(parent);
    parent->installEventFilter(this);
    resizeToParent();
    raise();
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

bool ProgressIndicator::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == parent() && ev->type() == QEvent::Resize) {
        resizeToParent();
    }
    return QWidget::eventFilter(obj, ev);
}

void ProgressIndicator::step()
{
    m_rotation = (m_rotation + m_rotationStep + 360) % 360;
    update();
}

void ProgressIndicator::resizeToParent()
{
    QTC_ASSERT(parentWidget(), return);
    setGeometry(QRect(QPoint(0, 0), parentWidget()->size()));
}


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

#include "icon.h"
#include "qtcassert.h"
#include "stylehelper.h"

#include <QEvent>
#include <QPainter>
#include <QPixmap>

namespace {

static QString imageFileNameForIndicatorSize(Utils::ProgressIndicatorSize size)
{
    switch (size) {
    case Utils::ProgressIndicatorSize::Large:
        return QLatin1String(":/utils/images/progressindicator_big.png");
    case Utils::ProgressIndicatorSize::Medium:
        return QLatin1String(":/utils/images/progressindicator_medium.png");
    case Utils::ProgressIndicatorSize::Small:
    default:
            return QLatin1String(":/utils/images/progressindicator_small.png");
    }
}

} // namespace

namespace Utils {

ProgressIndicatorPainter::ProgressIndicatorPainter(ProgressIndicatorSize size)
{
    m_timer.setSingleShot(false);
    QObject::connect(&m_timer, &QTimer::timeout, [this]() {
        nextAnimationStep();
        if (m_callback)
            m_callback();
    });

    setIndicatorSize(size);
}

void ProgressIndicatorPainter::setIndicatorSize(ProgressIndicatorSize size)
{
    m_size = size;
    m_rotationStep = size == ProgressIndicatorSize::Small ? 45 : 30;
    m_timer.setInterval(size == ProgressIndicatorSize::Small ? 100 : 80);
    m_pixmap = Icon({{imageFileNameForIndicatorSize(size),
                      Theme::PanelTextColorMid}}, Icon::Tint).pixmap();
}

ProgressIndicatorSize ProgressIndicatorPainter::indicatorSize() const
{
    return m_size;
}

void ProgressIndicatorPainter::setUpdateCallback(const UpdateCallback &cb)
{
    m_callback = cb;
}

QSize ProgressIndicatorPainter::size() const
{
    return m_pixmap.size() / m_pixmap.devicePixelRatio();
}

// Paint indicator centered on the rect
void ProgressIndicatorPainter::paint(QPainter &painter, const QRect &rect) const
{
    painter.save();
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPoint translate(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
    QTransform t;
    t.translate(translate.x(), translate.y());
    t.rotate(m_rotation);
    t.translate(-translate.x(), -translate.y());
    painter.setTransform(t);
    QSize pixmapUserSize(m_pixmap.size() / m_pixmap.devicePixelRatio());
    painter.drawPixmap(QPoint(rect.x() + ((rect.width() - pixmapUserSize.width()) / 2),
                              rect.y() + ((rect.height() - pixmapUserSize.height()) / 2)),
                 m_pixmap);
    painter.restore();
}

void ProgressIndicatorPainter::startAnimation()
{
    QTC_ASSERT(m_callback, return);
    m_timer.start();
}

void ProgressIndicatorPainter::stopAnimation()
{
    m_timer.stop();
}

void Utils::ProgressIndicatorPainter::nextAnimationStep()
{
    m_rotation = (m_rotation + m_rotationStep + 360) % 360;
}

ProgressIndicator::ProgressIndicator(ProgressIndicatorSize size, QWidget *parent)
    : QWidget(parent), m_paint(size)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    m_paint.setUpdateCallback([this]() { update(); });
    updateGeometry();
}

void ProgressIndicator::setIndicatorSize(ProgressIndicatorSize size)
{
    m_paint.setIndicatorSize(size);
    updateGeometry();
}

QSize ProgressIndicator::sizeHint() const
{
    return m_paint.size();
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
    m_paint.paint(p, rect());
}

void ProgressIndicator::showEvent(QShowEvent *)
{
    m_paint.startAnimation();
}

void ProgressIndicator::hideEvent(QHideEvent *)
{
    m_paint.stopAnimation();
}

bool ProgressIndicator::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == parent() && ev->type() == QEvent::Resize) {
        resizeToParent();
    }
    return QWidget::eventFilter(obj, ev);
}

void ProgressIndicator::resizeToParent()
{
    QTC_ASSERT(parentWidget(), return);
    setGeometry(QRect(QPoint(0, 0), parentWidget()->size()));
}

} // namespace Utils

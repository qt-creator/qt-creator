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

/*!
    \class Utils::ProgressIndicator
    \brief The ProgressIndicator class shows an circular, endlessly animated progress indicator.

    Use it if you want to indicate that some work is being done, but you do not have the detailed
    progress information needed for a progress bar. You can either create the widget on demand,
    or create the widget once and only show it on demand. The animation only runs while the widget
    is visible.

    \inmodule Qt Creator
*/

/*!
    \class Utils::ProgressIndicatorPainter
    \brief The ProgressIndicatorPainter class is the painting backend for the ProgressIndicator
    class.

    You can use it to paint a circular, endlessly animated progress indicator directly onto a
    QPaintDevice, for example, if you want to show a progress indicator where you cannot use
    a QWidget.

    \inmodule Qt Creator
*/

/*!
    \enum Utils::ProgressIndicatorSize

    Size of a progress indicator.
    \sa Utils::ProgressIndicator
    \sa Utils::ProgressIndicatorPainter

    \value Small
           Small icon size. Useful for tool bars, status bars, rows in tree views,
           and so on.
    \value Medium
           Larger progress indicator useful for covering whole medium sized widgets.
    \value Large
           Very large progress indicator that can be used to cover large parts of a UI.
*/

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

/*!
    Constructs a progress indicator painter for the indicator \a size.

    \sa setUpdateCallback
*/
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

/*!
    Changes the size of the progress indicator to \a size. Users of the class need
    to adapt their painting or layouting code to the change in resulting pixel size.

    \sa indicatorSize
    \sa size
*/
void ProgressIndicatorPainter::setIndicatorSize(ProgressIndicatorSize size)
{
    m_size = size;
    m_rotationStep = size == ProgressIndicatorSize::Small ? 45 : 30;
    m_timer.setInterval(size == ProgressIndicatorSize::Small ? 100 : 80);
    m_pixmap = Icon({{imageFileNameForIndicatorSize(size),
                      Theme::PanelTextColorMid}}, Icon::Tint).pixmap();
}

/*!
    Returns the current indicator size. Use \l size to get the resulting
    pixel size.

    \sa setIndicatorSize
*/
ProgressIndicatorSize ProgressIndicatorPainter::indicatorSize() const
{
    return m_size;
}

/*!
    Sets the callback \a cb that is called whenever the progress indicator needs a repaint, because
    its animation progressed. The callback is a void function taking no parameters, and should
    usually trigger a QWidget::update on the widget that does the actual painting.
*/
void ProgressIndicatorPainter::setUpdateCallback(const UpdateCallback &cb)
{
    m_callback = cb;
}

/*!
    Returns the size of the progress indicator in device independent pixels.

    \sa setIndicatorSize
    \sa paint
*/
QSize ProgressIndicatorPainter::size() const
{
    return m_pixmap.size() / m_pixmap.devicePixelRatio();
}

/*!
    Paints the progress indicator centered in the \a rect on the given \a painter.

    \sa size
*/
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

/*!
    Starts the progress indicator animation.

    \sa setUpdateCallback
    \sa stopAnimation
*/
void ProgressIndicatorPainter::startAnimation()
{
    QTC_ASSERT(m_callback, return);
    m_timer.start();
}

/*!
    Stops the progress indicator animation.

    \sa setUpdateCallback
    \sa startAnimation
*/
void ProgressIndicatorPainter::stopAnimation()
{
    m_timer.stop();
}

/*!
    \internal
*/
void ProgressIndicatorPainter::nextAnimationStep()
{
    m_rotation = (m_rotation + m_rotationStep + 360) % 360;
}

/*!
    Constructs a ProgressIndicator of the size \a size and with the parent \a parent.

    Use \l attachToWidget to make the progress indicator automatically resize and center on the
    parent widget.

    \sa attachToWidget
    \sa setIndicatorSize
*/
ProgressIndicator::ProgressIndicator(ProgressIndicatorSize size, QWidget *parent)
    : OverlayWidget(parent)
    , m_paint(size)
{
    setPaintFunction(
        [this](QWidget *w, QPainter &p, QPaintEvent *) { m_paint.paint(p, w->rect()); });
    m_paint.setUpdateCallback([this]() { update(); });
    updateGeometry();
}

/*!
    Changes the size of the progress indicator to \a size.

    \sa indicatorSize
*/
void ProgressIndicator::setIndicatorSize(ProgressIndicatorSize size)
{
    m_paint.setIndicatorSize(size);
    updateGeometry();
}

/*!
    Returns the size of the indicator in device independent pixels.

    \sa indicatorSize
*/
QSize ProgressIndicator::sizeHint() const
{
    return m_paint.size();
}

/*!
    \internal
*/
void ProgressIndicator::showEvent(QShowEvent *)
{
    m_paint.startAnimation();
}

/*!
    \internal
*/
void ProgressIndicator::hideEvent(QHideEvent *)
{
    m_paint.stopAnimation();
}

} // namespace Utils

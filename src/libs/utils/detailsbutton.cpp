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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "detailsbutton.h"
#include "hostosinfo.h"
#include "theme/theme.h"

#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QPropertyAnimation>
#include <QPaintEvent>
#include <QPainter>
#include <QStyleOption>

using namespace Utils;

FadingWidget::FadingWidget(QWidget *parent) :
    FadingPanel(parent),
    m_opacityEffect(new QGraphicsOpacityEffect)
{
    m_opacityEffect->setOpacity(0);
    setGraphicsEffect(m_opacityEffect);

    // Workaround for issue with QGraphicsEffect. GraphicsEffect
    // currently clears with Window color. Remove if flickering
    // no longer occurs on fade-in
    QPalette pal;
    pal.setBrush(QPalette::All, QPalette::Window, Qt::transparent);
    setPalette(pal);
}

void FadingWidget::setOpacity(qreal value)
{
    m_opacityEffect->setOpacity(value);
}

void FadingWidget::fadeTo(qreal value)
{
    QPropertyAnimation *animation = new QPropertyAnimation(m_opacityEffect, "opacity");
    animation->setDuration(200);
    animation->setEndValue(value);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

qreal FadingWidget::opacity()
{
    return m_opacityEffect->opacity();
}

DetailsButton::DetailsButton(QWidget *parent) : QAbstractButton(parent), m_fader(0)
{
    setCheckable(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    setText(tr("Details"));
}

QSize DetailsButton::sizeHint() const
{
    // TODO: Adjust this when icons become available!
    const int w = fontMetrics().width(text()) + 32;
    if (HostOsInfo::isMacHost())
        return QSize(w, 34);
    return QSize(w, 22);
}

bool DetailsButton::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::Enter:
        {
            QPropertyAnimation *animation = new QPropertyAnimation(this, "fader");
            animation->setDuration(200);
            animation->setEndValue(1.0);
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }
        break;
    case QEvent::Leave:
        {
            QPropertyAnimation *animation = new QPropertyAnimation(this, "fader");
            animation->setDuration(200);
            animation->setEndValue(0.0);
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }
        break;
    default:
        return QAbstractButton::event(e);
    }
    return false;
}

void DetailsButton::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    QPainter p(this);

    // draw hover animation
    if (!HostOsInfo::isMacHost() && !isDown() && m_fader > 0) {
        QColor c = creatorTheme()->color(Theme::DetailsButtonBackgroundColorHover);
        c.setAlpha (int(m_fader * c.alpha()));

        QRect r = rect();
        if (creatorTheme()->widgetStyle() == Theme::StyleDefault)
            r.adjust(1, 1, -2, -2);
        p.fillRect(r, c);
    }

    if (isChecked()) {
        if (m_checkedPixmap.isNull() || m_checkedPixmap.size() / m_checkedPixmap.devicePixelRatio() != contentsRect().size())
            m_checkedPixmap = cacheRendering(contentsRect().size(), true);
        p.drawPixmap(contentsRect(), m_checkedPixmap);
    } else {
        if (m_uncheckedPixmap.isNull() || m_uncheckedPixmap.size() / m_uncheckedPixmap.devicePixelRatio() != contentsRect().size())
            m_uncheckedPixmap = cacheRendering(contentsRect().size(), false);
        p.drawPixmap(contentsRect(), m_uncheckedPixmap);
    }
    if (isDown()) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 20));
        p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 1, 1);
    }
    if (hasFocus()) {
        QStyleOptionFocusRect option;
        option.initFrom(this);
        style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &p, this);
    }
}

QPixmap DetailsButton::cacheRendering(const QSize &size, bool checked)
{
    const qreal pixelRatio = devicePixelRatio();
    QPixmap pixmap(size * pixelRatio);
    pixmap.setDevicePixelRatio(pixelRatio);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.translate(0.5, 0.5);

    if (creatorTheme()->widgetStyle() == Theme::StyleDefault) {
        QLinearGradient lg;
        lg.setCoordinateMode(QGradient::ObjectBoundingMode);
        lg.setFinalStop(0, 1);
        if (!checked) {
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
        if (checked)
            p.drawLine(1, size.height() - 1, size.width() - 1, size.height() - 1);
    } else {
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(0, 0, size.width(), size.height(), 1, 1);
    }

    p.setPen(palette().color(QPalette::Text));

    QRect textRect = p.fontMetrics().boundingRect(text());
    textRect.setWidth(textRect.width() + 15);
    textRect.setHeight(textRect.height() + 4);
    textRect.moveCenter(rect().center());

    p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text());

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

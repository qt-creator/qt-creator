// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "seekerslider.h"

#include <utils/icon.h>

#include <QMouseEvent>
#include <QStyleOption>
#include <QSlider>
#include <QPainter>

namespace QmlDesigner {

SeekerSlider::SeekerSlider(QWidget *parent)
    : QSlider(parent)
{
    setProperty("panelwidget", true);
    setProperty("panelwidget_singlerow", true);
    setProperty("DSSlider", true);
    setOrientation(Qt::Horizontal);
    setFixedWidth(120);
    setMaxValue(30);
}

int SeekerSlider::maxValue() const
{
    return maximum();
}

void SeekerSlider::setMaxValue(int maxValue)
{
    maxValue = std::abs(maxValue);
    setRange(-maxValue, +maxValue);
}

void SeekerSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    QStyleOptionSlider os;
    initStyleOption(&os);
    QRect handleRect = style()->subControlRect(QStyle::CC_Slider, &os, QStyle::SC_SliderHandle, this);
    m_moving = handleRect.contains(event->localPos().toPoint());
    if (m_moving)
        QSlider::mousePressEvent(event);
    else
        event->setAccepted(false);
}

void SeekerSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_moving)
        return;

    QSlider::mouseMoveEvent(event);
}

void SeekerSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_moving)
        return;

    setValue(0);
    m_moving = false;
    QSlider::mouseReleaseEvent(event);
}

} // namespace QmlDesigner

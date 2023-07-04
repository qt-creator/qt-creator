// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "seekerslider.h"

#include <utils/icon.h>
#include <utils/stylehelper.h>

#include <QMouseEvent>
#include <QStyleOption>
#include <QSlider>
#include <QPainter>

namespace QmlDesigner {

SeekerSlider::SeekerSlider(QWidget *parent)
    : QSlider(parent)
{
    Utils::StyleHelper::setPanelWidget(this);
    Utils::StyleHelper::setPanelWidgetSingleRow(this);
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

SeekerSliderAction::SeekerSliderAction(QObject *parent)
    : QWidgetAction(parent)
    , m_defaultSlider(new SeekerSlider())
{
    setDefaultWidget(m_defaultSlider);
    QObject::connect(m_defaultSlider, &QSlider::valueChanged, this, &SeekerSliderAction::valueChanged);
}

SeekerSliderAction::~SeekerSliderAction()
{
    m_defaultSlider->deleteLater();
}

SeekerSlider *SeekerSliderAction::defaultSlider() const
{
    return m_defaultSlider;
}

int SeekerSliderAction::value()
{
    return m_defaultSlider->value();
}

QWidget *SeekerSliderAction::createWidget(QWidget *parent)
{
    SeekerSlider *slider = new SeekerSlider(parent);

    QObject::connect(m_defaultSlider, &SeekerSlider::valueChanged, slider, &SeekerSlider::setValue);
    QObject::connect(slider, &SeekerSlider::valueChanged, m_defaultSlider, &SeekerSlider::setValue);
    QObject::connect(m_defaultSlider, &QSlider::rangeChanged, slider, &QSlider::setRange);
    QObject::connect(this, &QWidgetAction::enabledChanged, slider, &QSlider::setEnabled);

    slider->setValue(m_defaultSlider->value());
    slider->setMaxValue(m_defaultSlider->maxValue());
    slider->setEnabled(isEnabled());

    return slider;
}

} // namespace QmlDesigner

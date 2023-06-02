// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "navigatorslider.h"

#include <QSlider>
#include <QToolButton>

#include <utils/layoutbuilder.h>
#include <utils/utilsicons.h>

using namespace ScxmlEditor::Common;

NavigatorSlider::NavigatorSlider(QWidget *parent)
    : QFrame(parent)
{
    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setMinimum(0);
    m_slider->setMaximum(100);

    auto zoomIn = new QToolButton;
    zoomIn->setIcon(Utils::Icons::PLUS.icon());
    auto zoomOut = new QToolButton;
    zoomOut->setIcon(Utils::Icons::MINUS.icon());
    for (auto btn : {zoomIn, zoomOut}) {
        btn->setAutoRaise(true);
        btn->setAutoRepeat(true);
        btn->setAutoRepeatDelay(200);
        btn->setAutoRepeatInterval(10);
    }

    using namespace Layouting;
    Row {
        spacing(0),
        zoomOut,
        m_slider,
        zoomIn,
        Space(20),
        noMargin,
    }.attachTo(this);

    connect(zoomOut, &QToolButton::clicked, this, &NavigatorSlider::zoomOut);
    connect(zoomIn, &QToolButton::clicked, this, &NavigatorSlider::zoomIn);
    connect(m_slider, &QSlider::valueChanged, this, [=](int newValue){
        emit valueChanged(newValue);
    });
}

void NavigatorSlider::zoomIn()
{
    m_slider->setValue(m_slider->value() + 1);
}

void NavigatorSlider::zoomOut()
{
    m_slider->setValue(m_slider->value() - 1);
}

int NavigatorSlider::value() const
{
    return m_slider->value();
}

void NavigatorSlider::setSliderValue(int val)
{
    QSignalBlocker blocker(m_slider);
    m_slider->setValue(val);
}

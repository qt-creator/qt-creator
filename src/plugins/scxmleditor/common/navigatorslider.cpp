// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "navigatorslider.h"

using namespace ScxmlEditor::Common;

NavigatorSlider::NavigatorSlider(QWidget *parent)
    : QFrame(parent)
{
    m_ui.setupUi(this);

    connect(m_ui.m_zoomOut, &QToolButton::clicked, this, &NavigatorSlider::zoomOut);
    connect(m_ui.m_zoomIn, &QToolButton::clicked, this, &NavigatorSlider::zoomIn);
    connect(m_ui.m_slider, &QSlider::valueChanged, this, [=](int newValue){
        emit valueChanged(newValue);
    });
}

void NavigatorSlider::zoomIn()
{
    m_ui.m_slider->setValue(m_ui.m_slider->value() + 1);
}

void NavigatorSlider::zoomOut()
{
    m_ui.m_slider->setValue(m_ui.m_slider->value() - 1);
}

void NavigatorSlider::setSliderValue(int val)
{
    QSignalBlocker blocker(m_ui.m_slider);
    m_ui.m_slider->setValue(val);
}

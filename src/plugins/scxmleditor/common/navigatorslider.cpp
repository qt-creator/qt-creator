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

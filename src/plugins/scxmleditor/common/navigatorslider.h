// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "ui_navigatorslider.h"

#include <QFrame>

namespace ScxmlEditor {

namespace Common {

class NavigatorSlider : public QFrame
{
    Q_OBJECT

public:
    explicit NavigatorSlider(QWidget *parent = nullptr);

    int value() const
    {
        return m_ui.m_slider->value();
    }

    void setSliderValue(int val);
    void zoomIn();
    void zoomOut();

signals:
    void valueChanged(int);

private:
    Ui::NavigatorSlider m_ui;
};

} // namespace Common
} // namespace ScxmlEditor

// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include <QSlider>

namespace QmlDesigner {
class SeekerSlider : public QSlider
{
    Q_OBJECT

public:
    explicit SeekerSlider(QWidget *parent = nullptr);

    int maxValue() const;
    void setMaxValue(int maxValue);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    using QSlider::setMinimum;
    using QSlider::setMaximum;
    using QSlider::setRange;

    bool m_moving = false;
};

} // namespace QmlDesigner

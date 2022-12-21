// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QFrame>

QT_BEGIN_NAMESPACE
class QSlider;
QT_END_NAMESPACE

namespace ScxmlEditor {

namespace Common {

class NavigatorSlider : public QFrame
{
    Q_OBJECT

public:
    explicit NavigatorSlider(QWidget *parent = nullptr);

    int value() const;
    void setSliderValue(int val);
    void zoomIn();
    void zoomOut();

signals:
    void valueChanged(int);

private:
    QSlider *m_slider;
};

} // namespace Common
} // namespace ScxmlEditor

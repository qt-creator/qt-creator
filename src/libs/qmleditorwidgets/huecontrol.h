// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmleditorwidgets_global.h"
#include <QWidget>

namespace QmlEditorWidgets {

class QMLEDITORWIDGETS_EXPORT HueControl : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal hue READ hue WRITE setHue NOTIFY hueChanged)

public:
    HueControl(QWidget *parent = nullptr) : QWidget(parent), m_color(Qt::white), m_mousePressed(false)
    {
        setFixedWidth(28);
        setFixedHeight(130);
    }

    void setHue(int newHue);
    int hue() const { return m_color.hsvHue(); }

signals:
    void hueChanged(int hue);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void setCurrent(int y);

private:
    QColor m_color;
    bool m_mousePressed;
    QPixmap m_cache;
};

} //QmlEditorWidgets

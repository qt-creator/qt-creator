// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmleditorwidgets_global.h"
#include <QWidget>

namespace QmlEditorWidgets {

class QMLEDITORWIDGETS_EXPORT ColorBox : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString strColor READ strColor WRITE setStrColor NOTIFY colorChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(int hue READ hue WRITE setHue NOTIFY hueChanged)
    Q_PROPERTY(int saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int alpha READ alpha WRITE setAlpha NOTIFY alphaChanged)

public:
    ColorBox(QWidget *parent = nullptr) : QWidget(parent), m_color(Qt::white), m_saturatedColor(Qt::white), m_mousePressed(false), m_lastHue(0)
    {
        setFixedWidth(130);
        setFixedHeight(130);
    }

    void setHue(int newHue);
    int hue() const;
    void setAlpha(int newAlpha);
    int alpha() const { return m_color.alpha(); }
    void setStrColor(const QString &colorStr);
    void setColor(const QColor &color);
    QString strColor() const;
    QColor color() const { return m_color; }
    int saturation() const { return m_color.hsvSaturation(); }
    void setSaturation(int newsaturation);
    int value() const { return m_color.value(); }
    void setValue(int newvalue);

signals:
    void colorChanged();
    void hueChanged();
    void saturationChanged();
    void valueChanged();
    void alphaChanged();

protected:
    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void setCurrent(int x, int y);

private:
    QColor m_color;
    QColor m_saturatedColor;
    bool m_mousePressed;
    int m_lastHue;
    QPixmap m_cache;
};

} //QmlEditorWidgets

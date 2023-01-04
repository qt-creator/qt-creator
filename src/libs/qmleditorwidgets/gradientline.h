// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmleditorwidgets_global.h"
#include <QWidget>
#include <QLinearGradient>

namespace QmlEditorWidgets {

class QMLEDITORWIDGETS_EXPORT GradientLine : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor activeColor READ activeColor WRITE setActiveColor NOTIFY activeColorChanged)
    Q_PROPERTY(QString gradientName READ gradientName WRITE setGradientName NOTIFY gradientNameChanged)
    Q_PROPERTY(bool active READ active WRITE setActive)
    Q_PROPERTY(QLinearGradient gradient READ gradient WRITE setGradient NOTIFY gradientChanged)

public:
    GradientLine(QWidget *parent = nullptr);

    QString gradientName() const { return m_gradientName; }
    void setGradientName(const QString &newName);
    QColor activeColor() const { return m_activeColor; }
    void setActiveColor(const QColor &newColor);
    bool active() const { return m_active; }
    void setActive(bool a) { m_active = a; }
    QLinearGradient gradient() const { return m_gradient; }
    void setGradient(const QLinearGradient &);

signals:
    void activeColorChanged();
    void itemNodeChanged();
    void gradientNameChanged();
    void gradientChanged();
    void openColorDialog(const QPoint &pos);
protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent * event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;

private:
    void setup();
    void readGradient();
    void updateGradient();
    int currentColorIndex() const { return m_colorIndex; }
    void setCurrentIndex(int i);

    QColor m_activeColor;
    QString m_gradientName;
    QList<QColor> m_colorList;
    QList<qreal> m_stops;
    int m_colorIndex;
    bool m_dragActive;
    QPoint m_dragStart;
    QLinearGradient m_gradient;
    int m_yOffset;
    bool m_create;
    bool m_active;
    bool m_dragOff;
    bool m_useGradient;

};

} //QmlEditorWidgets

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
    GradientLine(QWidget *parent = 0);

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
    bool event(QEvent *event);
    void keyPressEvent(QKeyEvent * event);
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);

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

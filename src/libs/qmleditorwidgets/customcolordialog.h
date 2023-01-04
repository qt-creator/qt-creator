// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmleditorwidgets_global.h"

#include <QFrame>

QT_BEGIN_NAMESPACE
class QDoubleSpinBox;
QT_END_NAMESPACE

namespace QmlEditorWidgets {

class ColorBox;
class HueControl;

class QMLEDITORWIDGETS_EXPORT CustomColorDialog : public QFrame {

    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    CustomColorDialog(QWidget *parent = nullptr);
    QColor color() const { return m_color; }
    void setupColor(const QColor &color);
    void setColor(const QColor &color)
    {
        if (color == m_color)
            return;

        m_color = color;
        setupWidgets();
        emit colorChanged();
    }

    void changeColor(const QColor &color) { setColor(color); }
    void spinBoxChanged();
    void onColorBoxChanged();
    void onHueChanged(int newHue)
    {
        if (m_blockUpdate)
            return;

        if (m_color.hsvHue() == newHue)
            return;
        m_color.setHsv(newHue, m_color.hsvSaturation(), m_color.value());
        setupWidgets();
        emit colorChanged();
    }
    void onAccept()
    {
        emit accepted(m_color);
    }

signals:
    void colorChanged();
    void accepted(const QColor &color);
    void rejected();

protected:
    void setupWidgets();
    void leaveEvent(QEvent *) override;
    void enterEvent(QEnterEvent *) override;

private:
    QFrame *m_beforeColorWidget;
    QFrame *m_currentColorWidget;
    ColorBox *m_colorBox;
    HueControl *m_hueControl;

    QDoubleSpinBox *m_rSpinBox;
    QDoubleSpinBox *m_gSpinBox;
    QDoubleSpinBox *m_bSpinBox;
    QDoubleSpinBox *m_alphaSpinBox;

    QColor m_color;
    bool m_blockUpdate;
};

} //QmlEditorWidgets

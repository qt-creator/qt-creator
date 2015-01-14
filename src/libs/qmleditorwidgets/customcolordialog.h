/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CUSTOMCOLORDIALOG_H
#define CUSTOMCOLORDIALOG_H

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
    CustomColorDialog(QWidget *parent = 0);
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

public slots:
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
    void leaveEvent(QEvent *);
    void enterEvent(QEvent *);

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

#endif //CUSTOMCOLORDIALOG_H

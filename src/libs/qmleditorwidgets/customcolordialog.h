/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CUSTOMCOLORDIALOG_H
#define CUSTOMCOLORDIALOG_H

#include <qmleditorwidgets_global.h>
#include <QFrame>
#include <QDialog>

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

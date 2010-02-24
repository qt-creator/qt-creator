/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef COLORWIDGET_H
#define COLORWIDGET_H

#include <QWeakPointer>
#include <QtGui/QWidget>
#include <QLabel>
#include <QToolButton>
#include <QMouseEvent>
#include <modelnode.h>
#include <qml.h>
#include <propertyeditorvalue.h>

QT_BEGIN_NAMESPACE
class QtColorButton;
class QToolButton;
QT_END_NAMESPACE

namespace QmlDesigner {

class ColorButton : public QToolButton {

Q_OBJECT

Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)

public:

    ColorButton(QWidget *parent = 0) : QToolButton (parent), m_colorString("#ffffff")
    {
    }

    void setColor(const QString &colorStr)
    {
        if (m_colorString == colorStr)
            return;

        m_colorString = colorStr;
        update();
        emit colorChanged();
    }

    QString color() const
    {
        return m_colorString;
    }

signals:
    void colorChanged();

protected:
    void paintEvent(QPaintEvent *event);
private:
    QString m_colorString;
};

class ColorBox : public QWidget
{
Q_OBJECT

Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
Q_PROPERTY(int hue READ hue WRITE setHue NOTIFY hueChanged)
Q_PROPERTY(int saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

public:

ColorBox(QWidget *parent = 0) : QWidget(parent), m_color(Qt::white), m_saturatedColor(Qt::white), m_lastHue(0)
{
    setFixedWidth(130);
    setFixedHeight(130);
}

void setHue(int newHue)
{
    if (m_color.hsvHue() == newHue)
        return;

    m_color.setHsv(newHue,m_color.hsvSaturation(),m_color.value());
    update();
    emit hueChanged();
    emit colorChanged();
}

int hue() const
{
    int retval = m_color.hsvHue();
    if (retval<0) retval=0;
    if (retval>359) retval=359;
    return retval;
}

void setColor(const QString &colorStr)
{
    if (m_color.name() == colorStr)
        return;

    setColor(QColor(colorStr));
}

void setColor(const QColor &color)
{
    if (m_color == color)
        return;

    int oldsaturation = m_color.hsvSaturation();
    int oldvalue = m_color.value();
    int oldhue = m_color.hsvHue();
    m_color=color;
    update();
    if (oldhue != m_color.hsvHue()) emit hueChanged();
    if (oldsaturation != saturation()) emit saturationChanged();
    if (oldvalue != value()) emit valueChanged();
    emit colorChanged();
}

QString color() const
{
    return m_color.name();
}


int saturation() const
{
    return m_color.hsvSaturation();
}

void setSaturation(int newsaturation)
{
    if (m_color.hsvSaturation()==newsaturation) return;
    m_color.setHsv(m_color.hsvHue(),newsaturation,m_color.value());
    update();
    emit saturationChanged();
    emit colorChanged();
}

int value() const
{
    return m_color.value();
}

void setValue(int newvalue)
{
    if (m_color.value()==newvalue) return;
    m_color.setHsv(m_color.hsvHue(),m_color.hsvSaturation(),newvalue);
    update();
    emit valueChanged();
    emit colorChanged();
}

signals:
    void colorChanged();
    void hueChanged();
    void saturationChanged();
    void valueChanged();

protected:
    void paintEvent(QPaintEvent *event);

    void mousePressEvent(QMouseEvent *e)
    {
        // The current cell marker is set to the cell the mouse is pressed in
        QPoint pos = e->pos();
        m_mousePressed = true;
        setCurrent(pos.x() - 5, pos.y() - 5);
    }

    void mouseReleaseEvent(QMouseEvent * /* event */)
    {
        m_mousePressed = false;
    }

void mouseMoveEvent(QMouseEvent *e)
{
    if (!m_mousePressed)
        return;
    QPoint pos = e->pos();
    setCurrent(pos.x() - 5, pos.y() - 5);
}

void setCurrent(int x, int y);


private:
    QColor m_color;
    QColor m_saturatedColor;
    bool m_mousePressed;
    int m_lastHue;
    QPixmap m_cache;
};

class HueControl : public QWidget
{
Q_OBJECT

Q_PROPERTY(qreal hue READ hue WRITE setHue NOTIFY hueChanged)

public:

HueControl(QWidget *parent = 0) : QWidget(parent), m_color(Qt::white), m_mousePressed(false)
{
    setFixedWidth(40);
    setFixedHeight(130);
}

void setHue(int newHue)
{
    if (m_color.hsvHue() == newHue)
        return;
    m_color.setHsv(newHue, m_color.hsvSaturation(), m_color.value());
    update();
    emit hueChanged();
}

int hue() const
{
    return m_color.hsvHue();
}

signals:
    void hueChanged();

protected:
    void paintEvent(QPaintEvent *event);

    void mousePressEvent(QMouseEvent *e)
    {
        // The current cell marker is set to the cell the mouse is pressed in
        QPoint pos = e->pos();
        m_mousePressed = true;
        setCurrent(pos.y() - 5);
    }

    void mouseReleaseEvent(QMouseEvent * /* event */)
    {
        m_mousePressed = false;
    }

void mouseMoveEvent(QMouseEvent *e)
{
    if (!m_mousePressed)
        return;
    QPoint pos = e->pos();
    setCurrent(pos.y() - 5);
}

void setCurrent(int y);


private:
    QColor m_color;
    bool m_mousePressed;
    QPixmap m_cache;
};


} //QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::ColorButton);
QML_DECLARE_TYPE(QmlDesigner::HueControl);
QML_DECLARE_TYPE(QmlDesigner::ColorBox);

#endif //COLORWIDGET_H

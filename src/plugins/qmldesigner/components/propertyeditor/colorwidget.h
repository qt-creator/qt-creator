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

#ifndef COLORWIDGET_H
#define COLORWIDGET_H

#include <QWeakPointer>
#include <QtGui/QWidget>
#include <QLabel>
#include <QToolButton>
#include <QMouseEvent>
#include <modelnode.h>
#include <qdeclarative.h>
#include <propertyeditorvalue.h>
#include <qmlitemnode.h>

QT_BEGIN_NAMESPACE
class QtColorButton;
class QToolButton;
QT_END_NAMESPACE

namespace QmlDesigner {

class ColorButton : public QToolButton {

Q_OBJECT

Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
Q_PROPERTY(bool noColor READ noColor WRITE setNoColor)


public:

    ColorButton(QWidget *parent = 0) : QToolButton (parent), m_colorString("#ffffff"), m_noColor(false)
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

    bool noColor() const { return m_noColor; }
    void setNoColor(bool f) { m_noColor = f; update(); }

signals:
    void colorChanged();

protected:
    void paintEvent(QPaintEvent *event);
private:
    QString m_colorString;
    bool m_noColor;
};

inline QString properName(const QColor &color)
{
    QString s;
    if (color.alpha() == 255)
        s.sprintf("#%02x%02x%02x", color.red(), color.green(), color.blue());
    else
        s.sprintf("#%02x%02x%02x%02x", color.alpha(), color.red(), color.green(), color.blue());
    return s;
}

inline QColor properColor(const QString &str)
{
    int lalpha = 255;
    QString lcolorStr = str;
    if (lcolorStr.at(0) == '#' && lcolorStr.length() == 9) {
        QString alphaStr = lcolorStr;
        alphaStr.truncate(3);
        lcolorStr.remove(0, 3);
        lcolorStr = "#" + lcolorStr;
        alphaStr.remove(0,1);
        bool v;
        lalpha = alphaStr.toInt(&v, 16);
        if (!v)
            lalpha = 255;
    }
    QColor lcolor(lcolorStr);
    lcolor.setAlpha(lalpha);
    return lcolor;
}

class ColorBox : public QWidget
{
Q_OBJECT

Q_PROPERTY(QString strColor READ strColor WRITE setStrColor NOTIFY colorChanged)
Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
Q_PROPERTY(int hue READ hue WRITE setHue NOTIFY hueChanged)
Q_PROPERTY(int saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
Q_PROPERTY(int alpha READ alpha WRITE setAlpha NOTIFY alphaChanged)

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

    int oldAlpha = m_color.alpha();
    m_color.setHsv(newHue,m_color.hsvSaturation(),m_color.value());
    m_color.setAlpha(oldAlpha);
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

void setAlpha(int newAlpha)
{
    if (m_color.alpha() == newAlpha)
        return;

    m_color.setAlpha(newAlpha);
    update();
    emit alphaChanged();
    emit colorChanged();
}

int alpha() const
{
    return m_color.alpha();
}

void setStrColor(const QString &colorStr)
{
    if (properName(m_color) == colorStr)
        return;

    setColor(properColor(colorStr));
}

void setColor(const QColor &color)
{
    if (m_color == color)
        return;

    int oldsaturation = m_color.hsvSaturation();
    int oldvalue = m_color.value();
    int oldhue = m_color.hsvHue();
    int oldAlpha = m_color.alpha();
    m_color=color;
    update();
    if (oldhue != m_color.hsvHue()) emit hueChanged();
    if (oldsaturation != saturation()) emit saturationChanged();
    if (oldvalue != value()) emit valueChanged();
    if (oldAlpha != alpha()) emit alphaChanged();
    emit colorChanged();
}

QString strColor() const
{
    return properName(m_color);
}

QColor color() const
{
    return m_color;
}

int saturation() const
{
    return m_color.hsvSaturation();
}

void setSaturation(int newsaturation)
{
    if (m_color.hsvSaturation()==newsaturation) return;
    int oldAlpha = m_color.alpha();
    m_color.setHsv(m_color.hsvHue(),newsaturation,m_color.value());
    m_color.setAlpha(oldAlpha);
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
    int oldAlpha = m_color.alpha();
    m_color.setHsv(m_color.hsvHue(),m_color.hsvSaturation(),newvalue);
    m_color.setAlpha(oldAlpha);
    update();
    emit valueChanged();
    emit colorChanged();
}

signals:
    void colorChanged();
    void hueChanged();
    void saturationChanged();
    void valueChanged();
    void alphaChanged();

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

class GradientLine : public QWidget {

    Q_OBJECT
    Q_PROPERTY(QColor activeColor READ activeColor WRITE setActiveColor NOTIFY activeColorChanged)
    Q_PROPERTY(QVariant itemNode READ itemNode WRITE setItemNode NOTIFY itemNodeChanged)
    Q_PROPERTY(QString gradientName READ gradientName WRITE setGradientName NOTIFY gradientNameChanged)
    Q_PROPERTY(bool active READ active WRITE setActive)

public:
    GradientLine(QWidget *parent = 0) : QWidget(parent), m_gradientName("gradient"), m_activeColor(Qt::black), m_dragActive(false), m_yOffset(0), m_create(false), m_active(false)
    {
        setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
        setFocusPolicy(Qt::StrongFocus);
        setFixedHeight(50);
        setMinimumWidth(160);
        resize(160, 50);
        m_colorList << m_activeColor << QColor(Qt::white);
        m_stops << 0 << 1;
        updateGradient();
        setCurrentIndex(0);
    }


    QVariant itemNode() const { return QVariant::fromValue(m_itemNode.modelNode()); }
    void setItemNode(const QVariant &itemNode);
    QString gradientName() const { return m_gradientName; }
    void setGradientName(const QString &newName)
    {
        if (newName == m_gradientName)
            return;
        m_gradientName = newName;
        setup();
        emit gradientNameChanged();
    }

    QColor activeColor() const { return m_activeColor; }
    void setActiveColor(const QColor &newColor)
    {
        if (newColor.name() == m_activeColor.name() && newColor.alpha() == m_activeColor.alpha())
            return;

        m_activeColor = newColor;
        m_colorList.removeAt(currentColorIndex());
        m_colorList.insert(currentColorIndex(), m_activeColor);
        updateGradient();
        update();
    }

    bool active() const { return m_active; }
    void setActive(bool a) { m_active = a; }

public slots:
    void setupGradient();

signals:
    void activeColorChanged();
    void itemNodeChanged();
    void gradientNameChanged();
protected:
    bool GradientLine::event(QEvent *event);
    void keyPressEvent(QKeyEvent * event);
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);    

private:
    void setup();
    void updateGradient();
    int currentColorIndex() const { return m_colorIndex; }
    void setCurrentIndex(int i)
    {
        if (i == m_colorIndex)
            return;
        m_colorIndex = i;
        setActiveColor(m_colorList.at(i));
        emit activeColorChanged();
        update();
    }

    QColor m_activeColor;
    QmlItemNode m_itemNode;
    QString m_gradientName;
    QList<QColor> m_colorList;
    QList<qreal> m_stops;
    int m_colorIndex;
    bool m_dragActive;
    QPoint m_dragStart;
    int m_yOffset;
    bool m_create;
    bool m_active;
};


class ColorWidget {

public:
    static void registerDeclarativeTypes();


};


} //QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::ColorButton);
QML_DECLARE_TYPE(QmlDesigner::HueControl);
QML_DECLARE_TYPE(QmlDesigner::ColorBox);

#endif //COLORWIDGET_H

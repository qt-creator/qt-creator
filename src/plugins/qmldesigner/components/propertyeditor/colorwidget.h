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
#include <QDialog>

QT_BEGIN_NAMESPACE
class QtColorButton;
class QToolButton;
class QDoubleSpinBox;
QT_END_NAMESPACE

namespace QmlDesigner {

class ColorButton : public QToolButton {

Q_OBJECT

Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
Q_PROPERTY(bool noColor READ noColor WRITE setNoColor)
Q_PROPERTY(bool showArrow READ showArrow WRITE setShowArrow)

public:
    ColorButton(QWidget *parent = 0) : QToolButton (parent), m_colorString("#ffffff"), m_noColor(false), m_showArrow(true) {}

    void setColor(const QString &colorStr);
    QString color() const { return m_colorString; }
    bool noColor() const { return m_noColor; }
    void setNoColor(bool f) { m_noColor = f; update(); }
    bool showArrow() const { return m_showArrow; }
    void setShowArrow(bool b) { m_showArrow = b; }

signals:
    void colorChanged();

protected:
    void paintEvent(QPaintEvent *event);
private:
    QString m_colorString;
    bool m_noColor;
    bool m_showArrow;
};

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
    void paintEvent(QPaintEvent *event);

    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
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
        setFixedWidth(28);
        setFixedHeight(130);
    }

    void setHue(int newHue);
    int hue() const { return m_color.hsvHue(); }

signals:
    void hueChanged(int hue);

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
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
    GradientLine(QWidget *parent = 0);

    QVariant itemNode() const { return QVariant::fromValue(m_itemNode.modelNode()); }
    void setItemNode(const QVariant &itemNode);
    QString gradientName() const { return m_gradientName; }
    void setGradientName(const QString &newName);
    QColor activeColor() const { return m_activeColor; }
    void setActiveColor(const QColor &newColor);
    bool active() const { return m_active; }
    void setActive(bool a) { m_active = a; }

public slots:
    void setupGradient();
    void deleteGradient();

signals:
    void activeColorChanged();
    void itemNodeChanged();
    void gradientNameChanged();
protected:
    bool event(QEvent *event);
    void keyPressEvent(QKeyEvent * event);
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);    

private:
    void setup();
    void updateGradient();
    int currentColorIndex() const { return m_colorIndex; }
    void setCurrentIndex(int i);

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
    bool m_dragOff;
};

class BauhausColorDialog : public QFrame {

    Q_OBJECT

    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)               

public:
    BauhausColorDialog(QWidget *parent = 0);

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


class ColorWidget {

public:
    static void registerDeclarativeTypes();


};


} //QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::ColorButton);
QML_DECLARE_TYPE(QmlDesigner::HueControl);
QML_DECLARE_TYPE(QmlDesigner::ColorBox);

#endif //COLORWIDGET_H

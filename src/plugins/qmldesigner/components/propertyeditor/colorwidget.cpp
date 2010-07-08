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

#include "colorwidget.h"
#include <modelnode.h>
#include <abstractview.h>
#include <nodeproperty.h>
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <qmlobjectnode.h>
#include <qmlitemnode.h>
#include <QGradient>
#include <metainfo.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QGradient>
#include <QPainter>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QGraphicsEffect>

static inline int clamp(int x, int lower, int upper)
{
    if (x < lower)
        x = lower;
    if (x > upper)
        x = upper;
    return x;
}

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

namespace QmlDesigner {

void ColorWidget::registerDeclarativeTypes() {
    qmlRegisterType<QmlDesigner::ColorButton>("Bauhaus",1,0,"ColorButton");
    qmlRegisterType<QmlDesigner::HueControl>("Bauhaus",1,0,"HueControl");
    qmlRegisterType<QmlDesigner::ColorBox>("Bauhaus",1,0,"ColorBox");
    qmlRegisterType<QmlDesigner::GradientLine>("Bauhaus",1,0,"GradientLine");
}

void ColorButton::setColor(const QString &colorStr)
{
    if (m_colorString == colorStr)
        return;

    m_colorString = colorStr;
    update();
    emit colorChanged();
}

void ColorButton::paintEvent(QPaintEvent *event)
{
    QToolButton::paintEvent(event);
    if (!isEnabled())
        return;

    QColor color(m_colorString);

    QPainter p(this);


    QRect r(0, 0, width() - 2, height() - 2);
    if (isEnabled())
        p.setBrush(color);
    else
        p.setBrush(Qt::transparent);

    if (color.value() > 80)
        p.setPen(QColor(0x444444));
    else
        p.setPen(QColor(0x9e9e9e));
    p.drawRect(r.translated(1, 1));   

    if (m_showArrow) {
        p.setRenderHint(QPainter::Antialiasing, true);
        QVector<QPointF> points;
        if (isChecked()) {
            points.append(QPointF(2, 3));
            points.append(QPointF(8, 3));
            points.append(QPointF(5, 9));
        } else {
            points.append(QPointF(8, 6));
            points.append(QPointF(2, 9));
            points.append(QPointF(2, 3));
        }
        p.translate(0.5, 0.5);
        p.setBrush(QColor(0xaaaaaa));
        p.setPen(QColor(0x444444));
        p.drawPolygon(points);
    }
}


void HueControl::setCurrent(int y)
{
    y = clamp(y, 0, 120); 
    int oldAlpha = m_color.alpha();
    m_color.setHsv((y * 359)/120, m_color.hsvSaturation(), m_color.value());
    m_color.setAlpha(oldAlpha);
    update(); // redraw pointer
    emit hueChanged(m_color.hsvHue());
}

void HueControl::setHue(int newHue)
{
    if (m_color.hsvHue() == newHue)
        return;
    m_color.setHsv(newHue, m_color.hsvSaturation(), m_color.value());
    update();
    emit hueChanged(m_color.hsvHue());
}

void HueControl::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter p(this);

    int localHeight = 120;

    if (m_cache.isNull()) {
        m_cache = QPixmap(10, localHeight);

        QPainter cacheP(&m_cache);

        for (int i = 0; i < localHeight; i++)
        {
            QColor c;
            c.setHsv( (i*359) / 120.0, 255,255);
            cacheP.fillRect(0, i, 10, i + 1, c);
        }
    }

    //p.setPen(QColor(0x404040));
    //p.drawRect(QRect(1, 1, width() - 1, height() - 1).adjusted(10, 5, -20, -5));

    p.drawPixmap(0, 5, m_cache);

    QVector<QPointF> points;

    int y = m_color.hueF() * 120 + 5;

    points.append(QPointF(5, y));
    points.append(QPointF(15, y + 5));
    points.append(QPointF(15, y - 5));


    p.setRenderHint(QPainter::Antialiasing, true);
    p.translate(0.5, 1.5);
    p.setPen(QColor(0, 0, 0, 120));
    p.drawPolygon(points);
    p.translate(0, -1);
    p.setPen(0x222222);
    p.setBrush(QColor(0x707070));
    p.drawPolygon(points);
}

void HueControl::mousePressEvent(QMouseEvent *e)
{
    // The current cell marker is set to the cell the mouse is pressed in
    QPoint pos = e->pos();
    m_mousePressed = true;
    setCurrent(pos.y() - 5);
}

void HueControl::mouseReleaseEvent(QMouseEvent * /* event */)
{
    m_mousePressed = false;
}

void HueControl::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_mousePressed)
        return;
    QPoint pos = e->pos();
    setCurrent(pos.y() - 5);
}

void ColorBox::setHue(int newHue)
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

int ColorBox::hue() const
{
    int retval = m_color.hsvHue();
    if (retval<0) retval=0;
    if (retval>359) retval=359;
    return retval;
}

void ColorBox::setAlpha(int newAlpha)
{
    if (m_color.alpha() == newAlpha)
        return;

    m_color.setAlpha(newAlpha);
    update();
    emit alphaChanged();
    emit colorChanged();
}

QString ColorBox::strColor() const
{
    return properName(m_color);
}

void ColorBox::setStrColor(const QString &colorStr)
{
    if (properName(m_color) == colorStr)
        return;

    setColor(properColor(colorStr));
}

void ColorBox::setColor(const QColor &color)
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
}

void ColorBox::setSaturation(int newsaturation)
{
    if (m_color.hsvSaturation()==newsaturation) return;
    int oldAlpha = m_color.alpha();
    m_color.setHsv(m_color.hsvHue(),newsaturation,m_color.value());
    m_color.setAlpha(oldAlpha);
    update();
    emit saturationChanged();
    emit colorChanged();
}

void ColorBox::setCurrent(int x, int y)
{
    QColor newColor;
    x = clamp(x, 0, 120);
    y = clamp(y, 0, 120);
    int oldAlpha = m_color.alpha();
    newColor.setHsv(hue(), (x*255) / 120, 255 - (y*255) / 120);
    newColor.setAlpha(oldAlpha);
    setColor(newColor);
}

void ColorBox::setValue(int newvalue)
{
    if (m_color.value()==newvalue) return;
    int oldAlpha = m_color.alpha();
    m_color.setHsv(m_color.hsvHue(),m_color.hsvSaturation(),newvalue);
    m_color.setAlpha(oldAlpha);
    update();
    emit valueChanged();
    emit colorChanged();
}

void ColorBox::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter p(this);

    if ((m_color.saturation()>1) && (m_color.value()>1))
        m_saturatedColor.setHsv(m_color.hsvHue(),255,255);

    if ((hue() != m_lastHue) || (m_cache.isNull())) {
        m_lastHue = hue();

        int fixedHue = clamp(m_lastHue, 0, 359);

        m_cache = QPixmap(120, 120);

        int height = 120;
        int width = 120;

        QPainter chacheP(&m_cache);

        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
            {
                QColor c;
                c.setHsv(fixedHue, (x*255) / width, 255 - (y*255) / height);
                chacheP.setPen(c);
                chacheP.drawPoint(x ,y);
            }
    }
    p.drawPixmap(5, 5, m_cache);

    int x = clamp(m_color.hsvSaturationF() * 120, 0, 119) + 5;
    int y = clamp(120 - m_color.valueF() * 120, 0, 119) + 5;

    p.setPen(QColor(255, 255, 255, 50));
    p.drawLine(5, y, x-1, y);
    p.drawLine(x+1, y, width()-7, y);
    p.drawLine(x, 5, x, y-1);
    p.drawLine(x, y+1, x, height()-7);

}

void ColorBox::mousePressEvent(QMouseEvent *e)
{
    // The current cell marker is set to the cell the mouse is pressed in
    QPoint pos = e->pos();
    m_mousePressed = true;
    setCurrent(pos.x() - 5, pos.y() - 5);
}

void ColorBox::mouseReleaseEvent(QMouseEvent * /* event */)
{
    if (m_mousePressed)
        emit colorChanged();
    m_mousePressed = false;
}

void ColorBox::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_mousePressed)
        return;
    QPoint pos = e->pos();
    setCurrent(pos.x() - 5, pos.y() - 5);
}

void GradientLine::setItemNode(const QVariant &itemNode)
{

    if (!itemNode.value<ModelNode>().isValid())
        return;
    m_itemNode = itemNode.value<ModelNode>();
    setup();
    emit itemNodeChanged();
}

static inline QColor invertColor(const QColor color)
{
    QColor c = color.toHsv();
    c.setHsv(c.hue(), c.saturation(), 255 - c.value());
    return c;
}

GradientLine::GradientLine(QWidget *parent) :
        QWidget(parent),
        m_activeColor(Qt::black),
        m_gradientName("gradient"),
        m_colorIndex(0),
        m_dragActive(false),
        m_yOffset(0),
        m_create(false),
        m_active(false),
        m_dragOff(false)
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

void GradientLine::setGradientName(const QString &newName)
{
    if (newName == m_gradientName)
        return;
    m_gradientName = newName;
    setup();
    emit gradientNameChanged();
}

void GradientLine::setActiveColor(const QColor &newColor)
{
    if (newColor.name() == m_activeColor.name() && newColor.alpha() == m_activeColor.alpha())
        return;

    m_activeColor = newColor;
    m_colorList.removeAt(currentColorIndex());
    m_colorList.insert(currentColorIndex(), m_activeColor);
    updateGradient();
    update();
}

void GradientLine::setupGradient()
{
    ModelNode modelNode = m_itemNode.modelNode();
    if (!modelNode.isValid())
        return;
    m_colorList.clear();
    m_stops.clear();

    if (modelNode.hasProperty(m_gradientName)) { //gradient exists

        ModelNode gradientNode = modelNode.nodeProperty(m_gradientName).modelNode();
        QList<ModelNode> stopList = gradientNode.nodeListProperty("stops").toModelNodeList();

        foreach (const ModelNode &stopNode, stopList) {
            QmlObjectNode stopObjectNode = stopNode;
            if (stopObjectNode.isValid()) {
                m_stops << stopObjectNode.instanceValue("position").toReal();
                m_colorList << stopObjectNode.instanceValue("color").value<QColor>();
            }
        }
    } else {
        m_colorList << m_activeColor << QColor(Qt::black);
        m_stops << 0 << 1;
    }

    updateGradient();
}

void GradientLine::deleteGradient()
{
    if (!m_itemNode.isValid())
        return;

    if (!m_itemNode.modelNode().metaInfo().hasProperty(m_gradientName))
        return;

    ModelNode modelNode = m_itemNode.modelNode();

    if (m_itemNode.isInBaseState()) {
        if (modelNode.hasProperty(m_gradientName)) {
            RewriterTransaction transaction = m_itemNode.modelNode().view()->beginRewriterTransaction();
            ModelNode gradientNode = modelNode.nodeProperty(m_gradientName).modelNode();
            if (QmlObjectNode(gradientNode).isValid())
                QmlObjectNode(gradientNode).destroy();
        }
    }
}

bool GradientLine::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride)
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Delete)) {
            event->accept();
            return true;
    }

    return QWidget::event(event);
}

void GradientLine::keyPressEvent(QKeyEvent * event)
{
    if (event->matches(QKeySequence::Delete)) {
        if ((currentColorIndex()) != 0 && (currentColorIndex() < m_stops.size() - 1)) {
            m_dragActive = false;
            m_stops.removeAt(currentColorIndex());
            m_colorList.removeAt(currentColorIndex());
            updateGradient();
            setCurrentIndex(0);
            //delete item
        }
    } else {
       QWidget::keyPressEvent(event);
    }
}

void GradientLine::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter p(this);


    QLinearGradient linearGradient(QPointF(0, 0), QPointF(width(), 0));

    for (int i =0; i < m_stops.size(); i++)
         linearGradient.setColorAt(m_stops.at(i), m_colorList.at(i));

    p.setBrush(Qt::NoBrush);
    p.setPen(QColor(0x444444));
    p.drawRect(9, 31, width() - 14, height() - 32);

    p.setBrush(linearGradient);
    p.setPen(QColor(0x222222));
    p.drawRect(8, 30, width() - 14, height() - 32);
    p.setPen(QColor(255, 255, 255, 40));
    p.drawRect(9, 31, width() - 16, height() - 34);

    p.setPen(Qt::black);

    for (int i =0; i < m_colorList.size(); i++) {
        int localYOffset = 0;
        QColor arrowColor(Qt::black);
        if (i == currentColorIndex()) {
            localYOffset = m_yOffset;
            arrowColor = QColor(0xeeeeee);
        }
        p.setPen(arrowColor);
        if (i == 0 || i == (m_colorList.size() - 1))
            localYOffset = 0;

        int pos = qreal((width() - 16)) * m_stops.at(i) + 9;
        p.setBrush(arrowColor);
        QVector<QPointF> points;
        points.append(QPointF(pos + 0.5, 28.5 + localYOffset)); //triangle
        points.append(QPointF(pos - 3.5, 22.5 + localYOffset));
        points.append(QPointF(pos + 4.5, 22.5 + localYOffset));
        p.setRenderHint(QPainter::Antialiasing, true);
        p.drawPolygon(points);
        p.setRenderHint(QPainter::Antialiasing, false);
        p.setBrush(Qt::NoBrush);
        p.setPen(QColor(0x424242));
        p.drawRect(pos - 4, 9 + localYOffset, 10, 11);
        p.setPen(QColor(0x141414));
        p.setBrush(m_colorList.at(i));
        p.drawRect(pos - 5, 8 + localYOffset, 10, 11);
        p.setBrush(Qt::NoBrush);
        p.setPen(QColor(255, 255, 255, 30));
        p.drawRect(pos - 4, 9 + localYOffset, 8, 9);
    }
}

void GradientLine::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        int xPos = event->pos().x();
        int yPos = event->pos().y();

        int draggedIndex = -1;
        m_create = false;
        m_dragActive = false;
        if ((yPos > 10) && (yPos < 30))
            for (int i =0; i < m_stops.size(); i++) {
            int pos = qreal((width() - 16)) * m_stops.at(i) + 9;
            if (((xPos + 5) > pos) && ((xPos - 5) < pos)) {
                draggedIndex = i;
                m_dragActive = true;
                m_dragStart = event->pos();
                setCurrentIndex(draggedIndex);
                update();
            }
        }
        if (draggedIndex == -1)
            m_create = true;
    }
    setFocus(Qt::MouseFocusReason);
    event->accept();
}

void GradientLine::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_dragActive == false && m_create) {
            qreal stopPos = qreal(event->pos().x() - 9) / qreal((width() - 15));
            int index = -1;
            for (int i =0; i < m_stops.size() - 1; i++) {
                if ((stopPos > m_stops.at(i)) && (index == -1))
                    index = i +1;
            }
            if (index != -1 && m_itemNode.isInBaseState()) { //creating of items only in base state
                m_stops.insert(index, stopPos);
                m_colorList.insert(index, QColor(Qt::white));
                setCurrentIndex(index);
                updateGradient();
            }
        }
    }
    m_dragActive = false;
    m_yOffset = 0;
    update();
    updateGradient();
    event->accept();
}

void GradientLine::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragActive) {
        int xPos = event->pos().x();
        int pos = qreal((width() - 20)) * m_stops.at(currentColorIndex()) + 8;
        int offset = m_dragOff ? 2 : 20;
        if (xPos < pos + offset && xPos > pos - offset) {
            m_dragOff = false;
            int xDistance = event->pos().x() - m_dragStart.x();
            qreal distance = qreal(xDistance) / qreal((width() - 20));
            qreal newStop  = m_stops.at(currentColorIndex()) + distance;
            if ((newStop >=0) && (newStop <= 1))
                m_stops[currentColorIndex()] = newStop;
            m_yOffset += event->pos().y() - m_dragStart.y();
            if (m_yOffset > 0 || !m_itemNode.isInBaseState()) { //deleting only in base state
                m_yOffset = 0;
            } else if ((m_yOffset < - 12) && (currentColorIndex()) != 0 && (currentColorIndex() < m_stops.size() - 1)) {
                m_yOffset = 0;
                m_dragActive = false;
                m_stops.removeAt(currentColorIndex());
                m_colorList.removeAt(currentColorIndex());
                updateGradient();
                setCurrentIndex(0);
                //delete item
            }
        } else {
            m_dragOff = true;
        }
        m_dragStart = event->pos();
        update();
    }

    QWidget::mouseMoveEvent(event);
}

void GradientLine::setup()
{

}

static inline QColor normalizeColor(const QColor &color)
{
    QColor newColor = QColor(color.name());
    newColor.setAlpha(color.alpha());
    return newColor;
}

static inline qreal roundReal(qreal real)
{
    int i = real * 100;
    return qreal(i) / 100;
}

void GradientLine::updateGradient()
{
    if (!active())
        return;
    RewriterTransaction transaction = m_itemNode.modelNode().view()->beginRewriterTransaction();
    if (!m_itemNode.isValid())
        return;

    if (!m_itemNode.modelNode().metaInfo().hasProperty(m_gradientName))
        return;

    ModelNode modelNode = m_itemNode.modelNode();

    if (m_itemNode.isInBaseState()) {
        if (modelNode.hasProperty(m_gradientName)) {
            modelNode.removeProperty(m_gradientName);
        }

        ModelNode gradientNode = modelNode.view()->createModelNode("Qt/Gradient", 4, 7);


        for (int i = 0;i < m_stops.size(); i++) {
            ModelNode gradientStopNode = modelNode.view()->createModelNode("Qt/GradientStop", 4, 7);
            gradientStopNode.variantProperty("position") = roundReal(m_stops.at(i));
            gradientStopNode.variantProperty("color") = normalizeColor(m_colorList.at(i));
            gradientNode.nodeListProperty("stops").reparentHere(gradientStopNode);
        }
        modelNode.nodeProperty(m_gradientName).reparentHere(gradientNode);
    } else { //state
        if  (!modelNode.hasProperty(m_gradientName)) {
            qWarning(" GradientLine::updateGradient: no gradient in state");
            return;
        }
        ModelNode gradientNode = modelNode.nodeProperty(m_gradientName).modelNode();
        QList<ModelNode> stopList = gradientNode.nodeListProperty("stops").toModelNodeList();
        for (int i = 0;i < m_stops.size(); i++) {            
            QmlObjectNode stopObjectNode = stopList.at(i);
            stopObjectNode.setVariantProperty("position", roundReal(m_stops.at(i)));
            stopObjectNode.setVariantProperty("color", normalizeColor(m_colorList.at(i)));
        }
    }
}

void GradientLine::setCurrentIndex(int i)
{
    if (i == m_colorIndex)
        return;
    m_colorIndex = i;
    setActiveColor(m_colorList.at(i));
    emit activeColorChanged();
    update();
}

BauhausColorDialog::BauhausColorDialog(QWidget *parent) : QFrame(parent )
{

    setFrameStyle(QFrame::NoFrame);
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Sunken);

    QGraphicsDropShadowEffect *dropShadowEffect = new QGraphicsDropShadowEffect;
    dropShadowEffect->setBlurRadius(6);
    dropShadowEffect->setOffset(2, 2);
    setGraphicsEffect(dropShadowEffect);
    setAutoFillBackground(true);

    m_hueControl = new HueControl(this);
    m_colorBox = new ColorBox(this);

    QWidget *colorFrameWidget = new QWidget(this);
    QVBoxLayout* vBox = new QVBoxLayout(colorFrameWidget);
    colorFrameWidget->setLayout(vBox);
    vBox->setSpacing(0);
    vBox->setMargin(0);
    vBox->setContentsMargins(0,5,0,28);

    m_beforeColorWidget = new QFrame(colorFrameWidget);
    m_beforeColorWidget->setFixedSize(30, 18);
    m_beforeColorWidget->setAutoFillBackground(true);
    //m_beforeColorWidget->setFrameShape(QFrame::StyledPanel);
    m_currentColorWidget = new QFrame(colorFrameWidget);
    m_currentColorWidget->setFixedSize(30, 18);
    m_currentColorWidget->setAutoFillBackground(true);
    //m_currentColorWidget->setFrameShape(QFrame::StyledPanel);

    vBox->addWidget(m_beforeColorWidget);
    vBox->addWidget(m_currentColorWidget);


    m_rSpinBox = new QDoubleSpinBox(this);
    m_gSpinBox = new QDoubleSpinBox(this);
    m_bSpinBox = new QDoubleSpinBox(this);
    m_alphaSpinBox = new QDoubleSpinBox(this);

    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(4);
    gridLayout->setVerticalSpacing(4);
    gridLayout->setMargin(4);
    setLayout(gridLayout);

    gridLayout->addWidget(m_colorBox, 0, 0, 4, 1);
    gridLayout->addWidget(m_hueControl, 0, 1, 4, 1);

    gridLayout->addWidget(colorFrameWidget, 0, 2, 2, 1);


    gridLayout->addWidget(new QLabel("R", this), 0, 3, 1, 1);
    gridLayout->addWidget(new QLabel("G", this), 1, 3, 1, 1);
    gridLayout->addWidget(new QLabel("B", this), 2, 3, 1, 1);
    gridLayout->addWidget(new QLabel("A", this), 3, 3, 1, 1);

    gridLayout->addWidget(m_rSpinBox, 0, 4, 1, 1);
    gridLayout->addWidget(m_gSpinBox, 1, 4, 1, 1);
    gridLayout->addWidget(m_bSpinBox, 2, 4, 1, 1);
    gridLayout->addWidget(m_alphaSpinBox, 3, 4, 1, 1);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);

    QPushButton *cancelButton = buttonBox->addButton(QDialogButtonBox::Cancel);
    QPushButton *applyButton = buttonBox->addButton(QDialogButtonBox::Apply);

    gridLayout->addWidget(buttonBox, 4, 0, 1, 2);

    resize(sizeHint());

    connect(m_colorBox, SIGNAL(colorChanged()), this, SLOT(onColorBoxChanged()));
    connect(m_alphaSpinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxChanged()));
    connect(m_rSpinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxChanged()));
    connect(m_gSpinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxChanged()));
    connect(m_bSpinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxChanged()));
    connect(m_hueControl, SIGNAL(hueChanged(int)), this, SLOT(onHueChanged(int)));

    connect(applyButton, SIGNAL(pressed()), this, SLOT(onAccept()));
    connect(cancelButton, SIGNAL(pressed()), this, SIGNAL(rejected()));

    m_alphaSpinBox->setMaximum(1);
    m_rSpinBox->setMaximum(1);
    m_gSpinBox->setMaximum(1);
    m_bSpinBox->setMaximum(1);
    m_alphaSpinBox->setSingleStep(0.1);
    m_rSpinBox->setSingleStep(0.1);
    m_gSpinBox->setSingleStep(0.1);
    m_bSpinBox->setSingleStep(0.1);

    m_blockUpdate = false;

}

void BauhausColorDialog::setupColor(const QColor &color)
{
    QPalette pal = m_beforeColorWidget->palette();
    pal.setColor(QPalette::Background, color);
    m_beforeColorWidget->setPalette(pal);
    setColor(color);
}

void BauhausColorDialog::spinBoxChanged()
{
    if (m_blockUpdate)
        return;
    QColor newColor;
    newColor.setAlphaF(m_alphaSpinBox->value());
    newColor.setRedF(m_rSpinBox->value());
    newColor.setGreenF(m_gSpinBox->value());
    newColor.setBlueF(m_bSpinBox->value());
    setColor(newColor);
}

void BauhausColorDialog::onColorBoxChanged()
{
    if (m_blockUpdate)
        return;

    setColor(m_colorBox->color());
}

void BauhausColorDialog::setupWidgets()
{
    m_blockUpdate = true;
    m_hueControl->setHue(m_color.hsvHue());
    m_alphaSpinBox->setValue(m_color.alphaF());
    m_rSpinBox->setValue(m_color.redF());
    m_gSpinBox->setValue(m_color.greenF());
    m_bSpinBox->setValue(m_color.blueF());
    m_colorBox->setColor(m_color);
    QPalette pal = m_currentColorWidget->palette();
    pal.setColor(QPalette::Background, m_color);
    m_currentColorWidget->setPalette(pal);
    m_blockUpdate = false;
}

}

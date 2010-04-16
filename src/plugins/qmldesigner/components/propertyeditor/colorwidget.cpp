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
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QGradient>
#include <QPainter>
#include <QtDebug>
#include <modelnode.h>
#include <abstractview.h>
#include <nodeproperty.h>
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <qmlobjectnode.h>
#include <qmlitemnode.h>
#include <QGradient>
#include <metainfo.h>

namespace QmlDesigner {

    void ColorWidget::registerDeclarativeTypes() {
        qmlRegisterType<QmlDesigner::ColorButton>("Bauhaus",1,0,"ColorButton");
        qmlRegisterType<QmlDesigner::HueControl>("Bauhaus",1,0,"HueControl");
        qmlRegisterType<QmlDesigner::ColorBox>("Bauhaus",1,0,"ColorBox");
        qmlRegisterType<QmlDesigner::GradientLine>("Bauhaus",1,0,"GradientLine");
    }

    void ColorButton::paintEvent(QPaintEvent *event)
    {
        QToolButton::paintEvent(event);
        if (!isEnabled())
            return;

        QColor color(m_colorString);

        QPainter p(this);

        QRect r(0, 0, width(), height());
        if (isEnabled())
            p.setBrush(color);
        else
            p.setBrush(Qt::transparent);
        p.setPen(Qt::black);

        if (!m_noColor) {
            p.drawRect(r);
        } else {
            p.fillRect(r, Qt::white);
            p.fillRect(0, 0, width() /2, height() /2, QColor(Qt::gray));
            p.fillRect(width() /2, height() /2, width() /2, height() /2, QColor(Qt::gray));
            p.setBrush(Qt::transparent);
            p.drawRect(r);
        }


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
        p.setPen("#707070");
        p.setBrush(Qt::white);
        p.drawPolygon(points);
    }


    void HueControl::setCurrent(int y)
    {
        if (y<0) y=0;
        if (y>120) y=120;
        int oldAlpha = m_color.alpha();
        m_color.setHsv((y * 359)/120, m_color.hsvSaturation(), m_color.value());
        m_color.setAlpha(oldAlpha);
        update(); // redraw pointer
        emit hueChanged();
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

        p.drawPixmap(10, 5, m_cache);

        QVector<QPointF> points;

        int y = m_color.hueF() * 120 + 5;

        points.append(QPointF(15, y));
        points.append(QPointF(25, y + 5));
        points.append(QPointF(25, y - 5));

        p.setPen(Qt::black);
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRect(0, 0, width() - 1, height() - 1).adjusted(10, 5, -20, -5));

        p.setPen(Qt::black);
        p.setBrush(QColor("#707070"));
        p.drawPolygon(points);
    }

    void ColorBox::setCurrent(int x, int y)
    {
        QColor newColor;
        if (x<0) x=0;
        if (x>120) x=120;
        if (y<0) y=0;
        if (y>120) y=120;
        int oldAlpha = m_color.alpha();
        newColor.setHsv(hue(), (x*255) / 120, 255 - (y*255) / 120);
        newColor.setAlpha(oldAlpha);
        setColor(newColor);
    }

    void ColorBox::paintEvent(QPaintEvent *event)
    {
        QWidget::paintEvent(event);

        QPainter p(this);

        if ((m_color.saturation()>1) && (m_color.value()>1))
            m_saturatedColor.setHsv(m_color.hsvHue(),255,255);

        if ((hue() != m_lastHue) || (m_cache.isNull())) {
            m_lastHue = hue();

            int fixedHue = m_lastHue;

            if (fixedHue<0) fixedHue=0;
            if (fixedHue>359) fixedHue=359;

            m_cache = QPixmap(120, 120);

            int height = 120;
            int width = 120;

            QPainter chacheP(&m_cache);

            for (int y = 0; y < height; y++)
                for (int x = 0; x < width; x++)
                {
                QColor c;
                c.setHsv(fixedHue, (x*255) / 120, 255 - (y*255) / 120);
                chacheP.setPen(c);
                chacheP.drawPoint(x ,y);
            }
        }

        p.drawPixmap(5, 5, m_cache);

        int x = m_color.hsvSaturationF() * 120 + 5;
        int y = 120 - m_color.valueF() * 120 + 5;

        if (x<5) x=5;
        if (x>125) x=125;
        if (y<5) y=5;
        if (y>125) y=125;

        p.setPen(Qt::white);
        p.drawEllipse(x - 2, y - 2, 4, 4);

        p.setPen(Qt::black);
        p.drawRect(QRect(0, 0, width() - 1, height() -1).adjusted(4, 4, -4, -4));
    }

void GradientLine::setItemNode(const QVariant &itemNode)
{

    if (!itemNode.value<ModelNode>().isValid() || !QmlItemNode(itemNode.value<ModelNode>()).hasNodeParent())
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

void GradientLine::setupGradient()
{
    ModelNode modelNode = m_itemNode.modelNode();
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

    QPen pen(Qt::black);
    pen.setWidth(1);
    p.setPen(pen);

    QLinearGradient linearGradient(QPointF(2, 0), QPointF(width() -2, 0));

    for (int i =0; i < m_stops.size(); i++)
         linearGradient.setColorAt(m_stops.at(i), m_colorList.at(i));

    p.setBrush(linearGradient);
    p.drawRoundedRect(8, 30, width() - 16, height() - 32, 5, 5);

    for (int i =0; i < m_colorList.size(); i++) {
        int localYOffset = 0;
        QColor arrowColor(Qt::black);
        if (i == currentColorIndex()) {
            localYOffset = m_yOffset;
            arrowColor = QColor("#cdcdcd");
        }
        p.setPen(arrowColor);
        if (i == 0 || i == (m_colorList.size() - 1))
            localYOffset = 0;

        int pos = qreal((width() - 20)) * m_stops.at(i) + 10;
        p.setBrush(arrowColor);
        QVector<QPointF> points;
        points.append(QPointF(pos, 28 + localYOffset)); //triangle
        points.append(QPointF(pos - 4, 22 + localYOffset));
        points.append(QPointF(pos + 4, 22 + localYOffset));
        p.drawPolygon(points);

        if (i == currentColorIndex())
            p.fillRect(pos - 6, 9 + localYOffset, 13, 13, invertColor(m_colorList.at(i)));

        p.fillRect(pos - 4, 11 + localYOffset, 9, 9, m_colorList.at(i));
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
            int pos = qreal((width() - 20)) * m_stops.at(i) + 10;
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
    //QWidget::mousePressEvent(event);
}

void GradientLine::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_dragActive == false && m_create) {
            qreal stopPos = qreal(event->pos().x() - 10) / qreal((width() - 20));
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
    //QWidget::mouseReleaseEvent(event);
}

void GradientLine::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragActive) {
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

        int xPos = event->pos().x();
        int pos = qreal((width() - 20)) * m_stops.at(currentColorIndex()) + 10;
        if (!(((xPos + 5) > pos) && ((xPos - 5) < pos))) { //still on top of the item?
            m_dragActive = false; //abort drag
            m_yOffset = 0;
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
    RewriterTransaction transaction;
    if (!m_itemNode.isValid())
        return;

    if (!m_itemNode.modelNode().metaInfo().hasProperty(m_gradientName))
        return;

    ModelNode modelNode = m_itemNode.modelNode();

    if (m_itemNode.isInBaseState()) {
        if (modelNode.hasProperty(m_gradientName))
            modelNode.removeProperty(m_gradientName);

        ModelNode gradientNode = modelNode.view()->createModelNode("Qt/Gradient", 4, 6);


        for (int i = 0;i < m_stops.size(); i++) {
            ModelNode gradientStopNode = modelNode.view()->createModelNode("Qt/GradientStop", 4, 6);
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

}

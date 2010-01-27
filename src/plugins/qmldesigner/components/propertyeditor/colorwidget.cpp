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

#include "colorwidget.h"
#include "qtcolorline.h"
#include "qtcolorbutton.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QGradient>
#include <qtgradientdialog.h>
#include <QtDebug>
#include <modelnode.h>
#include <abstractview.h>
#include <nodeproperty.h>
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <qmlobjectnode.h>

QML_DEFINE_TYPE(Bauhaus,1,0,ColorWidget,QmlDesigner::ColorWidget);

namespace QmlDesigner {

ColorWidget::ColorWidget(QWidget *parent) :
        QWidget(parent),
        m_label(new QLabel(this)),
        m_colorButton(new QtColorButton(this)),
        m_gradientButton(new QToolButton(this))
{
    QHBoxLayout *horizonalBoxLayout = new QHBoxLayout(this);

    m_colorButton->setFixedHeight(32);
    m_colorButton->setStyleSheet("");

    m_gradientButton->setIcon(QIcon(":/images/gradient.png"));
    
    m_gradientButton->setFixedHeight(32);
    m_gradientButton->setFixedWidth(100);
    m_gradientButton->setStyleSheet("");

    QFont f = m_label->font();
    f.setBold(true);
    m_label->setFont(f);
    m_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    horizonalBoxLayout->addWidget(m_label);
    horizonalBoxLayout->addWidget(m_colorButton);
    horizonalBoxLayout->addWidget(m_gradientButton);
    horizonalBoxLayout->setSpacing(20);
    horizonalBoxLayout->setMargin(0);
    m_gradientButton->setCheckable(true);
    m_gradientButton->setVisible(false);
    connect(m_colorButton, SIGNAL(colorChanged(QColor)), this, SLOT(setColor(QColor)));
    connect(m_gradientButton, SIGNAL(toggled(bool)), SLOT(openGradientEditor(bool)));
}

ColorWidget::~ColorWidget()
{
}

QString ColorWidget::text() const
{
    if (m_label)
        return m_label->text();
    else
        return QString();
}

QColor ColorWidget::color() const
{
    return m_color;
}

void ColorWidget::setColor(const QColor &color)
{
    if (m_color == color)
        return;
    bool valid = m_color.isValid();
    m_color = color;
    m_colorButton->setColor(color);
    if (valid)
        emit colorChanged(color);
}

QString ColorWidget::strColor() const
{
    return m_color.name();
}

void ColorWidget::setText(const QString &text)
{
    if (m_label)
        m_label->setText(text);
}

ModelNode ColorWidget::gradientNode() const
{
    return m_gradientNode;
}

QGradient ColorWidget::gradient() const
{
    if (!m_gradientNode.isValid()) {
        return QGradient();
    }

    if (m_gradientNode.type() != "Qt/Gradient") {
        qWarning() << "ColorWidget: gradient node is of wrong type";
        return QGradient();
    }

    QLinearGradient gradient;

    QGradientStops oldStops;

    //QVariantList stopList(m_modelState.nodeState(m_gradientNode).property("stops").value().toList()); /###
    foreach (const ModelNode &stopNode, m_gradientNode.nodeListProperty("stops").toModelNodeList()) {
            oldStops.append(QPair<qreal, QColor>(stopNode.variantProperty("position").value().toReal(), stopNode.variantProperty("color").value().value<QColor>()));
        }
    gradient.setStops(oldStops);

    return gradient;
}

void ColorWidget::setGradientNode(const ModelNode &gradient)
{
    m_gradientNode = gradient;
}

bool ColorWidget::gradientButtonShown() const
{
    return m_gradientButton->isVisible();
}

void ColorWidget::setShowGradientButton(bool showButton)
{
    m_gradientButton->setVisible(showButton);
    if (showButton && layout()->children().contains(m_gradientButton))
        layout()->removeWidget(m_gradientButton);
    else if (showButton && !layout()->children().contains(m_gradientButton) )
        layout()->addWidget(m_gradientButton);
}

void ColorWidget::openGradientEditor(bool show)
{
    if (show && m_gradientDialog.isNull()) {
        m_gradientDialog = new QtGradientDialog(this);
        m_gradientDialog->setAttribute(Qt::WA_DeleteOnClose);
        m_gradientDialog->setGradient(gradient());
        m_gradientDialog->show();
        connect(m_gradientDialog.data(), SIGNAL(accepted()), SLOT(updateGradientNode()));
        connect(m_gradientDialog.data(), SIGNAL(destroyed()), SLOT(resetGradientButton()));
    } else {
        delete m_gradientDialog.data();
    }
}

void ColorWidget::updateGradientNode()
{
    QGradient gradient(m_gradientDialog->gradient());

    ModelNode gradientNode;

    if (m_modelNode.hasNodeProperty("gradient"))
        m_gradientNode = m_modelNode.nodeProperty("gradient").modelNode();

    if (m_gradientNode.isValid() && m_gradientNode.type() == "Qt/Gradient") {
        gradientNode = m_gradientNode;
    } else {
        if (m_modelNode.hasProperty("gradient")) {
            m_modelNode.removeProperty("gradient");
        }
        gradientNode = m_modelNode.addChildNode("Qt/Gradient", 4, 6, "gradient");
        m_gradientNode = gradientNode;
    }

    if (QmlObjectNode(m_gradientNode).isInBaseState()) {

        NodeListProperty stops(gradientNode.nodeListProperty("stops"));

        foreach (ModelNode node, stops.allSubNodes())
            node.destroy();

        if (gradientNode.hasProperty("stops"))
            gradientNode.removeProperty("stops");

        foreach(const QGradientStop &stop, gradient.stops()) {
            QList<QPair<QString, QVariant> > propertyList;

            qreal p = stop.first * 100;
            int ip = qRound(p);
            p = qreal(ip) / 100;

            ModelNode gradientStop(gradientNode.view()->createModelNode("Qt/GradientStop", 4, 6));
            stops.reparentHere(gradientStop);
            gradientStop.variantProperty("position").setValue(p);
            gradientStop.variantProperty("color").setValue(stop.second.name());
        }
    } else { //not in base state
        //## TODO
    }
}

void ColorWidget::resetGradientButton()
{
    m_gradientButton->setChecked(false);
}

void ColorWidget::setModelNode(const ModelNode &modelNode)
{
    if (m_modelNode != modelNode) {
        m_modelNode = modelNode;
        emit modelNodeChanged();
    }
}

ModelNode ColorWidget::modelNode() const
{
    return m_modelNode;
}

PropertyEditorNodeWrapper* ColorWidget::complexGradientNode() const
{
    return m_complexGradientNode;
}

void ColorWidget::setComplexGradientNode(PropertyEditorNodeWrapper* complexNode)
{
    m_complexGradientNode = complexNode;
    setModelNode(complexNode->parentModelNode());
}

}

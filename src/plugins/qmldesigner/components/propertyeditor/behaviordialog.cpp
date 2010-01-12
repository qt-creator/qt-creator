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

#include "behaviordialog.h"

#include <abstractview.h>
#include <nodeproperty.h>
#include <variantproperty.h>
#include <bindingproperty.h>
#include <nodeproperty.h>

#include <QLineEdit>
#include <QSpinBox>


QML_DEFINE_TYPE(Bauhaus,1,0,BehaviorWidget,QmlDesigner::BehaviorWidget);

namespace QmlDesigner {


BehaviorWidget::BehaviorWidget() : QPushButton(), m_BehaviorDialog(new BehaviorDialog(0))
{
    setCheckable(true);
    connect(this, SIGNAL(toggled(bool)), this, SLOT(buttonPressed(bool)));

}
BehaviorWidget::BehaviorWidget(QWidget *parent) : QPushButton(parent), m_BehaviorDialog(new BehaviorDialog(0))
{
    setCheckable(true);
    connect(this, SIGNAL(toggled(bool)), this, SLOT(buttonPressed(bool)));
}

PropertyEditorNodeWrapper* BehaviorWidget::complexNode() const
{
    return m_complexNode;
}

void BehaviorWidget::setComplexNode(PropertyEditorNodeWrapper* complexNode)
{
    m_complexNode = complexNode;
    m_propertyName = complexNode->propertyName();
    m_modelNode = complexNode->parentModelNode();

    if (!modelNode().isValid()) {
        m_BehaviorDialog->hide();
    }

    m_BehaviorDialog->setup(modelNode(), propertyName());
}

void BehaviorWidget::buttonPressed(bool show)
{
    if (show) {
        if (m_BehaviorDialog->isVisible()) {
            m_BehaviorDialog->reject();
        } else {
            m_BehaviorDialog->setup(modelNode(), propertyName());
            m_BehaviorDialog->show();
            setChecked(false);
        }
    }
}

BehaviorDialog::BehaviorDialog(QWidget *parent) : QDialog(parent), m_ui(new Ui_BehaviorDialog)
{
    m_ui->setupUi(this);
    setModal(true);
}

 void BehaviorDialog::setup(const ModelNode &node, const QString propertyName)
{
        m_modelNode = node;
        m_ui->duration->setValue(100);
        m_ui->velocity->setValue(2);
        m_ui->spring->setValue(2);
        m_ui->damping->setValue(2);
        m_ui->stackedWidget->setCurrentIndex(0);
        m_ui->curve->setCurrentIndex(0);

        if (m_modelNode.isValid()) {
            m_propertyName = propertyName;
            m_ui->id->setText(m_modelNode.id());
            m_ui->type->setText(m_modelNode.simplifiedTypeName());
            m_ui->propertyName->setText(propertyName);
            if (m_modelNode.hasProperty(m_propertyName) && m_modelNode.property(m_propertyName).isNodeProperty()) {
                NodeProperty nodeProperty(m_modelNode.nodeProperty(m_propertyName));
                if (nodeProperty.modelNode().type() == "Qt/SpringFollow") {
                    ModelNode springFollow = nodeProperty.modelNode();
                    m_ui->curve->setCurrentIndex(1);
                    m_ui->stackedWidget->setCurrentIndex(1);
                    if (springFollow.hasProperty("velocity") && springFollow.property("velocity").isVariantProperty())
                         m_ui->velocity->setValue(springFollow.variantProperty("velocity").value().toDouble());
                    if (springFollow.hasProperty("spring") && springFollow.property("spring").isVariantProperty())
                         m_ui->spring->setValue(springFollow.variantProperty("spring").value().toDouble());
                    if (springFollow.hasProperty("damping") && springFollow.property("damping").isVariantProperty())
                         m_ui->damping->setValue(springFollow.variantProperty("damping").value().toDouble());
                    if (springFollow.hasProperty("source") && springFollow.property("source").isVariantProperty())
                         m_ui->source->setText(springFollow.variantProperty("source").value().toString());
                } else if (nodeProperty.modelNode().type() == "Qt/Behavior") {
                    if (nodeProperty.modelNode().hasProperty("animation") &&
                        nodeProperty.modelNode().property("animation").isNodeProperty() &&
                        nodeProperty.modelNode().nodeProperty("animation").modelNode().type() == "Qt/NumberAnimation") {
                            m_ui->curve->setCurrentIndex(0);
                            ModelNode animation =  nodeProperty.modelNode().nodeProperty("animation").modelNode();
                            if (animation.hasProperty("duration") && animation.property("duration").isVariantProperty())
                                m_ui->duration->setValue(animation.variantProperty("duration").value().toInt());
                            if (animation.hasProperty("easing") && animation.property("easing").isVariantProperty()) {
                                QStringList easingItems;
                                for (int i = 0; i < m_ui->curve->count(); i++)
                                    easingItems.append(m_ui->curve->itemText(i));
                                m_ui->curve->setCurrentIndex(easingItems.indexOf(animation.variantProperty("easing").value().toString()));
                            }
                    }
                }
            }
        }
}

void BehaviorDialog::accept()
{
    QDialog::accept();
    if (m_modelNode.hasProperty(m_propertyName))
        m_modelNode.removeProperty(m_propertyName);
    if (m_ui->comboBox->currentIndex() == 0) {
        RewriterTransaction transaction(m_modelNode.view()->beginRewriterTransaction());
        ModelNode Behavior = m_modelNode.view()->createModelNode("Qt/Behavior", 4, 6);
        m_modelNode.nodeProperty(m_propertyName).reparentHere(Behavior);
        ModelNode animation = m_modelNode.view()->createModelNode("Qt/NumberAnimation", 4, 6);
        animation.variantProperty("duration") = m_ui->duration->value();
        animation.variantProperty("easing") = m_ui->curve->currentText();
        Behavior.nodeProperty("animation").reparentHere(animation);
    } else {
        RewriterTransaction transaction(m_modelNode.view()->beginRewriterTransaction());
        ModelNode springFollow = m_modelNode.view()->createModelNode("Qt/SpringFollow", 4, 6);
        m_modelNode.nodeProperty(m_propertyName).reparentHere(springFollow);
        springFollow.variantProperty("velocity") = m_ui->velocity->value();
        springFollow.variantProperty("spring") = m_ui->spring->value();
        springFollow.variantProperty("damping") = m_ui->damping->value();
        springFollow.bindingProperty("source") = m_ui->source->text();
    }
}

void BehaviorDialog::reject()
{
    QDialog::reject();
}

};

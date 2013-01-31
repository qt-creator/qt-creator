/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "behaviordialog.h"

#include <abstractview.h>
#include <nodeproperty.h>
#include <variantproperty.h>
#include <bindingproperty.h>
#include <nodeproperty.h>

#include <QLineEdit>
#include <QSpinBox>


namespace QmlDesigner {

void BehaviorDialog::registerDeclarativeType()
{
    qmlRegisterType<QmlDesigner::BehaviorWidget>("Bauhaus",1,0,"BehaviorWidget");
}

BehaviorWidget::BehaviorWidget(QWidget *parent) : QPushButton(parent), m_BehaviorDialog(new BehaviorDialog)
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

    if (!modelNode().isValid())
        m_BehaviorDialog->hide();

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

BehaviorDialog::BehaviorDialog(QWidget *parent) : QDialog(parent), m_ui(new Internal::Ui::BehaviorDialog)
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
        ModelNode Behavior = m_modelNode.view()->createModelNode("Qt/Behavior", 4, 7);
        m_modelNode.nodeProperty(m_propertyName).reparentHere(Behavior);
        ModelNode animation = m_modelNode.view()->createModelNode("Qt/NumberAnimation", 4, 7);
        animation.variantProperty("duration") = m_ui->duration->value();
        animation.variantProperty("easing") = m_ui->curve->currentText();
        Behavior.nodeProperty("animation").reparentHere(animation);
    } else {
        RewriterTransaction transaction(m_modelNode.view()->beginRewriterTransaction());
        ModelNode springFollow = m_modelNode.view()->createModelNode("Qt/SpringFollow", 4, 7);
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

}

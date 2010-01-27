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

#include "propertyeditorvalue.h"
#include <abstractview.h>
#include <nodeabstractproperty.h>
#include <nodeproperty.h>
#include <model.h>
#include <nodemetainfo.h>
#include <metainfo.h>
#include <propertymetainfo.h>
#include <nodeproperty.h>
#include <qmlobjectnode.h>

QML_DEFINE_TYPE(Bauhaus,1,0,PropertyEditorValue,PropertyEditorValue)
QML_DEFINE_TYPE(Bauhaus,1,0,PropertyEditorNodeWrapper,PropertyEditorNodeWrapper)
QML_DEFINE_TYPE(Bauhaus,1,0,QmlPropertyMap,QmlPropertyMap)


//using namespace QmlDesigner;

PropertyEditorValue::PropertyEditorValue(QObject *parent)
   : QObject(parent),
   m_isInSubState(false),
   m_isInModel(false),
   m_isBound(false),
   m_isValid(false),
   m_complexNode(new PropertyEditorNodeWrapper(this))
{
}

QVariant PropertyEditorValue::value() const
{
    QVariant returnValue = m_value;
    if (modelNode().isValid() && modelNode().metaInfo().isValid() && modelNode().metaInfo().property(name()).isValid())
        if (modelNode().metaInfo().property(name()).type() == QLatin1String("QUrl")) {
        returnValue = returnValue.toUrl().toString();
    }
    return returnValue;
}

static bool cleverDoubleCompare(QVariant value1, QVariant value2)
{ //we ignore slight changes on doubles
    if ((value1.type() == QVariant::Double) && (value2.type() == QVariant::Double)) {
        int a = value1.toDouble() * 100;
        int b = value2.toDouble() * 100;

        if (qFuzzyCompare((qreal(a) / 100), (qreal(b) / 100))) {
            return true;
        }
    }
    return false;
}

void PropertyEditorValue::setValueWithEmit(const QVariant &value)
{
    if ( m_value != value) {
        QVariant newValue = value;
        if (modelNode().metaInfo().isValid() && modelNode().metaInfo().property(name()).isValid())
            if (modelNode().metaInfo().property(name()).type() == QLatin1String("QUrl")) {
            newValue = QUrl(newValue.toString());
        }

        if (cleverDoubleCompare(newValue, m_value))
            return;
        setValue(newValue);
        m_isBound = false;
        emit valueChanged(name());
        emit isBoundChanged();
    }
}

void PropertyEditorValue::setValue(const QVariant &value)
{
    if ( m_value != value) {
        m_value = value;
        emit valueChanged(QString());
    }
    emit isBoundChanged();
}

QString PropertyEditorValue::expression() const
{

    return m_expression;
}

void PropertyEditorValue::setExpressionWithEmit(const QString &expression)
{
    if ( m_expression != expression) {
        setExpression(expression);
        m_isBound = true;
        emit expressionChanged(name());
    }
}

void PropertyEditorValue::setExpression(const QString &expression)
{
    if ( m_expression != expression) {
        m_expression = expression;
        emit expressionChanged(QString());
    }
}

bool PropertyEditorValue::isInSubState() const
{
    return m_isInSubState;
}

bool PropertyEditorValue::isBound() const
{
    return modelNode().isValid() && modelNode().property(name()).isValid() && modelNode().property(name()).isBindingProperty();
}

void PropertyEditorValue::setIsInSubState(bool isInSubState)
{
    m_isInSubState = isInSubState;
}

bool PropertyEditorValue::isInModel() const
{
    return m_isInModel;
}

void PropertyEditorValue::setIsInModel(bool isInModel)
{
    m_isInModel = isInModel;
}


QString PropertyEditorValue::name() const
{
    return m_name;
}

void PropertyEditorValue::setName(const QString &name)
{
    m_name = name;
}


bool PropertyEditorValue::isValid() const
{
    return m_isValid;
}

void PropertyEditorValue::setIsValid(bool valid)
{
    m_isValid = valid;
}

ModelNode PropertyEditorValue::modelNode() const
{
    return m_modelNode;
}

void PropertyEditorValue::setModelNode(const ModelNode &modelNode)
{
    if (modelNode != m_modelNode) {
        m_modelNode = modelNode;
        m_complexNode->update();
        emit modelNodeChanged();
    }
}

PropertyEditorNodeWrapper* PropertyEditorValue::complexNode()
{
    return m_complexNode;
}

void PropertyEditorValue::resetValue()
{
    if (m_value.isValid()) {
        setValue(QVariant());
        m_isBound = false;
        emit valueChanged(name());
    }
}

PropertyEditorNodeWrapper::PropertyEditorNodeWrapper(PropertyEditorValue* parent) : m_valuesPropertyMap(this)
{
    m_editorValue = parent;
    connect(m_editorValue, SIGNAL(modelNodeChanged()), this, SLOT(update()));
}

PropertyEditorNodeWrapper::PropertyEditorNodeWrapper(QObject *parent) : QObject(parent)
{
}

bool PropertyEditorNodeWrapper::exists()
{
    if (!(m_editorValue && m_editorValue->modelNode().isValid()))
        return false;

    return m_modelNode.isValid();
}

QString PropertyEditorNodeWrapper::type()
{
    if (!(m_modelNode.isValid()))
        return QString("");

    return m_modelNode.simplifiedTypeName();

}

ModelNode PropertyEditorNodeWrapper::parentModelNode() const
{
    return  m_editorValue->modelNode();
}

QString PropertyEditorNodeWrapper::propertyName() const
{
    return m_editorValue->name();
}

QmlPropertyMap* PropertyEditorNodeWrapper::properties()
{
    return &m_valuesPropertyMap;
}

void PropertyEditorNodeWrapper::add(const QString &type)
{

    QString propertyType = type;

    if ((m_editorValue && m_editorValue->modelNode().isValid())) {
        if (propertyType == "")
            propertyType = m_editorValue->modelNode().metaInfo().property(m_editorValue->name()).type();
        while (propertyType.contains('*')) //strip star
            propertyType.chop(1);
        m_modelNode = m_editorValue->modelNode().view()->createModelNode(propertyType, 4, 6);
        m_editorValue->modelNode().nodeAbstractProperty(m_editorValue->name()).reparentHere(m_modelNode);
        if (!m_modelNode.isValid()) {
            qWarning("PropertyEditorNodeWrapper::add failed");
        }
    } else {
        qWarning("PropertyEditorNodeWrapper::add failed - node invalid");
    }
    setup();
}

void PropertyEditorNodeWrapper::remove()
{
    if ((m_editorValue && m_editorValue->modelNode().isValid())) {
        if (QmlDesigner::QmlObjectNode(m_modelNode).isValid())
            QmlDesigner::QmlObjectNode(m_modelNode).destroy();
        m_editorValue->modelNode().removeProperty(m_editorValue->name());
    } else {
        qWarning("PropertyEditorNodeWrapper::remove failed - node invalid");
    }
    m_modelNode = QmlDesigner::ModelNode();

    foreach (const QString &propertyName, m_valuesPropertyMap.keys())
        m_valuesPropertyMap.clear(propertyName);
    foreach (QObject *object, m_valuesPropertyMap.children())
        delete object;
    emit propertiesChanged();
    emit existsChanged();
}

void PropertyEditorNodeWrapper::changeValue(const QString &name)
{
    if (name.isNull())
        return;
    if (m_modelNode.isValid()) {
        QmlDesigner::QmlObjectNode fxObjectNode(m_modelNode);

        PropertyEditorValue *valueObject = qvariant_cast<PropertyEditorValue *>(m_valuesPropertyMap.value(name));

        if (valueObject->value().isValid())
            fxObjectNode.setVariantProperty(name, valueObject->value());
        else
            fxObjectNode.removeVariantProperty(name);
    }
}

void PropertyEditorNodeWrapper::setup()
{
    Q_ASSERT(m_editorValue);
    Q_ASSERT(m_editorValue->modelNode().isValid());
    if ((m_editorValue->modelNode().isValid() && m_modelNode.isValid())) {
        QmlDesigner::QmlObjectNode fxObjectNode(m_modelNode);
        foreach ( const QString &propertyName, m_valuesPropertyMap.keys())
            m_valuesPropertyMap.clear(propertyName);
        foreach (QObject *object, m_valuesPropertyMap.children())
            delete object;

        foreach (const QString &propertyName, m_modelNode.metaInfo().properties().keys()) {
            PropertyEditorValue *valueObject = new PropertyEditorValue(&m_valuesPropertyMap);
            valueObject->setName(propertyName);
            valueObject->setIsInModel(fxObjectNode.hasProperty(propertyName));
            valueObject->setIsInSubState(fxObjectNode.propertyAffectedByCurrentState(propertyName));
            valueObject->setModelNode(fxObjectNode.modelNode());
            valueObject->setValue(fxObjectNode.instanceValue(propertyName));

            connect(valueObject, SIGNAL(valueChanged(QString)), &m_valuesPropertyMap, SIGNAL(valueChanged(QString)));
            m_valuesPropertyMap.insert(propertyName, QmlMetaType::qmlType(valueObject->metaObject())->fromObject(valueObject));
        }
    }
    connect(&m_valuesPropertyMap, SIGNAL(valueChanged(const QString &)), this, SLOT(changeValue(const QString&)));

    emit propertiesChanged();
    emit existsChanged();
}

void PropertyEditorNodeWrapper::update()
{
    if (m_editorValue && m_editorValue->modelNode().isValid()) {
        if (m_editorValue->modelNode().hasProperty(m_editorValue->name()) && m_editorValue->modelNode().property(m_editorValue->name()).isNodeProperty()) {
            m_modelNode = m_editorValue->modelNode().nodeProperty(m_editorValue->name()).modelNode();
        }
        setup();
        emit existsChanged();
        emit typeChanged();
    }
}

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

#include "propertyeditorvalue.h"
#include <QRegExp>
#include <QUrl>
#include <abstractview.h>
#include <nodeabstractproperty.h>
#include <nodeproperty.h>
#include <model.h>
#include <nodemetainfo.h>
#include <metainfo.h>
#include <nodeproperty.h>
#include <qmlobjectnode.h>

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
    if (modelNode().isValid() && modelNode().metaInfo().isValid() && modelNode().metaInfo().hasProperty(name()))
        if (modelNode().metaInfo().propertyTypeName(name()) == QLatin1String("QUrl"))
        returnValue = returnValue.toUrl().toString();
    return returnValue;
}

static bool cleverDoubleCompare(QVariant value1, QVariant value2)
{ //we ignore slight changes on doubles
    if ((value1.type() == QVariant::Double) && (value2.type() == QVariant::Double)) {
        int a = value1.toDouble() * 100;
        int b = value2.toDouble() * 100;

        if (qFuzzyCompare((qreal(a) / 100), (qreal(b) / 100)))
            return true;
    }
    return false;
}

static bool cleverColorCompare(QVariant value1, QVariant value2)
{
    if ((value1.type() == QVariant::Color) && (value2.type() == QVariant::Color)) {
        QColor c1 = value1.value<QColor>();
        QColor c2 = value2.value<QColor>();
        QString a = c1.name();
        QString b = c2.name();
        if (a != b)
            return false;
        return (c1.alpha() == c2.alpha());
    }
    if ((value1.type() == QVariant::String) && (value2.type() == QVariant::Color))
        return cleverColorCompare(QVariant(QColor(value1.toString())), value2);
    if ((value1.type() == QVariant::Color) && (value2.type() == QVariant::String))
        return cleverColorCompare(value1, QVariant(QColor(value2.toString())));
    return false;
}


/* "red" is the same color as "#ff0000"
  To simplify editing we convert all explicit color names in the hash format */
static void fixAmbigousColorNames(const QmlDesigner::ModelNode &modelNode, const QString &name, QVariant *value)
{
    if (modelNode.isValid() && modelNode.metaInfo().isValid()
            && (modelNode.metaInfo().propertyTypeName(name) == "QColor"
                || modelNode.metaInfo().propertyTypeName(name) == "color")) {
        if ((value->type() == QVariant::Color)) {
            QColor color = value->value<QColor>();
            int alpha = color.alpha();
            color = QColor(color.name());
            color.setAlpha(alpha);
            *value = color;
        } else if (value->toString() != QLatin1String("transparent")) {
            *value = QColor(value->toString()).name();
        }
    }
}

static void fixUrl(const QmlDesigner::ModelNode &modelNode, const QString &name, QVariant *value)
{
    if (modelNode.isValid() && modelNode.metaInfo().isValid()
            && (modelNode.metaInfo().propertyTypeName(name) == "QUrl"
                || modelNode.metaInfo().propertyTypeName(name) == "url")) {
        if (!value->isValid())
            *value = QString(QLatin1String(""));
    }
}

void PropertyEditorValue::setValueWithEmit(const QVariant &value)
{
    if (m_value != value || isBound()) {
        QVariant newValue = value;
        if (modelNode().isValid() && modelNode().metaInfo().isValid() && modelNode().metaInfo().hasProperty(name()))
            if (modelNode().metaInfo().propertyTypeName(name()) == QLatin1String("QUrl"))
            newValue = QUrl(newValue.toString());

        if (cleverDoubleCompare(newValue, m_value))
            return;
        if (cleverColorCompare(newValue, m_value))
            return;
        setValue(newValue);
        m_isBound = false;
        emit valueChanged(name(), value);
        emit valueChangedQml();
        emit isBoundChanged();
    }
}

void PropertyEditorValue::setValue(const QVariant &value)
{
    if ((m_value != value) &&
        !cleverDoubleCompare(value, m_value) &&
        !cleverColorCompare(value, m_value))

        m_value = value;

    fixAmbigousColorNames(modelNode(), name(), &m_value);
    fixUrl(modelNode(), name(), &m_value);

    if (m_value.isValid())
        emit valueChangedQml();
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

QString PropertyEditorValue::valueToString() const
{
    return value().toString();
}

bool PropertyEditorValue::isInSubState() const
{
    const QmlDesigner::QmlObjectNode objectNode(modelNode());
    return objectNode.isValid() && objectNode.currentState().isValid() && objectNode.propertyAffectedByCurrentState(name());
}

bool PropertyEditorValue::isBound() const
{
    const QmlDesigner::QmlObjectNode objectNode(modelNode());
    return objectNode.isValid() && objectNode.hasBindingProperty(name());
}

bool PropertyEditorValue::isInModel() const
{
    return modelNode().isValid() && modelNode().hasProperty(name());
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

bool PropertyEditorValue::isTranslated() const
{
    if (modelNode().isValid() && modelNode().metaInfo().isValid() && modelNode().metaInfo().hasProperty(name()))
        if (modelNode().metaInfo().propertyTypeName(name()) == QLatin1String("QString") || modelNode().metaInfo().propertyTypeName(name()) == QLatin1String("string")) {
            const QmlDesigner::QmlObjectNode objectNode(modelNode());
            if (objectNode.isValid() && objectNode.hasBindingProperty(name())) {
                //qsTr()
                QRegExp rx("qsTr(\"*\")");
                rx.setPatternSyntax(QRegExp::Wildcard);
                return rx.exactMatch(expression());
            }
            return false;
        }
    return false;
}

QmlDesigner::ModelNode PropertyEditorValue::modelNode() const
{
    return m_modelNode;
}

void PropertyEditorValue::setModelNode(const QmlDesigner::ModelNode &modelNode)
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
    if (m_value.isValid() || isBound()) {
        m_value = QVariant();
        m_isBound = false;
        emit valueChanged(name(), QVariant());
    }
}

void PropertyEditorValue::registerDeclarativeTypes()
{
    qmlRegisterType<PropertyEditorValue>("Bauhaus",1,0,"PropertyEditorValue");
    qmlRegisterType<PropertyEditorNodeWrapper>("Bauhaus",1,0,"PropertyEditorNodeWrapper");
    qmlRegisterType<QDeclarativePropertyMap>("Bauhaus",1,0,"QDeclarativePropertyMap");
}

PropertyEditorNodeWrapper::PropertyEditorNodeWrapper(PropertyEditorValue* parent) : QObject(parent), m_valuesPropertyMap(this)
{
    m_editorValue = parent;
    connect(m_editorValue, SIGNAL(modelNodeChanged()), this, SLOT(update()));
}

PropertyEditorNodeWrapper::PropertyEditorNodeWrapper(QObject *parent) : QObject(parent), m_editorValue(NULL)
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
        return QString();

    return m_modelNode.simplifiedTypeName();

}

QmlDesigner::ModelNode PropertyEditorNodeWrapper::parentModelNode() const
{
    return  m_editorValue->modelNode();
}

QString PropertyEditorNodeWrapper::propertyName() const
{
    return m_editorValue->name();
}

QDeclarativePropertyMap* PropertyEditorNodeWrapper::properties()
{
    return &m_valuesPropertyMap;
}

void PropertyEditorNodeWrapper::add(const QString &type)
{

    QString propertyType = type;

    if ((m_editorValue && m_editorValue->modelNode().isValid())) {
        if (propertyType.isEmpty())
            propertyType = m_editorValue->modelNode().metaInfo().propertyTypeName(m_editorValue->name());
        while (propertyType.contains('*')) //strip star
            propertyType.chop(1);
        m_modelNode = m_editorValue->modelNode().view()->createModelNode(propertyType, 4, 7);
        m_editorValue->modelNode().nodeAbstractProperty(m_editorValue->name()).reparentHere(m_modelNode);
        if (!m_modelNode.isValid())
            qWarning("PropertyEditorNodeWrapper::add failed");
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

        foreach (const QString &propertyName, m_modelNode.metaInfo().propertyNames()) {
            if (fxObjectNode.isValid()) {
                PropertyEditorValue *valueObject = new PropertyEditorValue(&m_valuesPropertyMap);
                valueObject->setName(propertyName);
                valueObject->setValue(fxObjectNode.instanceValue(propertyName));
                connect(valueObject, SIGNAL(valueChanged(QString,QVariant)), &m_valuesPropertyMap, SIGNAL(valueChanged(QString,QVariant)));
                m_valuesPropertyMap.insert(propertyName, QVariant::fromValue(valueObject));
            }
        }
    }
    connect(&m_valuesPropertyMap, SIGNAL(valueChanged(QString,QVariant)), this, SLOT(changeValue(QString)));

    emit propertiesChanged();
    emit existsChanged();
}

void PropertyEditorNodeWrapper::update()
{
    if (m_editorValue && m_editorValue->modelNode().isValid()) {
        if (m_editorValue->modelNode().hasProperty(m_editorValue->name()) && m_editorValue->modelNode().property(m_editorValue->name()).isNodeProperty())
            m_modelNode = m_editorValue->modelNode().nodeProperty(m_editorValue->name()).modelNode();
        setup();
        emit existsChanged();
        emit typeChanged();
    }
}


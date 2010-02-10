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

#include "abstractproperty.h"
#include "bindingproperty.h"
#include "filemanager/firstdefinitionfinder.h"
#include "filemanager/objectlengthcalculator.h"
#include "nodemetainfo.h"
#include "nodeproperty.h"
#include "propertymetainfo.h"
#include "texttomodelmerger.h"
#include "rewriterview.h"
#include "variantproperty.h"
#include <QmlDomDocument>
#include <QmlEngine>
#include <QSet>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

static inline bool equals(const QVariant &a, const QVariant &b)
{
    if (a.type() == QVariant::Double && b.type() == QVariant::Double)
        return qFuzzyCompare(a.toDouble(), b.toDouble());
    else
        return a == b;
}

TextToModelMerger::TextToModelMerger(RewriterView *reWriterView):
        m_rewriterView(reWriterView),
        m_isActive(false)
{
    Q_ASSERT(reWriterView);
}

void TextToModelMerger::setActive(bool active)
{
    m_isActive = active;
}

bool TextToModelMerger::isActive() const
{
    return m_isActive;
}

void TextToModelMerger::setupImports(QmlDomDocument &doc,
                                     DifferenceHandler &differenceHandler)
{
    QSet<Import> existingImports = m_rewriterView->model()->imports();

    foreach (const QmlDomImport &qmlImport, doc.imports()) {
        if (qmlImport.type() == QmlDomImport::Library) {
            Import import(Import::createLibraryImport(qmlImport.uri(),
                                                      qmlImport.version(),
                                                      qmlImport.qualifier()));

            if (!existingImports.remove(import))
                differenceHandler.modelMissesImport(import);
        } else if (qmlImport.type() == QmlDomImport::File) {
            Import import(Import:: createFileImport(qmlImport.uri(),
                                                    qmlImport.version(),
                                                    qmlImport.qualifier()));

            if (!existingImports.remove(import))
                differenceHandler.modelMissesImport(import);
        }
    }

    foreach (const Import &import, existingImports)
        differenceHandler.importAbsentInQMl(import);
}

bool TextToModelMerger::load(const QByteArray &data, DifferenceHandler &differenceHandler)
{
    setActive(true);

    try {
        QmlEngine engine;
        QmlDomDocument doc;
        const QUrl url = m_rewriterView->model()->fileUrl();
        const bool success = doc.load(&engine, data, url);

        if (success) {
            setupImports(doc, differenceHandler);

            const QmlDomObject rootDomObject = doc.rootObject();
            ModelNode modelRootNode = m_rewriterView->rootModelNode();
            syncNode(modelRootNode, rootDomObject, differenceHandler);
            m_rewriterView->positionStorage()->cleanupInvalidOffsets();
            m_rewriterView->clearErrors();
        } else {
            QList<RewriterView::Error> errors;
            foreach (const QmlError &qmlError, doc.errors())
                errors.append(RewriterView::Error(qmlError));
            m_rewriterView->setErrors(errors);
        }

        setActive(false);
        return success;
    } catch (Exception &e) {
        RewriterView::Error error(&e);
        // Somehow, the error below gets eaten in upper levels, so printing the
        // exception info here for debugging purposes:
        qDebug() << "*** An exception occurred while reading the QML file:"
                 << error.toString();
        m_rewriterView->addError(error);

        setActive(false);

        return false;
    }
}

void TextToModelMerger::syncNode(ModelNode &modelNode,
                                 const QmlDomObject &domObject,
                                 DifferenceHandler &differenceHandler)
{
    m_rewriterView->positionStorage()->setNodeOffset(modelNode, domObject.position());

    if (modelNode.type() != domObject.objectType()
            || modelNode.majorVersion() != domObject.objectTypeMajorVersion()
            || modelNode.minorVersion() != domObject.objectTypeMinorVersion()) {
        const bool isRootNode = m_rewriterView->rootModelNode() == modelNode;
        differenceHandler.typeDiffers(isRootNode, modelNode, domObject);
        if (!isRootNode)
            return; // the difference handler will create a new node, so we're done.
    }

    {
        QString domObjectId = domObject.objectId();
        const QmlDomProperty domIdProperty = domObject.property("id");
        if (domObjectId.isEmpty() && domIdProperty.value().isLiteral())
            domObjectId = domIdProperty.value().toLiteral().literal();

        if (domObjectId.isEmpty()) {
            if (!modelNode.id().isEmpty()) {
                ModelNode existingNodeWithId = m_rewriterView->modelNodeForId(domObjectId);
                if (existingNodeWithId.isValid())
                    existingNodeWithId.setId(QString());
                differenceHandler.idsDiffer(modelNode, domObjectId);
            }
        } else {
            if (modelNode.id() != domObjectId) {
                ModelNode existingNodeWithId = m_rewriterView->modelNodeForId(domObjectId);
                if (existingNodeWithId.isValid())
                    existingNodeWithId.setId(QString());
                differenceHandler.idsDiffer(modelNode, domObjectId);
            }
        }
    }

    QSet<QString> modelPropertyNames = QSet<QString>::fromList(modelNode.propertyNames());

    foreach (const QmlDomProperty &domProperty, domObject.properties()) {
        const QString domPropertyName = domProperty.propertyName();

        if (isSignalPropertyName(domPropertyName.toUtf8()))
            continue;

        if (domPropertyName == QLatin1String("id")) {
            // already done before
            continue;
        } else if (domPropertyName.isEmpty()) {
            qWarning() << "QML DOM returned an empty property name";
            continue;
        } else if (domPropertyName.at(0).isUpper() && domPropertyName.contains('.')) {
            // An attached property, which we currently don't handle.
            // So, skipping it.
            modelPropertyNames.remove(domPropertyName);
            continue;
        } else {
            const QmlDomDynamicProperty dynamicProperty = domObject.dynamicProperty(domProperty.propertyName());
            if (dynamicProperty.isValid() || modelNode.metaInfo().hasProperty(domPropertyName, true) || modelNode.type() == QLatin1String("Qt/PropertyChanges")) {
                AbstractProperty modelProperty = modelNode.property(domPropertyName);
                syncProperty(modelProperty, domProperty, dynamicProperty, differenceHandler);
                modelPropertyNames.remove(domPropertyName);
            }
        }
    }

    { // for new dynamic properties which have no property definitions:
        foreach (const QmlDomDynamicProperty &dynamicDomProperty, domObject.dynamicProperties()) {
            const QByteArray propertyName = dynamicDomProperty.propertyName();
            if (domObject.property(propertyName).isValid())
                continue;

            if (dynamicDomProperty.isAlias())
                continue; // we don't handle alias properties yet.

            AbstractProperty modelProperty = modelNode.property(propertyName);
            const QString dynamicTypeName = QMetaType::typeName(dynamicDomProperty.propertyType());

            // a dynamic property definition without a value
            if (modelProperty.isValid() && modelProperty.isVariantProperty()) {
                VariantProperty modelVariantProperty = modelProperty.toVariantProperty();
                if (modelVariantProperty.value() != QVariant())
                    differenceHandler.variantValuesDiffer(modelVariantProperty, QVariant(), dynamicTypeName);
            } else {
                differenceHandler.shouldBeVariantProperty(modelProperty, QVariant(), dynamicTypeName);
            }
        }
    }

    foreach (const QString &modelPropertyName, modelPropertyNames) {
        AbstractProperty modelProperty = modelNode.property(modelPropertyName);
        const QmlDomDynamicProperty dynamicDomProperty = domObject.dynamicProperty(modelPropertyName.toUtf8());

        if (dynamicDomProperty.isValid()) {
            const QString dynamicTypeName = QMetaType::typeName(dynamicDomProperty.propertyType());

            // a dynamic property definition without a value
            if (modelProperty.isValid() && modelProperty.isVariantProperty()) {
                VariantProperty modelVariantProperty = modelProperty.toVariantProperty();
                if (modelVariantProperty.value() != QVariant())
                    differenceHandler.variantValuesDiffer(modelVariantProperty, QVariant(), dynamicTypeName);
            } else {
                differenceHandler.shouldBeVariantProperty(modelProperty, QVariant(), dynamicTypeName);
            }
        } else {
            // property deleted.
            differenceHandler.propertyAbsentFromQml(modelProperty);
        }
    }
}

void TextToModelMerger::syncProperty(AbstractProperty &modelProperty,
                                     const QmlDomProperty &qmlProperty,
                                     const QmlDomDynamicProperty &qmlDynamicProperty,
                                     DifferenceHandler &differenceHandler)
{
    Q_ASSERT(modelProperty.name() == qmlProperty.propertyName());

    const QmlDomValue qmlValue = qmlProperty.value();

    if (qmlValue.isBinding()) {
        const QString qmlBinding = qmlValue.toBinding().binding();
        if (modelProperty.isBindingProperty()) {
            BindingProperty bindingProperty = modelProperty.toBindingProperty();
            if (bindingProperty.expression() != qmlBinding) {
                differenceHandler.bindingExpressionsDiffer(bindingProperty, qmlBinding);
            }
        } else {
            differenceHandler.shouldBeBindingProperty(modelProperty, qmlBinding);
        }
    } else if (qmlValue.isList()) {
        if (modelProperty.isNodeListProperty()) {
            NodeListProperty nodeListProperty = modelProperty.toNodeListProperty();
            syncNodeListProperty(nodeListProperty, qmlValue.toList(), differenceHandler);
        } else {
            differenceHandler.shouldBeNodeListProperty(modelProperty, qmlValue.toList());
        }
    } else if (qmlValue.isLiteral()) {
        const QVariant qmlVariantValue = convertToVariant(modelProperty.parentModelNode(), qmlProperty, qmlDynamicProperty);
        QString dynamicTypeName;
        if (qmlDynamicProperty.isValid())
            dynamicTypeName = QMetaType::typeName(qmlDynamicProperty.propertyType());

        if (modelProperty.isVariantProperty()) {
            VariantProperty modelVariantProperty = modelProperty.toVariantProperty();

            if (!equals(modelVariantProperty.value(), qmlVariantValue)
                    || qmlDynamicProperty.isValid() != modelVariantProperty.isDynamic()
                    || dynamicTypeName != modelVariantProperty.dynamicTypeName()) {
                differenceHandler.variantValuesDiffer(modelVariantProperty, qmlVariantValue, dynamicTypeName);
            }
        } else {
            differenceHandler.shouldBeVariantProperty(modelProperty, qmlVariantValue, dynamicTypeName);
        }
    } else if (qmlValue.isObject()) {
        if (modelProperty.isNodeProperty()) {
            ModelNode nodePropertyNode = modelProperty.toNodeProperty().modelNode();
            syncNode(nodePropertyNode, qmlValue.toObject(), differenceHandler);
        } else {
            differenceHandler.shouldBeNodeProperty(modelProperty, qmlValue.toObject());
        }
    } else if (qmlValue.isValueSource()) {
        if (modelProperty.isNodeProperty()) {
            ModelNode nodePropertyNode = modelProperty.toNodeProperty().modelNode();
            syncNode(nodePropertyNode, qmlValue.toValueSource().object(), differenceHandler);
        } else {
            differenceHandler.shouldBeNodeProperty(modelProperty, qmlValue.toValueSource().object());
        }
    } else if (qmlValue.isInvalid()) {
        // skip these nodes
    } else {
        qWarning() << "Found an unknown qml value!";
    }
}

void TextToModelMerger::syncNodeListProperty(NodeListProperty &modelListProperty, const QmlDomList &domList, DifferenceHandler &differenceHandler)
{
    QList<ModelNode> modelNodes = modelListProperty.toModelNodeList();
    QList<QmlDomValue> domValues = domList.values();
    int i = 0;
    for (; i < modelNodes.size() && i < domValues.size(); ++i) {
        QmlDomValue value = domValues.at(i);
        if (value.isObject()) {
            ModelNode modelNode = modelNodes.at(i);
            syncNode(modelNode, value.toObject(), differenceHandler);
        } else {
            qDebug() << "*** Oops, we got a non-object item in the list!";
        }
    }

    for (int j = i; j < domValues.size(); ++j) {
        // more elements in the dom-list, so add them to the model
        QmlDomValue value = domValues.at(j);
        if (value.isObject()) {
            const QmlDomObject qmlObject = value.toObject();
            const ModelNode newNode = differenceHandler.listPropertyMissingModelNode(modelListProperty, qmlObject);
            if (QString::fromUtf8(qmlObject.objectType()) == QLatin1String("Qt/Component"))
                setupComponent(newNode);
        } else {
            qDebug() << "*** Oops, we got a non-object item in the list!";
        }
    }

    for (int j = i; j < modelNodes.size(); ++j) {
        // more elements in the model, so remove them.
        ModelNode modelNode = modelNodes.at(j);
        differenceHandler.modelNodeAbsentFromQml(modelNode);
    }
}

ModelNode TextToModelMerger::createModelNode(const QmlDomObject &domObject, DifferenceHandler &differenceHandler)
{
    ModelNode newNode = m_rewriterView->createModelNode(domObject.objectType(), domObject.objectTypeMajorVersion(), domObject.objectTypeMinorVersion());
    syncNode(newNode, domObject, differenceHandler);
    return newNode;
}

QVariant TextToModelMerger::convertToVariant(const ModelNode &node, const QmlDomProperty &qmlProperty, const QmlDomDynamicProperty &qmlDynamicProperty)
{
    QString stringValue = qmlProperty.value().toLiteral().literal();

    if (qmlDynamicProperty.isValid()) {
        const int type = qmlDynamicProperty.propertyType();
        QVariant value(stringValue);
        value.convert(static_cast<QVariant::Type>(type));
        return value;
    }

    const NodeMetaInfo nodeMetaInfo = node.metaInfo();

    if (nodeMetaInfo.isValid()) {
        const PropertyMetaInfo propertyMetaInfo = nodeMetaInfo.property(qmlProperty.propertyName(), true);

        if (propertyMetaInfo.isValid()) {
            QVariant castedValue = propertyMetaInfo.castedValue(stringValue);
            if (!castedValue.isValid())
                qWarning() << "Casting the value" << stringValue << "of property" << propertyMetaInfo.name() << "to the property type" << propertyMetaInfo.type() << "failed";
            return castedValue;
        } else if (node.type() == QLatin1String("Qt/PropertyChanges")) {
            // In the future, we should do the type resolving in a second pass, or delay setting properties until the full file has been parsed.
            return QVariant(stringValue);
        } else {
            qWarning() << "Unknown property" << qmlProperty.propertyName() << "in node" << node.type() << "with value" << stringValue;
            return QVariant();
        }
    } else {
        qWarning() << "Unknown property" << qmlProperty.propertyName() << "in node" << node.type() << "with value" << stringValue;
        return QVariant::fromValue(stringValue);
    }
}

void ModelValidator::modelMissesImport(const Import &import)
{
    Q_ASSERT(m_merger->view()->model()->imports().contains(import));
}

void ModelValidator::importAbsentInQMl(const Import &import)
{
    Q_ASSERT(! m_merger->view()->model()->imports().contains(import));
}

void ModelValidator::bindingExpressionsDiffer(BindingProperty &modelProperty, const QString &qmlBinding)
{
    Q_ASSERT(modelProperty.expression() == qmlBinding);
    Q_ASSERT(0);
}

void ModelValidator::shouldBeBindingProperty(AbstractProperty &modelProperty, const QString &/*qmlBinding*/)
{
    Q_ASSERT(modelProperty.isBindingProperty());
    Q_ASSERT(0);
}

void ModelValidator::shouldBeNodeListProperty(AbstractProperty &modelProperty, const QmlDomList &/*domList*/)
{
    Q_ASSERT(modelProperty.isNodeListProperty());
    Q_ASSERT(0);
}

void ModelValidator::variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName)
{
    Q_ASSERT(modelProperty.isDynamic() == !dynamicTypeName.isEmpty());
    if (modelProperty.isDynamic()) {
        Q_ASSERT(modelProperty.dynamicTypeName() == dynamicTypeName);
    }

    Q_ASSERT(equals(modelProperty.value(), qmlVariantValue));
    Q_ASSERT(0);
}

void ModelValidator::shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &/*qmlVariantValue*/, const QString &/*dynamicTypeName*/)
{
    Q_ASSERT(modelProperty.isVariantProperty());
    Q_ASSERT(0);
}

void ModelValidator::shouldBeNodeProperty(AbstractProperty &modelProperty, const QmlDomObject &/*qmlObject*/)
{
    Q_ASSERT(modelProperty.isNodeProperty());
    Q_ASSERT(0);
}

void ModelValidator::modelNodeAbsentFromQml(ModelNode &modelNode)
{
    Q_ASSERT(!modelNode.isValid());
    Q_ASSERT(0);
}

ModelNode ModelValidator::listPropertyMissingModelNode(NodeListProperty &/*modelProperty*/, const QmlDomObject &/*qmlObject*/)
{
    Q_ASSERT(0);
    return ModelNode();
}

void ModelValidator::typeDiffers(bool /*isRootNode*/, ModelNode &modelNode, const QmlDomObject &domObject)
{
    Q_ASSERT(modelNode.type() == domObject.objectType());
    Q_ASSERT(modelNode.majorVersion() == domObject.objectTypeMajorVersion());
    Q_ASSERT(modelNode.minorVersion() == domObject.objectTypeMinorVersion());
    Q_ASSERT(0);
}

void ModelValidator::propertyAbsentFromQml(AbstractProperty &modelProperty)
{
    Q_ASSERT(!modelProperty.isValid());
    Q_ASSERT(0);
}

void ModelValidator::idsDiffer(ModelNode &modelNode, const QString &qmlId)
{
    Q_ASSERT(modelNode.id() == qmlId);
    Q_ASSERT(0);
}

void ModelAmender::modelMissesImport(const Import &import)
{
    m_merger->view()->model()->addImport(import);
}

void ModelAmender::importAbsentInQMl(const Import &import)
{
    m_merger->view()->model()->removeImport(import);
}

void ModelAmender::bindingExpressionsDiffer(BindingProperty &modelProperty, const QString &qmlBinding)
{
    modelProperty.toBindingProperty().setExpression(qmlBinding);
}

void ModelAmender::shouldBeBindingProperty(AbstractProperty &modelProperty, const QString &qmlBinding)
{
    ModelNode theNode = modelProperty.parentModelNode();
    BindingProperty newModelProperty = theNode.bindingProperty(modelProperty.name());
    newModelProperty.setExpression(qmlBinding);
}

void ModelAmender::shouldBeNodeListProperty(AbstractProperty &modelProperty, const QmlDomList &domList)
{
    ModelNode theNode = modelProperty.parentModelNode();
    NodeListProperty newNodeListProperty = theNode.nodeListProperty(modelProperty.name());
    m_merger->syncNodeListProperty(newNodeListProperty, domList, *this);
}

void ModelAmender::variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicType)
{
    if (dynamicType.isEmpty())
        modelProperty.setValue(qmlVariantValue);
    else
        modelProperty.setDynamicTypeNameAndValue(dynamicType, qmlVariantValue);
}

void ModelAmender::shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName)
{
//    qDebug() << "property" << modelProperty.name() << "in node" << modelProperty.parentModelNode().id();
    ModelNode theNode = modelProperty.parentModelNode();
    VariantProperty newModelProperty = theNode.variantProperty(modelProperty.name());

    if (dynamicTypeName.isEmpty())
        newModelProperty.setValue(qmlVariantValue);
    else
        newModelProperty.setDynamicTypeNameAndValue(dynamicTypeName, qmlVariantValue);
}

void ModelAmender::shouldBeNodeProperty(AbstractProperty &modelProperty, const QmlDomObject &qmlObject)
{
    ModelNode theNode = modelProperty.parentModelNode();
    NodeProperty newNodeProperty = theNode.nodeProperty(modelProperty.name());
    newNodeProperty.setModelNode(m_merger->createModelNode(qmlObject, *this));
}

void ModelAmender::modelNodeAbsentFromQml(ModelNode &modelNode)
{
    modelNode.destroy();
}

ModelNode ModelAmender::listPropertyMissingModelNode(NodeListProperty &modelProperty, const QmlDomObject &qmlObject)
{
    const ModelNode &newNode = m_merger->createModelNode(qmlObject, *this);
    modelProperty.reparentHere(newNode);
    return newNode;
}

void ModelAmender::typeDiffers(bool isRootNode, ModelNode &modelNode, const QmlDomObject &domObject)
{
    if (isRootNode) {
        modelNode.view()->changeRootNodeType(domObject.objectType(), domObject.objectTypeMajorVersion(), domObject.objectTypeMinorVersion());
    } else {
        NodeAbstractProperty parentProperty = modelNode.parentProperty();
        int nodeIndex = -1;
        if (parentProperty.isNodeListProperty()) {
            nodeIndex = parentProperty.toNodeListProperty().toModelNodeList().indexOf(modelNode);
            Q_ASSERT(nodeIndex >= 0);
        }

        modelNode.destroy();

        const ModelNode &newNode = m_merger->createModelNode(domObject, *this);
        parentProperty.reparentHere(newNode);
        if (nodeIndex >= 0) {
            int currentIndex = parentProperty.toNodeListProperty().toModelNodeList().indexOf(newNode);
            if (nodeIndex != currentIndex)
                parentProperty.toNodeListProperty().slide(currentIndex, nodeIndex);
        }
    }
}

void ModelAmender::propertyAbsentFromQml(AbstractProperty &modelProperty)
{
    modelProperty.parentModelNode().removeProperty(modelProperty.name());
}

void ModelAmender::idsDiffer(ModelNode &modelNode, const QString &qmlId)
{
    modelNode.setId(qmlId);
}

bool TextToModelMerger::isSignalPropertyName(const QString &signalName)
{
    // see QmlCompiler::isSignalPropertyName
    return signalName.length() >= 3 && signalName.startsWith(QLatin1String("on")) &&
           signalName.at(2).isLetter();
}

void TextToModelMerger::setupComponent(const ModelNode &node)
{
    Q_ASSERT(node.type() == QLatin1String("Qt/Component"));

    QString componentText = m_rewriterView->extractText(QList<ModelNode>() << node).value(node);

    if (componentText.isEmpty())
        return;

    QString result;
    if (componentText.contains("Component")) { //explicit component
        FirstDefinitionFinder firstDefinitionFinder(componentText);
        int offset = firstDefinitionFinder(0);
        ObjectLengthCalculator objectLengthCalculator(componentText);
        int length = objectLengthCalculator(offset);
        for (int i = offset;i<offset + length;i++)
            result.append(componentText.at(i));
    } else {
        result = componentText; //implicit component
    }

    node.variantProperty("__component_data") = result;
}

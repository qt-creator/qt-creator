// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "materialeditorqmlbackend.h"

#include "materialeditorcontextobject.h"
#include "materialeditorimageprovider.h"
#include "materialeditortransaction.h"
#include "propertyeditorvalue.h"
#include <qmldesignerconstants.h>
#include <qmltimeline.h>

#include <qmlobjectnode.h>
#include <nodemetainfo.h>
#include <variantproperty.h>
#include <bindingproperty.h>

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>
#include <QQuickItem>
#include <QQuickWidget>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

static QObject *variantToQObject(const QVariant &value)
{
    if (value.typeId() == QMetaType::QObjectStar || value.typeId() > QMetaType::User)
        return *(QObject **)value.constData();

    return nullptr;
}

namespace QmlDesigner {

MaterialEditorQmlBackend::MaterialEditorQmlBackend(MaterialEditorView *materialEditor)
    : m_quickWidget(Utils::makeUniqueObjectPtr<QQuickWidget>())
    , m_materialEditorTransaction(std::make_unique<MaterialEditorTransaction>(materialEditor))
    , m_contextObject(std::make_unique<MaterialEditorContextObject>(m_quickWidget.get()))
    , m_materialEditorImageProvider(new MaterialEditorImageProvider(materialEditor))
{
    m_quickWidget->setObjectName(Constants::OBJECT_NAME_MATERIAL_EDITOR);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_quickWidget->engine()->addImageProvider("materialEditor", m_materialEditorImageProvider);
    m_contextObject->setBackendValues(&m_backendValuesPropertyMap);
    m_contextObject->setModel(materialEditor->model());
    context()->setContextObject(m_contextObject.get());

    QObject::connect(&m_backendValuesPropertyMap, &DesignerPropertyMap::valueChanged,
                     materialEditor, &MaterialEditorView::changeValue);
}

MaterialEditorQmlBackend::~MaterialEditorQmlBackend()
{
}

PropertyName MaterialEditorQmlBackend::auxNamePostFix(PropertyNameView propertyName)
{
    return propertyName + "__AUX";
}

void MaterialEditorQmlBackend::createPropertyEditorValue(const QmlObjectNode &qmlObjectNode,
                                                         PropertyNameView name,
                                                         const QVariant &value,
                                                         MaterialEditorView *materialEditor)
{
    PropertyName propertyName(name.toByteArray());
    propertyName.replace('.', '_');
    auto valueObject = qobject_cast<PropertyEditorValue *>(variantToQObject(backendValuesPropertyMap().value(QString::fromUtf8(propertyName))));
    if (!valueObject) {
        valueObject = new PropertyEditorValue(&backendValuesPropertyMap());
        QObject::connect(valueObject, &PropertyEditorValue::valueChanged, &backendValuesPropertyMap(), &DesignerPropertyMap::valueChanged);
        QObject::connect(valueObject, &PropertyEditorValue::expressionChanged, materialEditor, &MaterialEditorView::changeExpression);
        QObject::connect(valueObject, &PropertyEditorValue::exportPropertyAsAliasRequested, materialEditor, &MaterialEditorView::exportPropertyAsAlias);
        QObject::connect(valueObject, &PropertyEditorValue::removeAliasExportRequested, materialEditor, &MaterialEditorView::removeAliasExport);
        backendValuesPropertyMap().insert(QString::fromUtf8(propertyName), QVariant::fromValue(valueObject));
    }
    valueObject->setName(name);
    valueObject->setModelNode(qmlObjectNode);

    if (qmlObjectNode.propertyAffectedByCurrentState(name) && !(qmlObjectNode.modelNode().property(name).isBindingProperty()))
        valueObject->setValue(qmlObjectNode.modelValue(name));
    else
        valueObject->setValue(value);

    if (propertyName != "id" && qmlObjectNode.currentState().isBaseState()
        && qmlObjectNode.modelNode().property(propertyName).isBindingProperty()) {
        valueObject->setExpression(qmlObjectNode.modelNode().bindingProperty(propertyName).expression());
    } else {
        if (qmlObjectNode.hasBindingProperty(name))
            valueObject->setExpression(qmlObjectNode.expression(name));
        else
            valueObject->setExpression(qmlObjectNode.instanceValue(name).toString());
    }
}

void MaterialEditorQmlBackend::setValue(const QmlObjectNode &,
                                        PropertyNameView name,
                                        const QVariant &value)
{
    // Vector*D values need to be split into their subcomponents
    if (value.typeId() == QMetaType::QVector2D) {
        const char *suffix[2] = {"_x", "_y"};
        auto vecValue = value.value<QVector2D>();
        for (int i = 0; i < 2; ++i) {
            PropertyName subPropName(name.size() + 2, '\0');
            subPropName.replace(0, name.size(), name);
            subPropName.replace(name.size(), 2, suffix[i]);
            auto propertyValue = qobject_cast<PropertyEditorValue *>(variantToQObject(m_backendValuesPropertyMap.value(QString::fromUtf8(subPropName))));
            if (propertyValue)
                propertyValue->setValue(QVariant(vecValue[i]));
        }
    } else if (value.typeId() == QMetaType::QVector3D) {
        const char *suffix[3] = {"_x", "_y", "_z"};
        auto vecValue = value.value<QVector3D>();
        for (int i = 0; i < 3; ++i) {
            PropertyName subPropName(name.size() + 2, '\0');
            subPropName.replace(0, name.size(), name);
            subPropName.replace(name.size(), 2, suffix[i]);
            auto propertyValue = qobject_cast<PropertyEditorValue *>(variantToQObject(m_backendValuesPropertyMap.value(QString::fromUtf8(subPropName))));
            if (propertyValue)
                propertyValue->setValue(QVariant(vecValue[i]));
        }
    } else if (value.typeId() == QMetaType::QVector4D) {
        const char *suffix[4] = {"_x", "_y", "_z", "_w"};
        auto vecValue = value.value<QVector4D>();
        for (int i = 0; i < 4; ++i) {
            PropertyName subPropName(name.size() + 2, '\0');
            subPropName.replace(0, name.size(), name);
            subPropName.replace(name.size(), 2, suffix[i]);
            auto propertyValue = qobject_cast<PropertyEditorValue *>(
                variantToQObject(m_backendValuesPropertyMap.value(QString::fromUtf8(subPropName))));
            if (propertyValue)
                propertyValue->setValue(QVariant(vecValue[i]));
        }
    } else {
        PropertyName propertyName = name.toByteArray();
        propertyName.replace('.', '_');
        auto propertyValue = qobject_cast<PropertyEditorValue *>(variantToQObject(m_backendValuesPropertyMap.value(QString::fromUtf8(propertyName))));
        if (propertyValue)
            propertyValue->setValue(value);
    }
}

void MaterialEditorQmlBackend::setExpression(PropertyNameView propName, const QString &exp)
{
    PropertyEditorValue *propertyValue = propertyValueForName(QString::fromUtf8(propName));
    if (propertyValue)
        propertyValue->setExpression(exp);
}

QQmlContext *MaterialEditorQmlBackend::context() const
{
    return m_quickWidget->rootContext();
}

MaterialEditorContextObject *MaterialEditorQmlBackend::contextObject() const
{
    return m_contextObject.get();
}

QQuickWidget *MaterialEditorQmlBackend::widget() const
{
    return m_quickWidget.get();
}

void MaterialEditorQmlBackend::setSource(const QUrl &url)
{
    m_quickWidget->setSource(url);
}

QmlAnchorBindingProxy &MaterialEditorQmlBackend::backendAnchorBinding()
{
    return m_backendAnchorBinding;
}

void MaterialEditorQmlBackend::updateMaterialPreview(const QPixmap &pixmap)
{
    m_materialEditorImageProvider->setPixmap(pixmap);
    QMetaObject::invokeMethod(m_quickWidget->rootObject(), "refreshPreview");
}

void MaterialEditorQmlBackend::refreshBackendModel()
{
    m_backendModelNode.refresh();
}

DesignerPropertyMap &MaterialEditorQmlBackend::backendValuesPropertyMap()
{
    return m_backendValuesPropertyMap;
}

MaterialEditorTransaction *MaterialEditorQmlBackend::materialEditorTransaction() const
{
    return m_materialEditorTransaction.get();
}

PropertyEditorValue *MaterialEditorQmlBackend::propertyValueForName(const QString &propertyName)
{
     return qobject_cast<PropertyEditorValue *>(variantToQObject(backendValuesPropertyMap().value(propertyName)));
}

void MaterialEditorQmlBackend::setup(const QmlObjectNode &selectedMaterialNode, const QString &stateName,
                                     const QUrl &qmlSpecificsFile, MaterialEditorView *materialEditor)
{
    if (selectedMaterialNode.isValid()) {
        m_contextObject->setModel(materialEditor->model());

        for (const auto &property : selectedMaterialNode.modelNode().metaInfo().properties()) {
            createPropertyEditorValue(selectedMaterialNode,
                                      property.name(),
                                      selectedMaterialNode.instanceValue(property.name()),
                                      materialEditor);
        }

        // model node
        m_backendModelNode.setup(selectedMaterialNode.modelNode());
        context()->setContextProperty("modelNodeBackend", &m_backendModelNode);
        context()->setContextProperty("hasMaterial", QVariant(true));

        // className
        auto valueObject = qobject_cast<PropertyEditorValue *>(variantToQObject(
            m_backendValuesPropertyMap.value(Constants::PROPERTY_EDITOR_CLASSNAME_PROPERTY)));
        if (!valueObject)
            valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
        valueObject->setName(Constants::PROPERTY_EDITOR_CLASSNAME_PROPERTY);
        valueObject->setModelNode(selectedMaterialNode.modelNode());
        valueObject->setValue(m_backendModelNode.simplifiedTypeName());
        QObject::connect(valueObject,
                         &PropertyEditorValue::valueChanged,
                         &backendValuesPropertyMap(),
                         &DesignerPropertyMap::valueChanged);
        m_backendValuesPropertyMap.insert(Constants::PROPERTY_EDITOR_CLASSNAME_PROPERTY,
                                          QVariant::fromValue(valueObject));

        // anchors
        m_backendAnchorBinding.setup(selectedMaterialNode.modelNode());
        context()->setContextProperties(QVector<QQmlContext::PropertyPair>{
            {{"anchorBackend"}, QVariant::fromValue(&m_backendAnchorBinding)},
            {{"transaction"}, QVariant::fromValue(m_materialEditorTransaction.get())}});

        contextObject()->setSpecificsUrl(qmlSpecificsFile);
        contextObject()->setStateName(stateName);

        QStringList stateNames = selectedMaterialNode.allStateNames();
        stateNames.prepend("base state");
        contextObject()->setAllStateNames(stateNames);
        contextObject()->setSelectedMaterial(selectedMaterialNode);
        contextObject()->setIsBaseState(selectedMaterialNode.isInBaseState());
        contextObject()->setHasAliasExport(selectedMaterialNode.isAliasExported());
        contextObject()->setHasActiveTimeline(QmlTimeline::hasActiveTimeline(selectedMaterialNode.view()));

        contextObject()->setSelectionChanged(false);

#ifndef QDS_USE_PROJECTSTORAGE
        NodeMetaInfo metaInfo = selectedMaterialNode.modelNode().metaInfo();
        contextObject()->setMajorVersion(metaInfo.isValid() ? metaInfo.majorVersion() : -1);
#endif
    } else {
        context()->setContextProperty("hasMaterial", QVariant(false));
    }
}

QString MaterialEditorQmlBackend::propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

void MaterialEditorQmlBackend::emitSelectionToBeChanged()
{
    m_backendModelNode.emitSelectionToBeChanged();
}

void MaterialEditorQmlBackend::emitSelectionChanged()
{
    m_backendModelNode.emitSelectionChanged();
}

void MaterialEditorQmlBackend::setValueforAuxiliaryProperties(const QmlObjectNode &qmlObjectNode,
                                                              AuxiliaryDataKeyView key)
{
    const PropertyName propertyName = auxNamePostFix(key.name.toByteArray());
    setValue(qmlObjectNode, propertyName, qmlObjectNode.modelNode().auxiliaryDataWithDefault(key));
}

} // namespace QmlDesigner

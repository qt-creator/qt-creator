// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textureeditorqmlbackend.h"

#include "assetimageprovider.h"
#include "bindingproperty.h"
#include "documentmanager.h"
#include "nodemetainfo.h"
#include "propertyeditorvalue.h"
#include "qmldesignerconstants.h"
#include "qmlobjectnode.h"
#include "qmltimeline.h"
#include "textureeditortransaction.h"
#include "textureeditorcontextobject.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hdrimage.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <QDir>
#include <QFileInfo>
#include <QQuickImageProvider>
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

TextureEditorQmlBackend::TextureEditorQmlBackend(TextureEditorView *textureEditor, AsynchronousImageCache &imageCache)
    : m_view(new QQuickWidget)
    , m_textureEditorTransaction(new TextureEditorTransaction(textureEditor))
    , m_contextObject(new TextureEditorContextObject(m_view->rootContext()))
{
    QImage defaultImage;
    defaultImage.load(Utils::StyleHelper::dpiSpecificImageFile(":/textureeditor/images/texture_default.png"));
    m_textureEditorImageProvider = new AssetImageProvider(imageCache, defaultImage);
    m_view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_view->setObjectName(Constants::OBJECT_NAME_TEXTURE_EDITOR);
    m_view->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_view->engine()->addImageProvider("qmldesigner_thumbnails", m_textureEditorImageProvider);
    m_contextObject->setBackendValues(&m_backendValuesPropertyMap);
    m_contextObject->setModel(textureEditor->model());
    context()->setContextObject(m_contextObject.data());

    QObject::connect(&m_backendValuesPropertyMap, &DesignerPropertyMap::valueChanged,
                     textureEditor, &TextureEditorView::changeValue);
}

TextureEditorQmlBackend::~TextureEditorQmlBackend()
{
}

PropertyName TextureEditorQmlBackend::auxNamePostFix(const PropertyName &propertyName)
{
    return propertyName + "__AUX";
}

void TextureEditorQmlBackend::createPropertyEditorValue(const QmlObjectNode &qmlObjectNode,
                                                         const PropertyName &name,
                                                         const QVariant &value,
                                                         TextureEditorView *textureEditor)
{
    PropertyName propertyName(name);
    propertyName.replace('.', '_');
    auto valueObject = qobject_cast<PropertyEditorValue *>(variantToQObject(backendValuesPropertyMap().value(QString::fromUtf8(propertyName))));
    if (!valueObject) {
        valueObject = new PropertyEditorValue(&backendValuesPropertyMap());
        QObject::connect(valueObject, &PropertyEditorValue::valueChanged, &backendValuesPropertyMap(), &DesignerPropertyMap::valueChanged);
        QObject::connect(valueObject, &PropertyEditorValue::expressionChanged, textureEditor, &TextureEditorView::changeExpression);
        QObject::connect(valueObject, &PropertyEditorValue::exportPropertyAsAliasRequested, textureEditor, &TextureEditorView::exportPropertyAsAlias);
        QObject::connect(valueObject, &PropertyEditorValue::removeAliasExportRequested, textureEditor, &TextureEditorView::removeAliasExport);
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

void TextureEditorQmlBackend::setValue(const QmlObjectNode &, const PropertyName &name, const QVariant &value)
{
    // Vector*D values need to be split into their subcomponents
    if (value.typeId() == QVariant::Vector2D) {
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
    } else if (value.typeId() == QVariant::Vector3D) {
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
    } else if (value.typeId() == QVariant::Vector4D) {
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
        PropertyName propertyName = name;
        propertyName.replace('.', '_');
        auto propertyValue = qobject_cast<PropertyEditorValue *>(variantToQObject(m_backendValuesPropertyMap.value(QString::fromUtf8(propertyName))));
        if (propertyValue)
            propertyValue->setValue(value);
    }
}

QQmlContext *TextureEditorQmlBackend::context() const
{
    return m_view->rootContext();
}

TextureEditorContextObject *TextureEditorQmlBackend::contextObject() const
{
    return m_contextObject.data();
}

QQuickWidget *TextureEditorQmlBackend::widget() const
{
    return m_view;
}

void TextureEditorQmlBackend::setSource(const QUrl &url)
{
    m_view->setSource(url);
}

Internal::QmlAnchorBindingProxy &TextureEditorQmlBackend::backendAnchorBinding()
{
    return m_backendAnchorBinding;
}

DesignerPropertyMap &TextureEditorQmlBackend::backendValuesPropertyMap()
{
    return m_backendValuesPropertyMap;
}

TextureEditorTransaction *TextureEditorQmlBackend::textureEditorTransaction() const
{
    return m_textureEditorTransaction.data();
}

PropertyEditorValue *TextureEditorQmlBackend::propertyValueForName(const QString &propertyName)
{
     return qobject_cast<PropertyEditorValue *>(variantToQObject(backendValuesPropertyMap().value(propertyName)));
}

void TextureEditorQmlBackend::setup(const QmlObjectNode &selectedTextureNode, const QString &stateName,
                                    const QUrl &qmlSpecificsFile, TextureEditorView *textureEditor)
{
    if (selectedTextureNode.isValid()) {
        m_contextObject->setModel(textureEditor->model());

        for (const auto &property : selectedTextureNode.modelNode().metaInfo().properties()) {
            createPropertyEditorValue(selectedTextureNode,
                                      property.name(),
                                      selectedTextureNode.instanceValue(property.name()),
                                      textureEditor);
        }

        // model node
        m_backendModelNode.setup(selectedTextureNode.modelNode());
        context()->setContextProperty("modelNodeBackend", &m_backendModelNode);
        context()->setContextProperty("hasTexture", QVariant(true));

        // className
        auto valueObject = qobject_cast<PropertyEditorValue *>(variantToQObject(
            m_backendValuesPropertyMap.value(Constants::PROPERTY_EDITOR_CLASSNAME_PROPERTY)));
        if (!valueObject)
            valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
        valueObject->setName(Constants::PROPERTY_EDITOR_CLASSNAME_PROPERTY);
        valueObject->setModelNode(selectedTextureNode.modelNode());
        valueObject->setValue(m_backendModelNode.simplifiedTypeName());
        QObject::connect(valueObject,
                         &PropertyEditorValue::valueChanged,
                         &backendValuesPropertyMap(),
                         &DesignerPropertyMap::valueChanged);
        m_backendValuesPropertyMap.insert(Constants::PROPERTY_EDITOR_CLASSNAME_PROPERTY,
                                          QVariant::fromValue(valueObject));

        // anchors
        m_backendAnchorBinding.setup(selectedTextureNode.modelNode());
        context()->setContextProperties(
            QVector<QQmlContext::PropertyPair>{
                {{"anchorBackend"}, QVariant::fromValue(&m_backendAnchorBinding)},
                {{"transaction"}, QVariant::fromValue(m_textureEditorTransaction.data())}
            }
        );

        contextObject()->setSpecificsUrl(qmlSpecificsFile);
        contextObject()->setStateName(stateName);

        QStringList stateNames = selectedTextureNode.allStateNames();
        stateNames.prepend("base state");
        contextObject()->setAllStateNames(stateNames);
        contextObject()->setSelectedMaterial(selectedTextureNode);
        contextObject()->setIsBaseState(selectedTextureNode.isInBaseState());
        contextObject()->setHasAliasExport(selectedTextureNode.isAliasExported());
        contextObject()->setHasActiveTimeline(QmlTimeline::hasActiveTimeline(selectedTextureNode.view()));

        contextObject()->setSelectionChanged(false);

        NodeMetaInfo metaInfo = selectedTextureNode.modelNode().metaInfo();
        contextObject()->setMajorVersion(metaInfo.isValid() ? metaInfo.majorVersion() : -1);
    } else {
        context()->setContextProperty("hasTexture", QVariant(false));
    }
}

QString TextureEditorQmlBackend::propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

void TextureEditorQmlBackend::emitSelectionToBeChanged()
{
    m_backendModelNode.emitSelectionToBeChanged();
}

void TextureEditorQmlBackend::emitSelectionChanged()
{
    m_backendModelNode.emitSelectionChanged();
}

void TextureEditorQmlBackend::setValueforAuxiliaryProperties(const QmlObjectNode &qmlObjectNode,
                                                             AuxiliaryDataKeyView key)
{
    const PropertyName propertyName = auxNamePostFix(PropertyName(key.name));
    setValue(qmlObjectNode, propertyName, qmlObjectNode.modelNode().auxiliaryDataWithDefault(key));
}

} // namespace QmlDesigner

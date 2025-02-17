// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "designerpropertymap.h"
#include "propertyeditorcontextobject.h"
#include "propertyeditorvalue.h"
#include "qmlanchorbindingproxy.h"
#include "qmlmodelnodeproxy.h"
#include "quick2propertyeditorview.h"

#include <utils/uniqueobjectptr.h>

#include <nodemetainfo.h>

#include <QQmlPropertyMap>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QQuickImageProvider)

class PropertyEditorValue;

namespace QmlDesigner {

class PropertyEditorTransaction;
class PropertyEditorView;

class PropertyEditorQmlBackend
{

    Q_DISABLE_COPY(PropertyEditorQmlBackend)


public:
    PropertyEditorQmlBackend(PropertyEditorView *propertyEditor,
                             class AsynchronousImageCache &imageCache);
    ~PropertyEditorQmlBackend();

    void setup(const QmlObjectNode &fxObjectNode, const QString &stateName, const QUrl &qmlSpecificsFile, PropertyEditorView *propertyEditor);
    void setValue(const QmlObjectNode &fxObjectNode, PropertyNameView name, const QVariant &value);
    void setExpression(PropertyNameView propName, const QString &exp);

    QQmlContext *context();
    PropertyEditorContextObject* contextObject();
    QQuickWidget *widget();
    void setSource(const QUrl& url);
    QmlAnchorBindingProxy &backendAnchorBinding();
    DesignerPropertyMap &backendValuesPropertyMap();
    PropertyEditorTransaction *propertyEditorTransaction();

    PropertyEditorValue *propertyValueForName(const QString &propertyName);

    static QString propertyEditorResourcesPath();
    static QString scriptsEditorResourcesPath();
    static QUrl emptyPaneUrl();
#ifndef QDS_USE_PROJECTSTORAGE
    static QString templateGeneration(const NodeMetaInfo &type,
                                      const NodeMetaInfo &superType,
                                      const QmlObjectNode &node);
    static QUrl getQmlFileUrl(const TypeName &relativeTypeName, const NodeMetaInfo &info);
    static std::tuple<QUrl, NodeMetaInfo> getQmlUrlForMetaInfo(const NodeMetaInfo &modelNode);
#endif

    static bool checkIfUrlExists(const QUrl &url);

    void emitSelectionToBeChanged();
    void emitSelectionChanged();

    void setValueforLayoutAttachedProperties(const QmlObjectNode &qmlObjectNode,
                                             PropertyNameView name);
    void setValueforInsightAttachedProperties(const QmlObjectNode &qmlObjectNode,
                                              PropertyNameView name);
    void setValueforAuxiliaryProperties(const QmlObjectNode &qmlObjectNode, AuxiliaryDataKeyView key);

    void setupLayoutAttachedProperties(const QmlObjectNode &qmlObjectNode,
                                       PropertyEditorView *propertyEditor);
    void setupInsightAttachedProperties(const QmlObjectNode &qmlObjectNode,
                                        PropertyEditorView *propertyEditor);
    void setupAuxiliaryProperties(const QmlObjectNode &qmlObjectNode,
                                  PropertyEditorView *propertyEditor);

    void handleInstancePropertyChangedInModelNodeProxy(const ModelNode &modelNode,
                                                       PropertyNameView propertyName);

    void handleVariantPropertyChangedInModelNodeProxy(const VariantProperty &property);
    void handleBindingPropertyChangedInModelNodeProxy(const BindingProperty &property);
    void handlePropertiesRemovedInModelNodeProxy(const AbstractProperty &property);
    void handleModelNodePreviewPixmapChanged(const ModelNode &node,
                                             const QPixmap &pixmap,
                                             const QByteArray &requestId);

    static NodeMetaInfo findCommonAncestor(const ModelNode &node);

    void refreshBackendModel();
    void refreshPreview();

    void setupContextProperties();

private:
    void createPropertyEditorValue(const QmlObjectNode &qmlObjectNode,
                                   PropertyNameView name,
                                   const QVariant &value,
                                   PropertyEditorView *propertyEditor);
    void setupPropertyEditorValue(PropertyNameView name,
                                  PropertyEditorView *propertyEditor,
                                  const NodeMetaInfo &type);
    void createPropertyEditorValues(const QmlObjectNode &qmlObjectNode, PropertyEditorView *propertyEditor);

    static QUrl fileToUrl(const QString &filePath);
    static QString fileFromUrl(const QUrl &url);
#ifndef QDS_USE_PROJECTSTORAGE
    static TypeName qmlFileName(const NodeMetaInfo &nodeInfo);
    static QString locateQmlFile(const NodeMetaInfo &info, const QString &relativePath);
#endif
    static TypeName fixTypeNameForPanes(const TypeName &typeName);
    static QString resourcesPath(const QString &dir);

private:
    // to avoid a crash while destructing DesignerPropertyMap in the QQmlData
    // this needs be destructed after m_quickWidget->engine() is destructed
    DesignerPropertyMap m_backendValuesPropertyMap;

    Utils::UniqueObjectPtr<Quick2PropertyEditorView> m_view = nullptr;
    QmlAnchorBindingProxy m_backendAnchorBinding;
    QmlModelNodeProxy m_backendModelNode;
    std::unique_ptr<PropertyEditorTransaction> m_propertyEditorTransaction;
    std::unique_ptr<PropertyEditorValue> m_dummyPropertyEditorValue;
    std::unique_ptr<PropertyEditorContextObject> m_contextObject;
};

} //QmlDesigner

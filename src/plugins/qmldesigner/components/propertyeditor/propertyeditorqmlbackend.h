// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlanchorbindingproxy.h"
#include "designerpropertymap.h"
#include "propertyeditorvalue.h"
#include "propertyeditorcontextobject.h"
#include "qmlmodelnodeproxy.h"
#include "quick2propertyeditorview.h"

#include <nodemetainfo.h>

#include <QQmlPropertyMap>

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
    void initialSetup(const TypeName &typeName, const QUrl &qmlSpecificsFile, PropertyEditorView *propertyEditor);
    void setValue(const QmlObjectNode &fxObjectNode, const PropertyName &name, const QVariant &value);
    void setExpression(const PropertyName &propName, const QString &exp);

    QQmlContext *context();
    PropertyEditorContextObject* contextObject();
    QQuickWidget *widget();
    void setSource(const QUrl& url);
    Internal::QmlAnchorBindingProxy &backendAnchorBinding();
    DesignerPropertyMap &backendValuesPropertyMap();
    PropertyEditorTransaction *propertyEditorTransaction();

    PropertyEditorValue *propertyValueForName(const QString &propertyName);

    static QString propertyEditorResourcesPath();
    static QString templateGeneration(const NodeMetaInfo &type, const NodeMetaInfo &superType, const QmlObjectNode &node);

    static QUrl getQmlFileUrl(const TypeName &relativeTypeName, const NodeMetaInfo &info);
    static std::tuple<QUrl, NodeMetaInfo> getQmlUrlForMetaInfo(const NodeMetaInfo &modelNode);

    static bool checkIfUrlExists(const QUrl &url);

    void emitSelectionToBeChanged();
    void emitSelectionChanged();

    void setValueforLayoutAttachedProperties(const QmlObjectNode &qmlObjectNode,
                                             const PropertyName &name);
    void setValueforInsightAttachedProperties(const QmlObjectNode &qmlObjectNode,
                                              const PropertyName &name);
    void setValueforAuxiliaryProperties(const QmlObjectNode &qmlObjectNode, AuxiliaryDataKeyView key);

    void setupLayoutAttachedProperties(const QmlObjectNode &qmlObjectNode,
                                       PropertyEditorView *propertyEditor);
    void setupInsightAttachedProperties(const QmlObjectNode &qmlObjectNode,
                                        PropertyEditorView *propertyEditor);
    void setupAuxiliaryProperties(const QmlObjectNode &qmlObjectNode, PropertyEditorView *propertyEditor);

    static NodeMetaInfo findCommonAncestor(const ModelNode &node);

private:
    void createPropertyEditorValue(const QmlObjectNode &qmlObjectNode,
                                   const PropertyName &name, const QVariant &value,
                                   PropertyEditorView *propertyEditor);
    void setupPropertyEditorValue(const PropertyName &name,
                                  PropertyEditorView *propertyEditor,
                                  const NodeMetaInfo &type);

    static TypeName qmlFileName(const NodeMetaInfo &nodeInfo);
    static QUrl fileToUrl(const QString &filePath);
    static QString fileFromUrl(const QUrl &url);
    static QString locateQmlFile(const NodeMetaInfo &info, const QString &relativePath);
    static TypeName fixTypeNameForPanes(const TypeName &typeName);

private:
    Quick2PropertyEditorView *m_view;
    Internal::QmlAnchorBindingProxy m_backendAnchorBinding;
    QmlModelNodeProxy m_backendModelNode;
    DesignerPropertyMap m_backendValuesPropertyMap;
    QScopedPointer<PropertyEditorTransaction> m_propertyEditorTransaction;
    QScopedPointer<PropertyEditorValue> m_dummyPropertyEditorValue;
    QScopedPointer<PropertyEditorContextObject> m_contextObject;
};

} //QmlDesigner

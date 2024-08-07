// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "designerpropertymap.h"
#include "qmlanchorbindingproxy.h"
#include "qmlmodelnodeproxy.h"

#include <utils/uniqueobjectptr.h>

#include <nodemetainfo.h>

#include <memory>

class PropertyEditorValue;

QT_BEGIN_NAMESPACE
class QQuickWidget;
QT_END_NAMESPACE

namespace QmlDesigner {

class AssetImageProvider;
class TextureEditorContextObject;
class TextureEditorTransaction;
class TextureEditorView;

class TextureEditorQmlBackend
{
    Q_DISABLE_COPY(TextureEditorQmlBackend)

public:
    TextureEditorQmlBackend(TextureEditorView *materialEditor,
                            class AsynchronousImageCache &imageCache);
    ~TextureEditorQmlBackend();

    void setup(const QmlObjectNode &selectedTextureNode, const QString &stateName, const QUrl &qmlSpecificsFile,
               TextureEditorView *textureEditor);
    void setValue(const QmlObjectNode &fxObjectNode, PropertyNameView name, const QVariant &value);

    QQmlContext *context() const;
    TextureEditorContextObject *contextObject() const;
    QQuickWidget *widget() const;
    void setSource(const QUrl &url);
    QmlAnchorBindingProxy &backendAnchorBinding();
    DesignerPropertyMap &backendValuesPropertyMap();
    TextureEditorTransaction *textureEditorTransaction() const;

    PropertyEditorValue *propertyValueForName(const QString &propertyName);

    static QString propertyEditorResourcesPath();

    void emitSelectionToBeChanged();
    void emitSelectionChanged();

    void setValueforAuxiliaryProperties(const QmlObjectNode &qmlObjectNode, AuxiliaryDataKeyView key);

private:
    void createPropertyEditorValue(const QmlObjectNode &qmlObjectNode,
                                   PropertyNameView name,
                                   const QVariant &value,
                                   TextureEditorView *textureEditor);
    PropertyName auxNamePostFix(PropertyNameView propertyName);

    // to avoid a crash while destructing DesignerPropertyMap in the QQmlData
    // this needs be destructed after m_quickWidget->engine() is destructed
    DesignerPropertyMap m_backendValuesPropertyMap;

    Utils::UniqueObjectPtr<QQuickWidget> m_quickWidget;
    QmlAnchorBindingProxy m_backendAnchorBinding;
    QmlModelNodeProxy m_backendModelNode;
    std::unique_ptr<TextureEditorTransaction> m_textureEditorTransaction;
    std::unique_ptr<TextureEditorContextObject> m_contextObject;
    AssetImageProvider *m_textureEditorImageProvider = nullptr;
};

} // namespace QmlDesigner

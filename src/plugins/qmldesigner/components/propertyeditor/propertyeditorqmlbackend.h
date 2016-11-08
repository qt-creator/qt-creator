/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    PropertyEditorQmlBackend(PropertyEditorView *propertyEditor);
    ~PropertyEditorQmlBackend();

    void setup(const QmlObjectNode &fxObjectNode, const QString &stateName, const QUrl &qmlSpecificsFile, PropertyEditorView *propertyEditor);
    void initialSetup(const TypeName &typeName, const QUrl &qmlSpecificsFile, PropertyEditorView *propertyEditor);
    void setValue(const QmlObjectNode &fxObjectNode, const PropertyName &name, const QVariant &value);

    QQmlContext *context();
    PropertyEditorContextObject* contextObject();
    QWidget *widget();
    void setSource(const QUrl& url);
    Internal::QmlAnchorBindingProxy &backendAnchorBinding();
    DesignerPropertyMap &backendValuesPropertyMap();
    PropertyEditorTransaction *propertyEditorTransaction();

    PropertyEditorValue *propertyValueForName(const QString &propertyName);

    static QString propertyEditorResourcesPath();
    static QString templateGeneration(NodeMetaInfo type, NodeMetaInfo superType,
                                      const QmlObjectNode &objectNode);

    static QUrl getQmlFileUrl(const TypeName &relativeTypeName, const NodeMetaInfo &info = NodeMetaInfo());
    static QUrl getQmlUrlForModelNode(const ModelNode &modelNode, TypeName &className);

    static bool checkIfUrlExists(const QUrl &url);

    void emitSelectionToBeChanged();
    void emitSelectionChanged();

    void setValueforLayoutAttachedProperties(const QmlObjectNode &qmlObjectNode, const PropertyName &name);

    void setupLayoutAttachedProperties(const QmlObjectNode &qmlObjectNode, PropertyEditorView *propertyEditor);

private:
    void createPropertyEditorValue(const QmlObjectNode &qmlObjectNode,
                                   const PropertyName &name, const QVariant &value,
                                   PropertyEditorView *propertyEditor);
    void setupPropertyEditorValue(const PropertyName &name, PropertyEditorView *propertyEditor, const QString &type);

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

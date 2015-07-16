/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PROPERTYEDITORQMLBACKEND_H
#define PROPERTYEDITORQMLBACKEND_H

#include "qmlanchorbindingproxy.h"
#include "designerpropertymap.h"
#include "propertyeditorvalue.h"
#include "propertyeditorcontextobject.h"
#include "qmlmodelnodeproxy.h"
#include "quick2propertyeditorview.h"

#include <nodemetainfo.h>

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

    static QUrl getQmlFileUrl(const QString &relativeTypeName, const NodeMetaInfo &info = NodeMetaInfo());
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

    static QString qmlFileName(const NodeMetaInfo &nodeInfo);
    static QUrl fileToUrl(const QString &filePath);
    static QString fileFromUrl(const QUrl &url);
    static QString locateQmlFile(const NodeMetaInfo &info, const QString &relativePath);
    static QString fixTypeNameForPanes(const QString &typeName);

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

#endif //PROPERTYEDITORQMLBACKEND_H

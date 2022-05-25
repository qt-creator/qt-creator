/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "designerpropertymap.h"
#include "qmlanchorbindingproxy.h"
#include "qmlmodelnodeproxy.h"

#include <nodemetainfo.h>

class PropertyEditorValue;

QT_BEGIN_NAMESPACE
class QQuickWidget;
QT_END_NAMESPACE

namespace QmlDesigner {

class MaterialEditorContextObject;
class MaterialEditorImageProvider;
class MaterialEditorTransaction;
class MaterialEditorView;

class MaterialEditorQmlBackend
{
    Q_DISABLE_COPY(MaterialEditorQmlBackend)

public:
    MaterialEditorQmlBackend(MaterialEditorView *materialEditor);
    ~MaterialEditorQmlBackend();

    void setup(const QmlObjectNode &selectedMaterialNode, const QString &stateName, const QUrl &qmlSpecificsFile,
               MaterialEditorView *materialEditor);
    void setValue(const QmlObjectNode &fxObjectNode, const PropertyName &name, const QVariant &value);

    QQmlContext *context() const;
    MaterialEditorContextObject *contextObject() const;
    QQuickWidget *widget() const;
    void setSource(const QUrl &url);
    Internal::QmlAnchorBindingProxy &backendAnchorBinding();
    void updateMaterialPreview(const QPixmap &pixmap);
    DesignerPropertyMap &backendValuesPropertyMap();
    MaterialEditorTransaction *materialEditorTransaction() const;

    PropertyEditorValue *propertyValueForName(const QString &propertyName);

    static QString propertyEditorResourcesPath();

    void emitSelectionToBeChanged();
    void emitSelectionChanged();

    void setValueforAuxiliaryProperties(const QmlObjectNode &qmlObjectNode, const PropertyName &name);

private:
    void createPropertyEditorValue(const QmlObjectNode &qmlObjectNode,
                                   const PropertyName &name, const QVariant &value,
                                   MaterialEditorView *materialEditor);
    PropertyName auxNamePostFix(const PropertyName &propertyName);
    QVariant properDefaultAuxiliaryProperties(const QmlObjectNode &qmlObjectNode, const PropertyName &propertyName);

    QQuickWidget *m_view = nullptr;
    Internal::QmlAnchorBindingProxy m_backendAnchorBinding;
    QmlModelNodeProxy m_backendModelNode;
    DesignerPropertyMap m_backendValuesPropertyMap;
    QScopedPointer<MaterialEditorTransaction> m_materialEditorTransaction;
    QScopedPointer<MaterialEditorContextObject> m_contextObject;
    MaterialEditorImageProvider *m_materialEditorImageProvider = nullptr;
};

} // namespace QmlDesigner

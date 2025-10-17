// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modelfwd.h"
#include "propertymetainfo.h"

#include <QList>
#include <QLoggingCategory>
#include <QString>
#include <QVariant>

namespace QmlDesigner {

Q_DECLARE_LOGGING_CATEGORY(ScriptEditorLog)

class AbstractView;
class AbstractProperty;
class BindingProperty;
class ModelNode;
class VariantProperty;

void callLater(const std::function<void()> &fun);
void showErrorMessage(const QString &text);

PropertyName uniquePropertyName(const PropertyName &suggestion, const ModelNode &modelNode);
QString addOnToSignalName(const QString &signal);
QString removeOnFromSignalName(const QString &signal);

NodeMetaInfo dynamicTypeMetaInfo(const AbstractProperty &property);
NodeMetaInfo dynamicTypeNameToNodeMetaInfo(const TypeName &typeName, Model *model);

QVariant typeConvertVariant(const QVariant &variant, const QmlDesigner::TypeName &typeName);
void convertVariantToBindingProperty(const VariantProperty &property, const QVariant &value);
void convertBindingToVariantProperty(const BindingProperty &property, const QVariant &value);

bool isBindingExpression(const QVariant &value);
bool isDynamicVariantPropertyType(const TypeName &type);
QVariant defaultValueForType(const TypeName &type);
QString defaultExpressionForType(const TypeName &type);
std::pair<QString, QString> splitExpression(const QString &expression);

QList<AbstractProperty> dynamicPropertiesFromNode(const ModelNode &node);
QStringList availableSources(AbstractView *view);
QStringList availableTargetProperties(const BindingProperty &bindingProperty);
QStringList availableSourceProperties(const QString &id,
                                      const BindingProperty &targetProp,
                                      AbstractView *view);

} // namespace QmlDesigner

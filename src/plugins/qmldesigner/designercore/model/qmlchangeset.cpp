// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlchangeset.h"
#include "bindingproperty.h"
#include "rewritertransaction.h"
#include "variantproperty.h"
#include "abstractview.h"
#include <metainfo.h>

#include <utils/algorithm.h>

namespace QmlDesigner {

ModelNode QmlModelStateOperation::target() const
{
    if (modelNode().property("target").isBindingProperty())
        return modelNode().bindingProperty("target").resolveToModelNode();
    else
        return ModelNode(); //exception?
}

void QmlModelStateOperation::setTarget(const ModelNode &target)
{
    modelNode().bindingProperty("target").setExpression(target.id());
}

bool QmlModelStateOperation::explicitValue() const
{
    if (modelNode().property("explicit").isVariantProperty())
        return modelNode().variantProperty("explicit").value().toBool();

    return false;
}

void QmlModelStateOperation::setExplicitValue(bool value)
{
    modelNode().variantProperty("explicit").setValue(value);
}

bool QmlModelStateOperation::restoreEntryValues() const
{
    if (modelNode().property("restoreEntryValues").isVariantProperty())
        return modelNode().variantProperty("restoreEntryValues").value().toBool();

    return false;
}

void QmlModelStateOperation::setRestoreEntryValues(bool value)
{
    modelNode().variantProperty("restoreEntryValues").setValue(value);
}

QList<AbstractProperty> QmlModelStateOperation::targetProperties() const
{
    return Utils::filtered(modelNode().properties(), [](const AbstractProperty &property) {
        const QList<PropertyName> ignore = {"target", "explicit", "restoreEntryValues"};
        return !ignore.contains(property.name());
    });
}

bool QmlPropertyChanges::isValid() const
{
    return isValidQmlPropertyChanges(modelNode());
}

bool QmlPropertyChanges::isValidQmlPropertyChanges(const ModelNode &modelNode)
{
    return isValidQmlModelNodeFacade(modelNode) && modelNode.metaInfo().isQtQuickPropertyChanges();
}

bool QmlModelStateOperation::isValid() const
{
    return isValidQmlModelStateOperation(modelNode());
}

bool QmlModelStateOperation::isValidQmlModelStateOperation(const ModelNode &modelNode)
{
    return isValidQmlModelNodeFacade(modelNode) && modelNode.metaInfo().isQtQuickStateOperation();
}

void QmlPropertyChanges::removeProperty(const PropertyName &name)
{
    RewriterTransaction transaction(view()->beginRewriterTransaction(QByteArrayLiteral("QmlPropertyChanges::removeProperty")));
    if (name == "name")
        return;
    modelNode().removeProperty(name);
    if (modelNode().variantProperties().isEmpty() && modelNode().bindingProperties().size() < 2)
        modelNode().destroy();
}

} //QmlDesigner

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlchangeset.h"
#include "bindingproperty.h"
#include "rewritertransaction.h"
#include "variantproperty.h"
#include "abstractview.h"

#include <utils/algorithm.h>

namespace QmlDesigner {

using NanotraceHR::keyValue;

using ModelTracing::category;

ModelNode QmlModelStateOperation::target(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state operation target",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (modelNode().property("target").isBindingProperty())
        return modelNode().bindingProperty("target").resolveToModelNode();
    else
        return ModelNode(); //exception?
}

void QmlModelStateOperation::setTarget(const ModelNode &target, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model state operation set target",
                               category(),
                               keyValue("model node", *this),
                               keyValue("target", target),
                               keyValue("caller location", sl)};

    modelNode().bindingProperty("target").setExpression(target.id());
}

bool QmlModelStateOperation::explicitValue(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state operation explicit value",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (modelNode().property("explicit").isVariantProperty())
        return modelNode().variantProperty("explicit").value().toBool();

    return false;
}

void QmlModelStateOperation::setExplicitValue(bool value, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model state operation set explicit value",
                               category(),
                               keyValue("model node", *this),
                               keyValue("value", value),
                               keyValue("caller location", sl)};

    modelNode().variantProperty("explicit").setValue(value);
}

bool QmlModelStateOperation::restoreEntryValues(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state operation restore entry values",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (modelNode().property("restoreEntryValues").isVariantProperty())
        return modelNode().variantProperty("restoreEntryValues").value().toBool();

    return false;
}

void QmlModelStateOperation::setRestoreEntryValues(bool value, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model state operation set restore entry values",
                               category(),
                               keyValue("model node", *this),
                               keyValue("value", value),
                               keyValue("caller location", sl)};

    modelNode().variantProperty("restoreEntryValues").setValue(value);
}

QList<AbstractProperty> QmlModelStateOperation::targetProperties(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state operation target properties",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return Utils::filtered(modelNode().properties(), [](const AbstractProperty &property) {
        const QList<PropertyName> ignore = {"target", "explicit", "restoreEntryValues"};
        return !ignore.contains(property.name());
    });
}

bool QmlPropertyChanges::isValid(SL sl) const
{
    return isValidQmlPropertyChanges(modelNode(), sl);
}

bool QmlPropertyChanges::isValidQmlPropertyChanges(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"is valid qml property changes",
                               category(),
                               keyValue("model node", modelNode),
                               keyValue("caller location", sl)};

    return isValidQmlModelNodeFacade(modelNode) && modelNode.metaInfo().isQtQuickPropertyChanges();
}

bool QmlModelStateOperation::isValid(SL sl) const
{
    return isValidQmlModelStateOperation(modelNode(), sl);
}

bool QmlModelStateOperation::isValidQmlModelStateOperation(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"is valid qml model state operation",
                               category(),
                               keyValue("model node", modelNode),
                               keyValue("caller location", sl)};

    return isValidQmlModelNodeFacade(modelNode) && modelNode.metaInfo().isQtQuickStateOperation();
}

void QmlPropertyChanges::removeProperty(PropertyNameView name, SL sl)
{
    NanotraceHR::Tracer tracer{"qml property changes remove property",
                               category(),
                               keyValue("model node", *this),
                               keyValue("property name", name),
                               keyValue("caller location", sl)};

    RewriterTransaction transaction(view()->beginRewriterTransaction(QByteArrayLiteral("QmlPropertyChanges::removeProperty")));
    if (name == "name")
        return;
    modelNode().removeProperty(name);
    if (modelNode().variantProperties().isEmpty() && modelNode().bindingProperties().size() < 2)
        modelNode().destroy();
}

} //QmlDesigner

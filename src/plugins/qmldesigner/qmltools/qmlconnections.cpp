// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlconnections.h"

#include <abstractview.h>
#include <bindingproperty.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

using NanotraceHR::keyValue;

using ModelTracing::category;

bool QmlConnections::isValid(SL sl) const
{
    return isValidQmlConnections(modelNode(), sl);
}

bool QmlConnections::isValidQmlConnections(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"qml connection is valid qml connections",
                               category(),
                               keyValue("model node", modelNode),
                               keyValue("caller location", sl)};
    return isValidQmlModelNodeFacade(modelNode) && modelNode.metaInfo().isValid()
           && (modelNode.type() == "Connections" || modelNode.type() == "QtQuick.Connections"
               || modelNode.type() == "Qt.Connections" || modelNode.type() == "QtQml.Connections");
}

/*!
    Removes connections node.
*/
void QmlConnections::destroy(SL sl)
{
    NanotraceHR::Tracer tracer{"qml connection destroy",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};
    modelNode().destroy();
}

QString QmlConnections::target(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml connections target",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (modelNode().isValid()) {
        const BindingProperty bindingproperty = modelNode().bindingProperty("target");
        if (bindingproperty.isValid())
            return bindingproperty.expression();
    }

    return QString();
}

void QmlConnections::setTarget(const QString &target, SL sl)
{
    NanotraceHR::Tracer tracer{"qml connections set target",
                               category(),
                               keyValue("model node", *this),
                               keyValue("target", target),
                               keyValue("caller location", sl)};

    modelNode().bindingProperty("target").setExpression(target);
}

ModelNode QmlConnections::getTargetNode(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml connections get target node",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    ModelNode result;

    if (!modelNode().isValid())
        return result;

    const BindingProperty bindingProperty = modelNode().bindingProperty("target");
    const QString bindExpression = bindingProperty.expression();

    if (bindingProperty.isValid()) {
        AbstractView *view = modelNode().view();
        if (bindExpression.contains(".")) {
            QStringList substr = bindExpression.split(".");
            const QString itemId = substr.constFirst();
            if (substr.size() > 1) {
                const ModelNode aliasParent = view->modelNodeForId(itemId);
                substr.removeFirst(); // remove id, only alias pieces left
                const QString aliasBody = substr.join(".");
                if (aliasParent.isValid() && aliasParent.hasBindingProperty(aliasBody.toUtf8())) {
                    const BindingProperty binding = aliasParent.bindingProperty(aliasBody.toUtf8());
                    if (binding.isValid() && view->hasId(binding.expression()))
                        result = view->modelNodeForId(binding.expression());
                }
            }
        } else {
            result = view->modelNodeForId(bindExpression);
        }
    }

    return result;
}

QList<SignalHandlerProperty> QmlConnections::signalProperties(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml connections signal properties",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().signalProperties();
}

ModelNode QmlConnections::createQmlConnections(AbstractView *view, SL sl)
{
    NanotraceHR::Tracer tracer{"qml connections create qml connections",
                               category(),
                               keyValue("view", view),
                               keyValue("caller location", sl)};

#ifdef QDS_USE_PROJECTSTORAGE
    return view->createModelNode("Connections");
#else
    NodeMetaInfo nodeMetaInfo = view->model()->qtQmlConnectionsMetaInfo();

    return view->createModelNode("QtQuick.Connections",
                                 nodeMetaInfo.majorVersion(),
                                 nodeMetaInfo.minorVersion());
#endif
}

} // QmlDesigner

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlmodelnodefacade.h"
#include "nodeinstanceview.h"

namespace QmlDesigner {

using NanotraceHR::keyValue;

using ModelTracing::category;

AbstractView *QmlModelNodeFacade::view() const
{
    return m_modelNode.view();
}

Model *QmlModelNodeFacade::model() const
{
    return m_modelNode.model();
}

const NodeInstanceView *nodeInstanceView(const ModelNode &node)
{
    if (auto model = node.model())
        return static_cast<const NodeInstanceView *>(model->nodeInstanceView());

    return nullptr;
}

const NodeInstanceView *QmlModelNodeFacade::nodeInstanceView(const ModelNode &modelNode)
{
    return QmlDesigner::nodeInstanceView(modelNode);
}

const NodeInstanceView *QmlModelNodeFacade::nodeInstanceView() const
{
    return nodeInstanceView(m_modelNode);
}

bool QmlModelNodeFacade::hasModelNode() const
{
    return m_modelNode.isValid();
}

bool QmlModelNodeFacade::isValid(SL sl) const
{
    NanotraceHR::Tracer tracer{"is valid qml model node facade",
                               category(),
                               keyValue("model node", m_modelNode),
                               keyValue("caller location", sl)};

    return isValidQmlModelNodeFacade(m_modelNode);
}

bool QmlModelNodeFacade::isValidQmlModelNodeFacade(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"is valid qml model node facade",
                               category(),
                               keyValue("model node", modelNode),
                               keyValue("caller location", sl)};

    return modelNode.isValid() && nodeInstanceView(modelNode)
           && nodeInstanceView(modelNode)->hasInstanceForModelNode(modelNode)
           && nodeInstanceView(modelNode)->instanceForModelNode(modelNode).isValid();
}

bool QmlModelNodeFacade::isRootNode(SL sl) const
{
    NanotraceHR::Tracer tracer{"is root node",
                               category(),
                               keyValue("model node", m_modelNode),
                               keyValue("caller location", sl)};

    return m_modelNode.isRootNode();
}

} //QmlDesigner

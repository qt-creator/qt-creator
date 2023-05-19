// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlmodelnodefacade.h"
#include "nodeinstanceview.h"

namespace QmlDesigner {

AbstractView *QmlModelNodeFacade::view() const
{
    if (modelNode().isValid())
        return modelNode().view();
    else
        return nullptr;
}

Model *QmlModelNodeFacade::model() const
{
    return m_modelNode.model();
}

const NodeInstanceView *QmlModelNodeFacade::nodeInstanceView(const ModelNode &modelNode)
{
    return modelNode.model()->nodeInstanceView();
}

const NodeInstanceView *QmlModelNodeFacade::nodeInstanceView() const
{
    return nodeInstanceView(m_modelNode);
}

bool QmlModelNodeFacade::hasModelNode() const
{
    return m_modelNode.isValid();
}

bool QmlModelNodeFacade::isValid() const
{
    return isValidQmlModelNodeFacade(m_modelNode);
}

namespace {
bool workaroundForIsValidQmlModelNodeFacadeInTests = false;
}

bool QmlModelNodeFacade::isValidQmlModelNodeFacade(const ModelNode &modelNode)
{
    if (workaroundForIsValidQmlModelNodeFacadeInTests) {
        return modelNode.isValid();
    }

    return modelNode.isValid() && nodeInstanceView(modelNode)
           && nodeInstanceView(modelNode)->hasInstanceForModelNode(modelNode)
           && nodeInstanceView(modelNode)->instanceForModelNode(modelNode).isValid();
}

bool QmlModelNodeFacade::isRootNode() const
{
    return modelNode().isRootNode();
}

void QmlModelNodeFacade::enableUglyWorkaroundForIsValidQmlModelNodeFacadeInTests()
{
    workaroundForIsValidQmlModelNodeFacadeInTests = true;
}
} //QmlDesigner

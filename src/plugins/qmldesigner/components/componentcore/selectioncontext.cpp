// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "selectioncontext.h"
#include "componentcoretracing.h"

#include <qmlstate.h>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using QmlDesigner::ComponentCoreTracing::category;

SelectionContext::SelectionContext()
{
    NanotraceHR::Tracer tracer{"selection context constructor", category()};
}

SelectionContext::SelectionContext(AbstractView *view) :
    m_view(view)
{
    NanotraceHR::Tracer tracer{"selection context constructor with view", category()};
}

void SelectionContext::setTargetNode(const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"selection context set target node",
                               category(),
                               keyValue("model node", modelNode)};

    m_targetNode = modelNode;
}

ModelNode SelectionContext::targetNode() const
{
    NanotraceHR::Tracer tracer{"selection context get target node", category()};

    return m_targetNode;
}

ModelNode SelectionContext::rootNode() const
{
    NanotraceHR::Tracer tracer{"selection context get root node", category()};

    return view()->rootModelNode();
}

bool SelectionContext::singleNodeIsSelected() const
{
    NanotraceHR::Tracer tracer{"selection context check single node is selected", category()};

    return view()->hasSingleSelectedModelNode();
}

bool SelectionContext::isInBaseState() const
{
    NanotraceHR::Tracer tracer{"selection context check is in base state", category()};

    return QmlModelState::isBaseState(view()->currentStateNode());
}

ModelNode SelectionContext::currentSingleSelectedNode() const
{
    NanotraceHR::Tracer tracer{"selection context get current single selected node", category()};

    return view()->singleSelectedModelNode();
}

ModelNode SelectionContext::firstSelectedModelNode() const
{
    NanotraceHR::Tracer tracer{"selection context get first selected model node", category()};

    return view()->firstSelectedModelNode();
}

QList<ModelNode> SelectionContext::selectedModelNodes() const
{
    NanotraceHR::Tracer tracer{"selection context get selected model nodes", category()};

    return view()->selectedModelNodes();
}

bool SelectionContext::hasSingleSelectedModelNode() const
{
    NanotraceHR::Tracer tracer{"selection context check has single selected model node", category()};

    return view()->hasSingleSelectedModelNode() && firstSelectedModelNode().isValid();
}

AbstractView *SelectionContext::view() const
{
    NanotraceHR::Tracer tracer{"selection context get view", category()};

    return m_view.data();
}

Model *SelectionContext::model() const
{
    NanotraceHR::Tracer tracer{"selection context get model", category()};

    if (m_view && m_view->isAttached())
        return m_view->model();

    return nullptr;
}

void SelectionContext::setShowSelectionTools(bool show)
{
    NanotraceHR::Tracer tracer{"selection context set show selection tools",
                               category(),
                               keyValue("show", show)};

    m_showSelectionTools = show;
}

bool SelectionContext::showSelectionTools() const
{
    NanotraceHR::Tracer tracer{"selection context get show selection tools", category()};

    return m_showSelectionTools;
}

void SelectionContext::setScenePosition(const QPointF &postition)
{
    NanotraceHR::Tracer tracer{"selection context set scene position", category()};

    m_scenePosition = postition;
}

QPointF SelectionContext::scenePosition() const
{
    NanotraceHR::Tracer tracer{"selection context get scene position", category()};

    return m_scenePosition;
}

void SelectionContext::setToggled(bool toggled)
{
    NanotraceHR::Tracer tracer{"selection context set toggled",
                               category(),
                               keyValue("toggled", toggled)};

    m_toggled = toggled;
}

bool SelectionContext::toggled() const
{
    NanotraceHR::Tracer tracer{"selection context get toggled", category()};

    return m_toggled;
}

bool SelectionContext::isValid() const
{
    NanotraceHR::Tracer tracer{"selection context is valid", category()};

    return view() && view()->model();
}

bool SelectionContext::fastUpdate() const
{
    NanotraceHR::Tracer tracer{"selection context fast update", category()};

    return m_updateReason != UpdateMode::Normal;
}

void SelectionContext::setUpdateMode(UpdateMode mode)
{
    NanotraceHR::Tracer tracer{"selection context set update mode", category()};

    m_updateReason = mode;
}

SelectionContext::UpdateMode SelectionContext::updateReason() const
{
    NanotraceHR::Tracer tracer{"selection context get update reason", category()};

    return m_updateReason;
}

} //QmlDesigner

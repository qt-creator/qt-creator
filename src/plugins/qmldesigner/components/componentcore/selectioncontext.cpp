// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "selectioncontext.h"

#include <qmlstate.h>

namespace QmlDesigner {

SelectionContext::SelectionContext() = default;

SelectionContext::SelectionContext(AbstractView *view) :
    m_view(view)
{
}

void SelectionContext::setTargetNode(const ModelNode &modelNode)
{
    m_targetNode = modelNode;
}

ModelNode SelectionContext::targetNode() const
{
    return m_targetNode;
}

ModelNode SelectionContext::rootNode() const
{
    return view()->rootModelNode();
}

bool SelectionContext::singleNodeIsSelected() const
{
    return view()->hasSingleSelectedModelNode();
}

bool SelectionContext::isInBaseState() const
{
    return view()->currentState().isBaseState();
}

ModelNode SelectionContext::currentSingleSelectedNode() const
{
    return view()->singleSelectedModelNode();
}

ModelNode SelectionContext::firstSelectedModelNode() const
{
    return view()->firstSelectedModelNode();
}

QList<ModelNode> SelectionContext::selectedModelNodes() const
{
    return view()->selectedModelNodes();
}

bool SelectionContext::hasSingleSelectedModelNode() const
{
    return view()->hasSingleSelectedModelNode()
            && firstSelectedModelNode().isValid();
}

AbstractView *SelectionContext::view() const
{
    return m_view.data();
}

void SelectionContext::setShowSelectionTools(bool show)
{
    m_showSelectionTools = show;
}

bool SelectionContext::showSelectionTools() const
{
    return m_showSelectionTools;
}

void SelectionContext::setScenePosition(const QPointF &postition)
{
    m_scenePosition = postition;
}

QPointF SelectionContext::scenePosition() const
{
    return m_scenePosition;
}

void SelectionContext::setToggled(bool toggled)
{
    m_toggled = toggled;
}

bool SelectionContext::toggled() const
{
    return m_toggled;
}

bool SelectionContext::isValid() const
{
    return view() && view()->model();
}

bool SelectionContext::fastUpdate() const
{
    return m_updateReason != UpdateMode::Normal;
}

void SelectionContext::setUpdateMode(UpdateMode mode)
{
    m_updateReason = mode;
}

SelectionContext::UpdateMode SelectionContext::updateReason() const
{
    return m_updateReason;
}

} //QmlDesigner

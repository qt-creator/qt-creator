/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "selectioncontext.h"

#include <qmlstate.h>

namespace QmlDesigner {


SelectionContext::SelectionContext() :
    m_toggled(false)
{

}

SelectionContext::SelectionContext(AbstractView *view) :
    m_view(view),
    m_toggled(false)
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
    return view()->hasSelectedModelNodes();
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

} //QmlDesigner

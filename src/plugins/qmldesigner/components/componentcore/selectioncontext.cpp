/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "selectioncontext.h"

namespace QmlDesigner {


SelectionContext::SelectionContext() :
    m_isInBaseState(false),
    m_toggled(false)
{

}

SelectionContext::SelectionContext(QmlModelView *qmlModelView) :
    m_qmlModelView(qmlModelView),
    m_isInBaseState(qmlModelView->currentState().isBaseState()),
    m_toggled(false)
{
    if (qmlModelView && qmlModelView->model())
        m_selectedModelNodes = qmlModelView->selectedModelNodes();
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
    return m_selectedModelNodes.count() == 1;
}

bool SelectionContext::isInBaseState() const
{
    return m_isInBaseState;
}

ModelNode SelectionContext::currentSingleSelectedNode() const
{
    if (m_selectedModelNodes.count() != 1)
        return ModelNode();

    return m_selectedModelNodes.first();
}

QList<ModelNode> SelectionContext::selectedModelNodes() const
{
    return m_selectedModelNodes;
}

QmlModelView *SelectionContext::qmlModelView() const
{
    return m_qmlModelView.data();
}

void SelectionContext::setShowSelectionTools(bool show)
{
    m_showSelectionTools = show;
}

bool SelectionContext::showSelectionTools() const
{
    return m_showSelectionTools;
}

void SelectionContext::setScenePos(const QPoint &postition)
{
    m_scenePosition = postition;
}

QPoint SelectionContext::scenePos() const
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
    return qmlModelView() && qmlModelView()->model() && qmlModelView()->nodeInstanceView();
}

} //QmlDesigner

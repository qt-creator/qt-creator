/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "qmlmodelnodefacade.h"
#include "nodeinstanceview.h"

namespace QmlDesigner {

QmlModelNodeFacade::QmlModelNodeFacade() : m_modelNode(ModelNode())
{}

AbstractView *QmlModelNodeFacade::view() const
{
    if (modelNode().isValid())
        return modelNode().view();
    else
        return 0;
}

NodeInstanceView *QmlModelNodeFacade::nodeInstanceView(const ModelNode &modelNode)
{
    return modelNode.model()->nodeInstanceView();
}

NodeInstanceView *QmlModelNodeFacade::nodeInstanceView() const
{
    return nodeInstanceView(m_modelNode);
}


QmlModelNodeFacade::QmlModelNodeFacade(const ModelNode &modelNode) : m_modelNode(modelNode)
{}

QmlModelNodeFacade::~QmlModelNodeFacade()
{}

QmlModelNodeFacade::operator ModelNode() const
{
    return m_modelNode;
}

ModelNode QmlModelNodeFacade::modelNode()
{
    return m_modelNode;
}

const ModelNode QmlModelNodeFacade::modelNode() const
{
    return m_modelNode;
}

bool QmlModelNodeFacade::hasModelNode() const
{
    return m_modelNode.isValid();
}

bool QmlModelNodeFacade::isValid() const
{
    return isValidQmlModelNodeFacade(m_modelNode);
}

bool QmlModelNodeFacade::isValidQmlModelNodeFacade(const ModelNode &modelNode)
{
    return modelNode.isValid()
            && nodeInstanceView(modelNode)
            && nodeInstanceView(modelNode)->hasInstanceForModelNode(modelNode)
            && nodeInstanceView(modelNode)->instanceForModelNode(modelNode).isValid();
}

bool QmlModelNodeFacade::isRootNode() const
{
    return modelNode().isRootNode();
}
} //QmlDesigner

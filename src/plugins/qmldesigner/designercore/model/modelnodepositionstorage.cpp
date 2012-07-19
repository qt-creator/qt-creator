/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "invalidmodelnodeexception.h"
#include "modelnodepositionstorage.h"

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

void ModelNodePositionStorage::setNodeOffset(const ModelNode &modelNode, int fileOffset)
{
    m_rewriterData[modelNode].setOffset(fileOffset);
}

void ModelNodePositionStorage::cleanupInvalidOffsets()
{
    QHash<ModelNode, RewriterData> validModelNodes;

    QHashIterator<ModelNode, RewriterData> iter(m_rewriterData);
    while (iter.hasNext()) {
        iter.next();
        const ModelNode modelNode = iter.key();
        if (modelNode.isValid())
            validModelNodes.insert(modelNode, iter.value());
    }
    m_rewriterData = validModelNodes;
}

int ModelNodePositionStorage::nodeOffset(const ModelNode &modelNode)
{
    QHash<ModelNode, RewriterData>::const_iterator iter = m_rewriterData.find(modelNode);
    if (iter == m_rewriterData.end())
        return INVALID_LOCATION;
    else
        return iter.value().offset();
}

QList<ModelNode> ModelNodePositionStorage::modelNodes() const
{
    return m_rewriterData.keys();
}

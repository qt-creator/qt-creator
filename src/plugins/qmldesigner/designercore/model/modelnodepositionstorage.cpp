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
    auto iter = m_rewriterData.find(modelNode);
    if (iter == m_rewriterData.end())
        return INVALID_LOCATION;
    else
        return iter.value().offset();
}

QList<ModelNode> ModelNodePositionStorage::modelNodes() const
{
    return m_rewriterData.keys();
}

/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qml3dnode.h"
#include <metainfo.h>
#include "qmlchangeset.h"
#include "nodelistproperty.h"
#include "nodehints.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "qmlanchors.h"
#include "invalidmodelnodeexception.h"
#include "itemlibraryinfo.h"

#include "plaintexteditmodifier.h"
#include "rewriterview.h"
#include "modelmerger.h"
#include "rewritingexception.h"

#include <QUrl>
#include <QPlainTextEdit>
#include <QFileInfo>
#include <QDir>

namespace QmlDesigner {

bool Qml3DNode::isValid() const
{
    return isValidQml3DNode(modelNode());
}

bool Qml3DNode::isValidQml3DNode(const ModelNode &modelNode)
{
    return isValidQmlObjectNode(modelNode)
            && modelNode.metaInfo().isValid()
            && (modelNode.metaInfo().isSubclassOf("QtQuick3D.Node"));
}

QList<ModelNode> toModelNodeList(const QList<Qml3DNode> &qmlVisualNodeList)
{
    QList<ModelNode> modelNodeList;

    for (const Qml3DNode &qml3DNode : qmlVisualNodeList)
        modelNodeList.append(qml3DNode.modelNode());

    return modelNodeList;
}

QList<Qml3DNode> toQml3DNodeList(const QList<ModelNode> &modelNodeList)
{
    QList<Qml3DNode> qml3DNodeList;

    for (const ModelNode &modelNode : modelNodeList) {
        if (Qml3DNode::isValidQml3DNode(modelNode))
            qml3DNodeList.append(modelNode);
    }

    return qml3DNodeList;
}

} //QmlDesigner

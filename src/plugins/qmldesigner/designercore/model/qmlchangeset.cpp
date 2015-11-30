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

#include "qmlchangeset.h"
#include "bindingproperty.h"
#include "variantproperty.h"
#include "abstractview.h"
#include <metainfo.h>

namespace QmlDesigner {

ModelNode QmlModelStateOperation::target() const
{
    if (modelNode().property("target").isBindingProperty())
        return modelNode().bindingProperty("target").resolveToModelNode();
    else
        return ModelNode(); //exception?
}

void QmlModelStateOperation::setTarget(const ModelNode &target)
{
    modelNode().bindingProperty("target").setExpression(target.id());
}

bool QmlPropertyChanges::isValid() const
{
    return isValidQmlPropertyChanges(modelNode());
}

bool QmlPropertyChanges::isValidQmlPropertyChanges(const ModelNode &modelNode)
{
    return isValidQmlModelNodeFacade(modelNode) && modelNode.metaInfo().isSubclassOf("QtQuick.PropertyChanges", -1, -1);
}

bool QmlModelStateOperation::isValid() const
{
    return isValidQmlModelStateOperation(modelNode());
}

bool QmlModelStateOperation::isValidQmlModelStateOperation(const ModelNode &modelNode)
{
    return isValidQmlModelNodeFacade(modelNode)
            && (modelNode.metaInfo().isSubclassOf("<cpp>.QDeclarative1StateOperation", -1, -1)
                || modelNode.metaInfo().isSubclassOf("<cpp>.QQuickStateOperation", -1, -1));
}

void QmlPropertyChanges::removeProperty(const PropertyName &name)
{
    RewriterTransaction transaction(view()->beginRewriterTransaction(QByteArrayLiteral("QmlPropertyChanges::removeProperty")));
    if (name == "name")
        return;
    modelNode().removeProperty(name);
    if (modelNode().variantProperties().isEmpty() && modelNode().bindingProperties().count() < 2)
        modelNode().destroy();
}

} //QmlDesigner

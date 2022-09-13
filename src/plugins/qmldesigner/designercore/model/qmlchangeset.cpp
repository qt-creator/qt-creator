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

#include "qmlchangeset.h"
#include "bindingproperty.h"
#include "variantproperty.h"
#include "abstractview.h"
#include <metainfo.h>

#include <utils/algorithm.h>

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

bool QmlModelStateOperation::explicitValue() const
{
    if (modelNode().property("explicit").isVariantProperty())
        return modelNode().variantProperty("explicit").value().toBool();

    return false;
}

void QmlModelStateOperation::setExplicitValue(bool value)
{
    modelNode().variantProperty("explicit").setValue(value);
}

bool QmlModelStateOperation::restoreEntryValues() const
{
    if (modelNode().property("restoreEntryValues").isVariantProperty())
        return modelNode().variantProperty("restoreEntryValues").value().toBool();

    return false;
}

void QmlModelStateOperation::setRestoreEntryValues(bool value)
{
    modelNode().variantProperty("restoreEntryValues").setValue(value);
}

QList<AbstractProperty> QmlModelStateOperation::targetProperties() const
{
    return Utils::filtered(modelNode().properties(), [](const AbstractProperty &property) {
        const QList<PropertyName> ignore = {"target", "explicit", "restoreEntryValues"};
        return !ignore.contains(property.name());
    });
}

bool QmlPropertyChanges::isValid() const
{
    return isValidQmlPropertyChanges(modelNode());
}

bool QmlPropertyChanges::isValidQmlPropertyChanges(const ModelNode &modelNode)
{
    return isValidQmlModelNodeFacade(modelNode) && modelNode.metaInfo().isSubclassOf("QtQuick.PropertyChanges");
}

bool QmlModelStateOperation::isValid() const
{
    return isValidQmlModelStateOperation(modelNode());
}

bool QmlModelStateOperation::isValidQmlModelStateOperation(const ModelNode &modelNode)
{
    return isValidQmlModelNodeFacade(modelNode)
            && (modelNode.metaInfo().isSubclassOf("<cpp>.QDeclarative1StateOperation")
                || modelNode.metaInfo().isSubclassOf("<cpp>.QQuickStateOperation"));
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

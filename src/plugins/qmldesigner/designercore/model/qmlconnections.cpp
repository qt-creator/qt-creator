/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qmlconnections.h"

#include <metainfo.h>
#include <abstractview.h>
#include <bindingproperty.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

QmlConnections::QmlConnections()
{
}

QmlConnections::QmlConnections(const ModelNode &modelNode)
    : QmlModelNodeFacade(modelNode)
{
}

bool QmlConnections::isValid() const
{
    return isValidQmlConnections(modelNode());
}

bool QmlConnections::isValidQmlConnections(const ModelNode &modelNode)
{
    return isValidQmlModelNodeFacade(modelNode)
            && modelNode.metaInfo().isValid()
            && (modelNode.type() == "Connections"
                || modelNode.type() == "QtQuick.Connections"
                || modelNode.type() == "Qt.Connections"
                || modelNode.type() == "QtQml.Connections");
}

/*!
    Removes connections node.
*/
void QmlConnections::destroy()
{
    Q_ASSERT(isValid());
    modelNode().destroy();
}

QString QmlConnections::target() const
{
    if (modelNode().isValid()) {
        const BindingProperty bindingproperty = modelNode().bindingProperty("target");
        if (bindingproperty.isValid())
            return bindingproperty.expression();
    }

    return QString();
}

void QmlConnections::setTarget(const QString &target)
{
    modelNode().bindingProperty("target").setExpression(target);
}

ModelNode QmlConnections::getTargetNode() const
{
    ModelNode result;

    if (!modelNode().isValid())
        return result;

    const BindingProperty bindingProperty = modelNode().bindingProperty("target");
    const QString bindExpression = bindingProperty.expression();

    if (bindingProperty.isValid()) {
        AbstractView *view = modelNode().view();
        if (bindExpression.contains(".")) {
            QStringList substr = bindExpression.split(".");
            const QString itemId = substr.constFirst();
            if (substr.size() > 1) {
                const ModelNode aliasParent = view->modelNodeForId(itemId);
                substr.removeFirst(); // remove id, only alias pieces left
                const QString aliasBody = substr.join(".");
                if (aliasParent.isValid() && aliasParent.hasBindingProperty(aliasBody.toUtf8())) {
                    const BindingProperty binding = aliasParent.bindingProperty(aliasBody.toUtf8());
                    if (binding.isValid() && view->hasId(binding.expression()))
                        result = view->modelNodeForId(binding.expression());
                }
            }
        } else {
            result = view->modelNodeForId(bindExpression);
        }
    }

    return result;
}

QList<SignalHandlerProperty> QmlConnections::signalProperties() const
{
    return modelNode().signalProperties();
}

ModelNode QmlConnections::createQmlConnections(AbstractView *view)
{
    NodeMetaInfo nodeMetaInfo = view->model()->metaInfo("QtQuick.Connections");
    return view->createModelNode("QtQuick.Connections",
                                 nodeMetaInfo.majorVersion(),
                                 nodeMetaInfo.minorVersion());
}

} // QmlDesigner

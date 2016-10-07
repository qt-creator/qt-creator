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

#include "nodehints.h"
#include "model.h"

#include "metainfo.h"
#include <enumeration.h>
#include <rewriterview.h>
#include <propertyparser.h>
#include <nodeabstractproperty.h>

#include <QDebug>

#include <qmljs/qmljsscopechain.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsvalueowner.h>
#include <languageutils/fakemetaobject.h>

#include <utils/qtcassert.h>

#include <itemlibraryinfo.h>

#include <QJSEngine>

namespace QmlDesigner {


namespace Internal {

static QJSEngine *s_qJSEngine= nullptr;
static JSObject *s_jsObject = nullptr;

static QVariant evaluateExpression(const QString &expression, const ModelNode &modelNode)
{
    if (!s_qJSEngine) {
        s_qJSEngine = new QJSEngine;
        s_jsObject = new JSObject(s_qJSEngine);
        QJSValue jsValue = s_qJSEngine->newQObject(s_jsObject);
        s_qJSEngine->globalObject().setProperty("model", jsValue);
    }

    s_jsObject->setModelNode(modelNode);
    return s_qJSEngine->evaluate(expression).toVariant();
}

} //Internal

QmlDesigner::NodeHints::NodeHints()
{

}

QmlDesigner::NodeHints::NodeHints(const ModelNode &node) : m_modelNode(node)
{
    if (isValid()) {
        const ItemLibraryInfo *libraryInfo = model()->metaInfo().itemLibraryInfo();
        QList <ItemLibraryEntry> itemLibraryEntryList = libraryInfo->entriesForType(
                    modelNode().type(), modelNode().majorVersion(), modelNode().minorVersion());
        m_hints =  itemLibraryEntryList.first().hints();
    }
}

bool NodeHints::canBeContainer() const
{
    /* The default is true for now to avoid confusion. Once our .metaInfo files in Qt
       use the feature we can change the default to false. */

    if (!isValid())
        return true;

    return evaluateBooleanExpression("canBeContainer", true);
}

bool NodeHints::forceClip() const
{
    if (!isValid())
        return false;

    return evaluateBooleanExpression("forceClip", false);
}

bool NodeHints::doesLayoutChildren() const
{
    if (!isValid())
        return false;

    return evaluateBooleanExpression("doesLayoutChildren", false);
}

bool NodeHints::canBeDroppedInFormEditor() const
{
    if (!isValid())
        return false;

    return evaluateBooleanExpression("canBeDroppedInFormEditor", false);
}

bool NodeHints::canBeDroppedInNavigator() const
{
    if (!isValid())
        return false;

    return evaluateBooleanExpression("canBeDroppedInNavigator", false);
}

bool NodeHints::isMovable() const
{
    if (!isValid())
        return true;

    return evaluateBooleanExpression("isMovable", true);
}

bool NodeHints::isStackedContainer() const
{
    if (!isValid())
        return false;

    return evaluateBooleanExpression("isStackedContainer", false);
}

QString NodeHints::indexPropertyForStackedContainer() const
{
    if (!isValid())
        return QString();

    const QString expression = m_hints.value("indexPropertyForStackedContainer");

    if (expression.isEmpty())
        return QString();

    return Internal::evaluateExpression(expression, modelNode()).toString();
}

QHash<QString, QString> NodeHints::hints() const
{
    return m_hints;
}

ModelNode NodeHints::modelNode() const
{
    return m_modelNode;
}

bool NodeHints::isValid() const
{
    return modelNode().isValid();
}

Model *NodeHints::model() const
{
    return modelNode().model();
}

bool NodeHints::evaluateBooleanExpression(const QString &hintName, bool defaultValue) const
{
    const QString expression = m_hints.value(hintName);

    if (expression.isEmpty())
        return defaultValue;

    return Internal::evaluateExpression(expression, modelNode()).toBool();
}

namespace Internal {

QmlDesigner::Internal::JSObject::JSObject::JSObject(QObject *parent) : QObject(parent)
{

}

void JSObject::setModelNode(const ModelNode &node)
{
    m_modelNode = node;
    emit modelNodeChanged();
}

bool JSObject::hasParent() const
{
    return !m_modelNode.isRootNode()
            && m_modelNode.hasParentProperty();
}

bool JSObject::hasChildren() const
{
    return m_modelNode.hasAnySubModelNodes();
}

bool JSObject::currentParentIsRoot() const
{
    return m_modelNode.hasParentProperty()
            && m_modelNode.parentProperty().isValid()
            && m_modelNode.parentProperty().parentModelNode().isRootNode();
}

bool JSObject::isSubclassOf(const QString &typeName)
{
    NodeMetaInfo metaInfo = m_modelNode.metaInfo();

    if (metaInfo.isValid())
        metaInfo.isSubclassOf(typeName.toUtf8());

    return false;
}

bool JSObject::rootItemIsSubclassOf(const QString &typeName)
{
    NodeMetaInfo metaInfo = m_modelNode.view()->rootModelNode().metaInfo();

    if (metaInfo.isValid())
        metaInfo.isSubclassOf(typeName.toUtf8());

    return false;
}

bool JSObject::currentParentIsSubclassOf(const QString &typeName)
{
    if ( m_modelNode.hasParentProperty()
         && m_modelNode.parentProperty().isValid()) {
        NodeMetaInfo metaInfo =  m_modelNode.parentProperty().parentModelNode().metaInfo();
        if (metaInfo.isValid())
            metaInfo.isSubclassOf(typeName.toUtf8());
    }
    return false;
}

JSObject::JSObject()
{

}

} //Internal

} // namespace QmlDesigner

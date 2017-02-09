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

#include <memory>
#include <mutex>

namespace QmlDesigner {

namespace Internal {

static std::once_flag s_singletonFlag;
static std::unique_ptr<QJSEngine> s_qJSEngine;
static JSObject *s_jsObject = nullptr;

static QVariant evaluateExpression(const QString &expression, const ModelNode &modelNode, const ModelNode &otherNode)
{
    std::call_once(s_singletonFlag, []() {
        s_qJSEngine.reset(new QJSEngine);
        s_jsObject = new JSObject(s_qJSEngine.get());
        QJSValue jsValue = s_qJSEngine->newQObject(s_jsObject);
        s_qJSEngine->globalObject().setProperty("model", jsValue);
    });

    s_jsObject->setModelNode(modelNode);
    s_jsObject->setOtherNode(otherNode);

    QJSValue value = s_qJSEngine->evaluate(expression);

    if (value.isError())
        return expression;
    return s_qJSEngine->evaluate(expression).toVariant();
}

} //Internal

QmlDesigner::NodeHints::NodeHints(const ModelNode &node) : m_modelNode(node)
{
    if (isValid()) {
        const ItemLibraryInfo *libraryInfo = model()->metaInfo().itemLibraryInfo();
        QList <ItemLibraryEntry> itemLibraryEntryList = libraryInfo->entriesForType(
                    modelNode().type(), modelNode().majorVersion(), modelNode().minorVersion());

        if (!itemLibraryEntryList.isEmpty())
            m_hints = itemLibraryEntryList.first().hints();
    }
}

NodeHints::NodeHints(const ItemLibraryEntry &entry)
{
    m_hints = entry.hints();
}

bool NodeHints::canBeContainerFor(const ModelNode &potenialChild) const
{
    /* The default is true for now to avoid confusion. Once our .metaInfo files in Qt
       use the feature we can change the default to false. */

    if (!isValid())
        return true;

    return evaluateBooleanExpression("canBeContainer", true, potenialChild);
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
    return evaluateBooleanExpression("canBeDroppedInFormEditor", true);
}

bool NodeHints::canBeDroppedInNavigator() const
{
    return evaluateBooleanExpression("canBeDroppedInNavigator", true);
}

bool NodeHints::isMovable() const
{
    if (!isValid())
        return true;

    return evaluateBooleanExpression("isMovable", true);
}

bool NodeHints::isResizable() const
{
    return evaluateBooleanExpression("isResizable", true);
}

bool NodeHints::isStackedContainer() const
{
    if (!isValid())
        return false;

    return evaluateBooleanExpression("isStackedContainer", false);
}

bool NodeHints::canBeReparentedTo(const ModelNode &potenialParent)
{
    if (!isValid())
        return true;

    return evaluateBooleanExpression("canBeReparented", true, potenialParent);
}

QString NodeHints::indexPropertyForStackedContainer() const
{
    if (!isValid())
        return QString();

    const QString expression = m_hints.value("indexPropertyForStackedContainer");

    if (expression.isEmpty())
        return QString();

    return Internal::evaluateExpression(expression, modelNode(), ModelNode()).toString();
}

QHash<QString, QString> NodeHints::hints() const
{
    return m_hints;
}

NodeHints NodeHints::fromModelNode(const ModelNode &modelNode)
{
    return NodeHints(modelNode);
}

NodeHints NodeHints::fromItemLibraryEntry(const ItemLibraryEntry &entry)
{
    return NodeHints(entry);
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

bool NodeHints::evaluateBooleanExpression(const QString &hintName, bool defaultValue, const ModelNode otherNode) const
{
    const QString expression = m_hints.value(hintName);

    if (expression.isEmpty())
        return defaultValue;

    return Internal::evaluateExpression(expression, modelNode(), otherNode).toBool();
}

namespace Internal {

JSObject::JSObject(QObject *parent) : QObject (parent)
{

}

void JSObject::setModelNode(const ModelNode &node)
{
    m_modelNode = node;
    emit modelNodeChanged();
}

void JSObject::setOtherNode(const ModelNode &node)
{
    m_otherNode = node;
    emit otherNodeChanged();
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

bool JSObject::potentialParentIsRoot() const
{
    return m_otherNode.isValid() && m_otherNode.isRootNode();
}

bool JSObject::potentialChildIsRoot() const
{
     return m_otherNode.isValid() && m_otherNode.isRootNode();
}

bool JSObject::isSubclassOf(const QString &typeName)
{
    NodeMetaInfo metaInfo = m_modelNode.metaInfo();

    if (metaInfo.isValid())
        return metaInfo.isSubclassOf(typeName.toUtf8());

    return false;
}

bool JSObject::rootItemIsSubclassOf(const QString &typeName)
{
    NodeMetaInfo metaInfo = m_modelNode.view()->rootModelNode().metaInfo();

    if (metaInfo.isValid())
        return metaInfo.isSubclassOf(typeName.toUtf8());

    return false;
}

bool JSObject::currentParentIsSubclassOf(const QString &typeName)
{
    if (m_modelNode.hasParentProperty()
         && m_modelNode.parentProperty().isValid()) {
        NodeMetaInfo metaInfo =  m_modelNode.parentProperty().parentModelNode().metaInfo();
        if (metaInfo.isValid())
            return metaInfo.isSubclassOf(typeName.toUtf8());
    }
    return false;
}

bool JSObject::potentialParentIsSubclassOf(const QString &typeName)
{
    NodeMetaInfo metaInfo = m_otherNode.metaInfo();

    if (metaInfo.isValid())
        return metaInfo.isSubclassOf(typeName.toUtf8());

    return false;
}

bool JSObject::potentialChildIsSubclassOf(const QString &typeName)
{
    NodeMetaInfo metaInfo = m_otherNode.metaInfo();

    if (metaInfo.isValid())
        return metaInfo.isSubclassOf(typeName.toUtf8());

    return false;
}

} //Internal

} // namespace QmlDesigner

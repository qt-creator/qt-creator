/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <QtCore/QDebug>

#include "nodeabstractproperty.h"
#include "rewriteaction.h"

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;
using namespace QmlDesigner;

static inline QString toString(QmlRefactoring::PropertyType type)
{
    switch (type) {
    case QmlRefactoring::ArrayBinding: return QLatin1String("array binding");
    case QmlRefactoring::ObjectBinding: return QLatin1String("object binding");
    case QmlRefactoring::ScriptBinding: return QLatin1String("script binding");
    default: return QLatin1String("UNKNOWN");
    }
}

bool AddPropertyRewriteAction::execute(QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_property.parentModelNode());
    bool result = false;

    if (m_property.isDefaultProperty())
        result = refactoring.addToObjectMemberList(nodeLocation, m_valueText);
    else
        result = refactoring.addProperty(nodeLocation, m_property.name(), m_valueText, m_propertyType);

    Q_ASSERT(result);
    return result;
}

void AddPropertyRewriteAction::dump(const QString &prefix) const
{
    qDebug() << qPrintable(prefix)
             << "AddPropertyRewriteAction for property"
             << m_property.name()
             << "(" << qPrintable(toString(m_propertyType)) << ")"
             ;
}

bool ChangeIdRewriteAction::execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_node);
    static const QLatin1String idPropertyName("id");
    bool result = false;

    if (m_oldId.isEmpty())
        result = refactoring.addProperty(nodeLocation, idPropertyName, m_newId, QmlRefactoring::ScriptBinding);
    else if (m_newId.isEmpty())
        result = refactoring.removeProperty(nodeLocation, idPropertyName);
    else
        result = refactoring.changeProperty(nodeLocation, idPropertyName, m_newId, QmlRefactoring::ScriptBinding);

    Q_ASSERT(result);
    return result;
}

void ChangeIdRewriteAction::dump(const QString &prefix) const
{
    qDebug() << qPrintable(prefix)
             << "ChangeIdRewriteAction"
             ;
}

bool ChangePropertyRewriteAction::execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_property.parentModelNode());
    bool result = false;

    if (m_property.isDefaultProperty())
        result = refactoring.addToObjectMemberList(nodeLocation, m_valueText);
    else if (m_propertyType == QmlRefactoring::ArrayBinding)
        result = refactoring.addToArrayMemberList(nodeLocation, m_property.name(), m_valueText);
    else
        result = refactoring.changeProperty(nodeLocation, m_property.name(), m_valueText, m_propertyType);

    Q_ASSERT(result);
    return result;
}

void ChangePropertyRewriteAction::dump(const QString &prefix) const
{
    qDebug() << qPrintable(prefix)
             << "ChangePropertyRewriteAction for property"
             << m_property.name()
             << "(" << qPrintable(toString(m_propertyType)) << ")"
             << "of node"
             << (m_property.parentModelNode().isValid() ? m_property.parentModelNode().id() : "(invalid)")
             << "with new value"
             << QString(m_valueText).replace('\n', "\\n")
             << "and contained object"
             << (m_containedModelNode.isValid() ? m_containedModelNode.id() : "(none)")
             ;
}

bool ChangeTypeRewriteAction::execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_node);
    bool result = false;

    QString newNodeType = m_node.type();
    const int slashIdx = newNodeType.lastIndexOf('/');
    if (slashIdx != -1)
        newNodeType = newNodeType.mid(slashIdx + 1);

    result = refactoring.changeObjectType(nodeLocation, newNodeType);

    Q_ASSERT(result);
    return result;
}

void ChangeTypeRewriteAction::dump(const QString &prefix) const
{
    qDebug() << qPrintable(prefix)
             << "ChangeTypeRewriteAction"
             ;
}

bool RemoveNodeRewriteAction::execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_node);
    bool result = false;

    result = refactoring.removeObject(nodeLocation);

    Q_ASSERT(result);
    return result;
}

void RemoveNodeRewriteAction::dump(const QString &prefix) const
{
    qDebug() << qPrintable(prefix)
             << "RemoveNodeRewriteAction"
             ;
}

bool RemovePropertyRewriteAction::execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_property.parentModelNode());
    bool result = false;

    result = refactoring.removeProperty(nodeLocation, m_property.name());

    Q_ASSERT(result);
    return result;
}

void RemovePropertyRewriteAction::dump(const QString &prefix) const
{
    qDebug() << qPrintable(prefix)
             << "RemovePropertyRewriteAction for property"
             << m_property.name()
             ;
}

bool ReparentNodeRewriteAction::execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_node);
    const int targetParentObjectLocation = positionStore.nodeOffset(m_targetProperty.parentModelNode());
    const bool isArrayBinding = m_targetProperty.isNodeListProperty();
    bool result = false;

    QString targetPropertyName;
    if (!m_targetProperty.isDefaultProperty())
        targetPropertyName = m_targetProperty.name();

    result = refactoring.moveObject(nodeLocation, targetPropertyName, isArrayBinding, targetParentObjectLocation);

    Q_ASSERT(result);
    return result;
}

void ReparentNodeRewriteAction::dump(const QString &prefix) const
{
    if (m_node.isValid()) {
        qDebug() << qPrintable(prefix)
                 << "ReparentNodeRewriteAction for node"
                 << m_node.id()
                 << "into property"
                 << m_targetProperty.name()
                 << "of node"
                 << m_targetProperty.parentModelNode().id()
                 ;
    } else {
        qDebug() << qPrintable(prefix)
                 << "reparentNodeRewriteAction for an invalid node"
                 ;
    }
}

bool MoveNodeRewriteAction::execute(QmlRefactoring &refactoring,
                                    ModelNodePositionStorage &positionStore)
{
    const int movingNodeLocation = positionStore.nodeOffset(m_movingNode);
    const int newTrailingNodeLocation = m_newTrailingNode.isValid() ? positionStore.nodeOffset(m_newTrailingNode) : -1;
    bool result = false;

    result = refactoring.moveObjectBeforeObject(movingNodeLocation, newTrailingNodeLocation);

    Q_ASSERT(result);
    return result;
}

void MoveNodeRewriteAction::dump(const QString &prefix) const
{
    if (m_movingNode.isValid()) {
        if (m_newTrailingNode.isValid())
            qDebug() << qPrintable(prefix)
                     << "MoveNodeRewriteAction for node"
                     << m_movingNode.id()
                     << "before node"
                     << m_newTrailingNode.id()
                     ;
        else
            qDebug() << qPrintable(prefix)
                     << "MoveNodeRewriteAction for node"
                     << m_movingNode.id()
                     << "to the end of its containing property"
                     ;
    } else {
        qDebug() << qPrintable(prefix)
                 << "MoveNodeRewriteAction for an invalid node"
                 ;
    }
}

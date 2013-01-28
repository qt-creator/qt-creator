/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <QDebug>

#include "nodeabstractproperty.h"
#include "nodelistproperty.h"
#include "nodemetainfo.h"
#include "rewriteaction.h"

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;
using namespace QmlDesigner;

namespace { // anonymous

static inline QString toInfo(const Import &import)
{
    QString txt;

    if (import.isEmpty()) {
        return QLatin1String("empty import");
    } else if (import.isFileImport()) {
        txt = QLatin1String("import file \"%1\"");
        txt = txt.arg(import.url());
    } else if (import.isLibraryImport()) {
        txt = QLatin1String("import library \"%1\"");
        txt = txt.arg(import.file());
    } else {
        return QLatin1String("unknown type of import");
    }

    if (import.hasVersion())
        txt += QString::fromLatin1("with version \"%1\"").arg(import.version());
    else
        txt += QLatin1String("without version");

    if (import.hasAlias())
        txt += QString::fromLatin1("aliassed as \"%1\"").arg(import.alias());
    else
        txt += QLatin1String("unaliassed");

    return txt;
}

static inline QString toString(QmlRefactoring::PropertyType type)
{
    switch (type) {
        case QmlRefactoring::ArrayBinding:  return QLatin1String("array binding");
        case QmlRefactoring::ObjectBinding: return QLatin1String("object binding");
        case QmlRefactoring::ScriptBinding: return QLatin1String("script binding");
        default:                            return QLatin1String("UNKNOWN");
    }
}

} // namespace anonymous

bool AddPropertyRewriteAction::execute(QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_property.parentModelNode());
    bool result = false;

    if (m_property.isDefaultProperty()) {
        result = refactoring.addToObjectMemberList(nodeLocation, m_valueText);

        if (!result) {
            qDebug() << "*** AddPropertyRewriteAction::execute failed in addToObjectMemberList("
                    << nodeLocation << ','
                    << m_valueText << ") **"
                    << info();
        }
    } else if (m_property.isNodeListProperty() && m_property.toNodeListProperty().toModelNodeList().size() > 1) {
        result = refactoring.addToArrayMemberList(nodeLocation, m_property.name(), m_valueText);

        if (!result) {
            qDebug() << "*** AddPropertyRewriteAction::execute failed in addToArrayMemberList("
                    << nodeLocation << ','
                    << m_property.name() << ','
                    << m_valueText << ") **"
                    << info();
        }
    } else {
        result = refactoring.addProperty(nodeLocation, m_property.name(), m_valueText, m_propertyType);

        if (!result) {
            qDebug() << "*** AddPropertyRewriteAction::execute failed in addProperty("
                    << nodeLocation << ','
                    << m_property.name() << ','
                    << m_valueText << ","
                    << qPrintable(toString(m_propertyType)) << ") **"
                    << info();
        }
    }

    return result;
}

QString AddPropertyRewriteAction::info() const
{
    return QString("AddPropertyRewriteAction for property \"%1\" (type: %2)").arg(m_property.name(), toString(m_propertyType));
}

bool ChangeIdRewriteAction::execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_node);
    static const QLatin1String idPropertyName("id");
    bool result = false;

    if (m_oldId.isEmpty()) {
        result = refactoring.addProperty(nodeLocation, idPropertyName, m_newId, QmlRefactoring::ScriptBinding);

        if (!result) {
            qDebug() << "*** ChangeIdRewriteAction::execute failed in addProperty("
                    << nodeLocation << ','
                    << idPropertyName << ','
                    << m_newId << ", ScriptBinding) **"
                    << info();
        }
    } else if (m_newId.isEmpty()) {
        result = refactoring.removeProperty(nodeLocation, idPropertyName);

        if (!result) {
            qDebug() << "*** ChangeIdRewriteAction::execute failed in removeProperty("
                    << nodeLocation << ','
                    << idPropertyName << ") **"
                    << info();
        }
    } else {
        result = refactoring.changeProperty(nodeLocation, idPropertyName, m_newId, QmlRefactoring::ScriptBinding);

        if (!result) {
            qDebug() << "*** ChangeIdRewriteAction::execute failed in changeProperty("
                    << nodeLocation << ','
                    << idPropertyName << ','
                    << m_newId << ", ScriptBinding) **"
                    << info();
        }
    }

    return result;
}

QString ChangeIdRewriteAction::info() const
{
    return QString("ChangeIdRewriteAction from \"%1\" to \"%2\"").arg(m_oldId, m_newId);
}

bool ChangePropertyRewriteAction::execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_property.parentModelNode());
    bool result = false;

    if (m_property.isDefaultProperty()) {
        result = refactoring.addToObjectMemberList(nodeLocation, m_valueText);

        if (!result) {
            qDebug() << "*** ChangePropertyRewriteAction::execute failed in addToObjectMemberList("
                    << nodeLocation << ','
                    << m_valueText << ") **"
                    << info();
        }
    } else if (m_propertyType == QmlRefactoring::ArrayBinding) {
        result = refactoring.addToArrayMemberList(nodeLocation, m_property.name(), m_valueText);

        if (!result) {
            qDebug() << "*** ChangePropertyRewriteAction::execute failed in addToArrayMemberList("
                    << nodeLocation << ','
                    << m_property.name() << ','
                    << m_valueText << ") **"
                    << info();
        }
    } else {
        result = refactoring.changeProperty(nodeLocation, m_property.name(), m_valueText, m_propertyType);

        if (!result) {
            qDebug() << "*** ChangePropertyRewriteAction::execute failed in changeProperty("
                    << nodeLocation << ','
                    << m_property.name() << ','
                    << m_valueText << ','
                    << qPrintable(toString(m_propertyType)) << ") **"
                    << info();
        }
    }

    return result;
}

QString ChangePropertyRewriteAction::info() const
{
    return QString("ChangePropertyRewriteAction for property \"%1\" (type: %2) of node \"%3\" with value >>%4<< and contained object \"%5\"")
             .arg(m_property.name(),
                  toString(m_propertyType),
                  (m_property.parentModelNode().isValid() ? m_property.parentModelNode().id() : "(invalid)"),
                  QString(m_valueText).replace('\n', "\\n"),
                  (m_containedModelNode.isValid() ? m_containedModelNode.id() : "(none)"));
}

bool ChangeTypeRewriteAction::execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_node);
    bool result = false;

    QString newNodeType = m_node.convertTypeToImportAlias();
    const int slashIdx = newNodeType.lastIndexOf('.');
    if (slashIdx != -1)
        newNodeType = newNodeType.mid(slashIdx + 1);

    result = refactoring.changeObjectType(nodeLocation, newNodeType);
    if (!result) {
        qDebug() << "*** ChangeTypeRewriteAction::execute failed in changeObjectType("
                << nodeLocation << ','
                << newNodeType << ") **"
                << info();
    }

    return result;
}

QString ChangeTypeRewriteAction::info() const
{
    return QString("ChangeTypeRewriteAction");
}

bool RemoveNodeRewriteAction::execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_node);
    bool result = refactoring.removeObject(nodeLocation);
    if (!result) {
        qDebug() << "*** RemoveNodeRewriteAction::execute failed in removeObject("
                << nodeLocation << ") **"
                << info();
    }

    return result;
}

QString RemoveNodeRewriteAction::info() const
{
    return QString("RemoveNodeRewriteAction");
}

bool RemovePropertyRewriteAction::execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_property.parentModelNode());
    bool result = refactoring.removeProperty(nodeLocation, m_property.name());
    if (!result) {
        qDebug() << "*** RemovePropertyRewriteAction::execute failed in removeProperty("
                << nodeLocation << ','
                << m_property.name() << ") **"
                << info();
    }

    return result;
}

QString RemovePropertyRewriteAction::info() const
{
    return QString("RemovePropertyRewriteAction for property \"%1\"").arg(m_property.name());
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
    if (!result) {
        qDebug() << "*** ReparentNodeRewriteAction::execute failed in moveObject("
                << nodeLocation << ','
                << targetPropertyName << ','
                << isArrayBinding << ','
                << targetParentObjectLocation << ") **"
                << info();
    }

    return result;
}

QString ReparentNodeRewriteAction::info() const
{
    if (m_node.isValid())
        return QString("ReparentNodeRewriteAction for node \"%1\" into property \"%2\" of node \"%3\"")
                .arg(m_node.id(),
                     m_targetProperty.name(),
                     m_targetProperty.parentModelNode().id());
    else
        return QString("ReparentNodeRewriteAction for an invalid node");
}

bool MoveNodeRewriteAction::execute(QmlRefactoring &refactoring,
                                    ModelNodePositionStorage &positionStore)
{
    const int movingNodeLocation = positionStore.nodeOffset(m_movingNode);
    const int newTrailingNodeLocation = m_newTrailingNode.isValid() ? positionStore.nodeOffset(m_newTrailingNode) : -1;
    bool result = false;

    bool inDefaultProperty = (m_movingNode.parentProperty().parentModelNode().metaInfo().defaultPropertyName() == m_movingNode.parentProperty().name());

    result = refactoring.moveObjectBeforeObject(movingNodeLocation, newTrailingNodeLocation, inDefaultProperty);
    if (!result) {
        qDebug() << "*** MoveNodeRewriteAction::execute failed in moveObjectBeforeObject("
                << movingNodeLocation << ','
                << newTrailingNodeLocation << ") **"
                << info();
    }

    return result;
}

QString MoveNodeRewriteAction::info() const
{
    if (m_movingNode.isValid()) {
        if (m_newTrailingNode.isValid())
            return QString("MoveNodeRewriteAction for node \"%1\" before node \"%2\"").arg(m_movingNode.id(), m_newTrailingNode.id());
        else
            return QString("MoveNodeRewriteAction for node \"%1\" to the end of its containing property").arg(m_movingNode.id());
    } else {
        return QString("MoveNodeRewriteAction for an invalid node");
    }
}

bool AddImportRewriteAction::execute(QmlDesigner::QmlRefactoring &refactoring,
                                     ModelNodePositionStorage &/*positionStore*/)
{
    const bool result = refactoring.addImport(m_import);

    if (!result)
        qDebug() << "*** AddImportRewriteAction::execute failed in changeImports ("
                 << m_import.toString()
                 << ") **"
                 << info();
    return result;
}

QString AddImportRewriteAction::info() const
{
    return toInfo(m_import);
}

bool RemoveImportRewriteAction::execute(QmlDesigner::QmlRefactoring &refactoring,
                                        ModelNodePositionStorage &/*positionStore*/)
{
    const bool result = refactoring.removeImport(m_import);

    if (!result)
        qDebug() << "*** RemoveImportRewriteAction::execute failed in changeImports ("
                 << m_import.toString()
                 << ") **"
                 << info();
    return result;
}

QString RemoveImportRewriteAction::info() const
{
    return toInfo(m_import);
}

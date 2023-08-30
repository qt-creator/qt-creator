// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rewriteaction.h"

#include <QDebug>

#include "nodelistproperty.h"
#include "nodemetainfo.h"

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;
using namespace QmlDesigner;
using namespace Qt::StringLiterals;

namespace { // anonymous

QString toInfo(const Import &import)
{
    QString txt;

    if (import.isEmpty()) {
        return QStringLiteral("empty import");
    } else if (import.isFileImport()) {
        txt = QStringView(u"import file \"%1\"").arg(import.url());
    } else if (import.isLibraryImport()) {
        txt = QStringView(u"import library \"%1\"").arg(import.file());
    } else {
        return QStringLiteral("unknown type of import");
    }

    if (import.hasVersion())
        txt += QStringView(u"with version \"%1\"").arg(import.version());
    else
        txt += QStringView(u"without version");

    if (import.hasAlias())
        txt += QStringView(u"aliassed as \"%1\"").arg(import.alias());
    else
        txt += QStringView(u"unaliassed");

    return txt;
}

QStringView toString(QmlRefactoring::PropertyType type)
{
    switch (type) {
    case QmlRefactoring::ArrayBinding:
        return QStringView(u"array binding");
    case QmlRefactoring::ObjectBinding:
        return QStringView(u"object binding");
    case QmlRefactoring::ScriptBinding:
        return QStringView(u"script binding");
    default:
        return QStringView(u"UNKNOWN");
    }
}

} // namespace anonymous

bool AddPropertyRewriteAction::execute(QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    if (m_sheduledInHierarchy) {
        const int nodeLocation = positionStore.nodeOffset(m_property.parentModelNode());
        bool result = false;

        if (m_propertyType != QmlRefactoring::ScriptBinding && m_property.isDefaultProperty()) {
            result = refactoring.addToObjectMemberList(nodeLocation, m_valueText);

            if (!result) {
                qDebug() << "*** AddPropertyRewriteAction::execute failed in addToObjectMemberList("
                         << nodeLocation << ','
                         << m_valueText << ") **"
                         << info();
            }
        } else if (m_property.isNodeListProperty() && m_property.toNodeListProperty().count() > 1) {
            result = refactoring.addToArrayMemberList(nodeLocation, m_property.name(), m_valueText);

            if (!result) {
                qDebug() << "*** AddPropertyRewriteAction::execute failed in addToArrayMemberList("
                         << nodeLocation << ','
                         << m_property.name() << ','
                         << m_valueText << ") **"
                         << info();
            }
        } else {
            result = refactoring.addProperty(nodeLocation, m_property.name(), m_valueText, m_propertyType, m_property.dynamicTypeName());

            if (!result) {
                qDebug() << "*** AddPropertyRewriteAction::execute failed in addProperty("
                         << nodeLocation << ',' << m_property.name() << ',' << m_valueText << ","
                         << toString(m_propertyType) << ") **" << info();
            }
        }

        return result;
    } else {
        return true;
    }
}

QString AddPropertyRewriteAction::info() const
{
    return QStringView(u"AddPropertyRewriteAction for property \"%1\" (type: %2) of node \"%3\" "
                       u"with value >>%4<< and contained object \"%5\"")
        .arg(QString::fromUtf8(m_property.name()),
             toString(m_propertyType),
             (m_property.parentModelNode().isValid() ? m_property.parentModelNode().id()
                                                     : QLatin1String("(invalid)")),
             QString(m_valueText).replace(QLatin1Char('\n'), QLatin1String("\\n")),
             (m_containedModelNode.isValid() ? m_containedModelNode.id()
                                             : QString(QStringLiteral("(none)"))));
}

bool ChangeIdRewriteAction::execute(QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_node);
    static const PropertyName idPropertyName("id");
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
    return QStringView(u"ChangeIdRewriteAction from \"%1\" to \"%2\"").arg(m_oldId, m_newId);
}

bool ChangePropertyRewriteAction::execute(QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    if (m_sheduledInHierarchy) {
        const int nodeLocation = positionStore.nodeOffset(m_property.parentModelNode());
        if (nodeLocation < 0) {
            qWarning() << "*** ChangePropertyRewriteAction::execute ignored. Invalid node location";
            return true;
        }
        bool result = false;

        if (m_propertyType != QmlRefactoring::ScriptBinding && m_property.isDefaultProperty()) {
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
                         << nodeLocation << ',' << m_property.name() << ',' << m_valueText << ','
                         << toString(m_propertyType) << ") **" << info();
            }
        }

        return result;
    } else {
        return true;
    }
}

QString ChangePropertyRewriteAction::info() const
{
    return QStringView(u"ChangePropertyRewriteAction for property \"%1\" (type: %2) of node \"%3\" "
                       u"with value >>%4<< and contained object \"%5\"")
        .arg(QString::fromUtf8(m_property.name()),
             toString(m_propertyType),
             (m_property.parentModelNode().isValid() ? m_property.parentModelNode().id()
                                                     : QLatin1String("(invalid)")),
             QString(m_valueText).replace(QLatin1Char('\n'), QLatin1String("\\n")),
             (m_containedModelNode.isValid() ? m_containedModelNode.id()
                                             : QString(QStringLiteral("(none)"))));
}

bool ChangeTypeRewriteAction::execute(QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_node);
    bool result = false;

    QString newNodeType = m_node.convertTypeToImportAlias();
    const int slashIdx = newNodeType.lastIndexOf(u'.');
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
    return QLatin1String("ChangeTypeRewriteAction");
}

bool RemoveNodeRewriteAction::execute(QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
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
    return QLatin1String("RemoveNodeRewriteAction") + QString::number(m_node.internalId());
}

bool RemovePropertyRewriteAction::execute(QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
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
    return QStringView(u"RemovePropertyRewriteAction for property \"%1\"")
        .arg(QString::fromUtf8(m_property.name()));
}

bool ReparentNodeRewriteAction::execute(QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore)
{
    const int nodeLocation = positionStore.nodeOffset(m_node);
    const int targetParentObjectLocation = positionStore.nodeOffset(m_targetProperty.parentModelNode());
    const bool isArrayBinding = m_targetProperty.isNodeListProperty();
    bool result = false;

    PropertyName targetPropertyName;
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
        return QStringView(
                   u"ReparentNodeRewriteAction for node \"%1\" into property \"%2\" of node \"%3\"")
            .arg(m_node.id(),
                 QString::fromUtf8(m_targetProperty.name()),
                 m_targetProperty.parentModelNode().id());
    else
        return "ReparentNodeRewriteAction for an invalid node"_L1;
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
            return QStringView(u"MoveNodeRewriteAction for node \"%1\" before node \"%2\"")
                .arg(m_movingNode.id(), m_newTrailingNode.id());
        else
            return QStringView(u"MoveNodeRewriteAction for node \"%1\" to the end of its "
                               u"containing property")
                .arg(m_movingNode.id());
    } else {
        return "MoveNodeRewriteAction for an invalid node"_L1;
    }
}

bool AddImportRewriteAction::execute(QmlRefactoring &refactoring,
                                     ModelNodePositionStorage &/*positionStore*/)
{
    const bool result = refactoring.addImport(m_import);

    if (!result)
        qDebug() << "*** AddImportRewriteAction::execute failed in changeImports ("
                 << m_import.toImportString()
                 << ") **"
                 << info();
    return result;
}

QString AddImportRewriteAction::info() const
{
    return toInfo(m_import);
}

bool RemoveImportRewriteAction::execute(QmlRefactoring &refactoring,
                                        ModelNodePositionStorage &/*positionStore*/)
{
    const bool result = refactoring.removeImport(m_import);

    if (!result)
        qDebug() << "*** RemoveImportRewriteAction::execute failed in changeImports ("
                 << m_import.toImportString()
                 << ") **"
                 << info();
    return result;
}

QString RemoveImportRewriteAction::info() const
{
    return toInfo(m_import);
}

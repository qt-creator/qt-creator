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

#ifndef DESIGNERNODE_H
#define DESIGNERNODE_H

#include "qmldesignercorelib_global.h"
#include <QWeakPointer>
#include <QList>
#include <QMetaType>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {
    class InternalNode;
    class ModelPrivate;
    class InternalNode;
    class InternalProperty;

    typedef QSharedPointer<InternalNode> InternalNodePointer;
    typedef QSharedPointer<InternalProperty> InternalPropertyPointer;
    typedef QWeakPointer<InternalNode> InternalNodeWeakPointer;
}
class NodeMetaInfo;
class AbstractProperty;
class BindingProperty;
class VariantProperty;
class Model;
class AbstractView;
class NodeListProperty;
class NodeProperty;
class NodeAbstractProperty;
class ModelNode;

QMLDESIGNERCORE_EXPORT QList<Internal::InternalNodePointer> toInternalNodeList(const QList<ModelNode> &nodeList);

typedef QList<QPair<QString, QVariant> > PropertyListType;

class QMLDESIGNERCORE_EXPORT  ModelNode
{
    friend QMLDESIGNERCORE_EXPORT bool operator ==(const ModelNode &firstNode, const ModelNode &secondNode);
    friend QMLDESIGNERCORE_EXPORT bool operator !=(const ModelNode &firstNode, const ModelNode &secondNode);
    friend QMLDESIGNERCORE_EXPORT uint qHash(const ModelNode & node);
    friend QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const ModelNode &modelNode);
    friend QMLDESIGNERCORE_EXPORT bool operator <(const ModelNode &firstNode, const ModelNode &secondNode);
    friend QMLDESIGNERCORE_EXPORT QList<Internal::InternalNodePointer> toInternalNodeList(const QList<ModelNode> &nodeList);
    friend class QmlDesigner::Model;
    friend class QmlDesigner::AbstractView;
    friend class QmlDesigner::NodeListProperty;
    friend class QmlDesigner::Internal::ModelPrivate;
    friend class QmlDesigner::NodeAbstractProperty;
    friend class QmlDesigner::NodeProperty;

public:
    enum NodeSourceType {
        NodeWithoutSource = 0,
        NodeWithCustomParserSource = 1,
        NodeWithComponentSource = 2
    };

    ModelNode();
    ModelNode(const Internal::InternalNodePointer &internalNode, Model *model, AbstractView *view);
    ModelNode(const ModelNode modelNode, AbstractView *view);
    ModelNode(const ModelNode &other);
    ~ModelNode();

    ModelNode& operator=(const ModelNode &other);
    QString type() const;
    QString simplifiedTypeName() const;
    int minorVersion() const;
    int majorVersion() const;
    int majorQtQuickVersion() const;

    bool isValid() const;
    bool isInHierarchy() const;


    NodeAbstractProperty parentProperty() const;
    void setParentProperty(NodeAbstractProperty parent);
    void setParentProperty(const ModelNode &newParentNode, const QString &propertyName);
    bool hasParentProperty() const;

    const QList<ModelNode> allDirectSubModelNodes() const;
    const QList<ModelNode> allSubModelNodes() const;
    bool hasAnySubModelNodes() const;

    //###

    AbstractProperty property(const QString &name) const;
    VariantProperty variantProperty(const QString &name) const;
    BindingProperty bindingProperty(const QString &name) const;
    NodeListProperty nodeListProperty(const QString &name) const;
    NodeProperty nodeProperty(const QString &name) const;
    NodeAbstractProperty nodeAbstractProperty(const QString &name) const;

    void removeProperty(const QString &name); //### also implement in AbstractProperty
    QList<AbstractProperty> properties() const;
    QList<VariantProperty> variantProperties() const;
    QList<NodeAbstractProperty> nodeAbstractProperties() const;
    QList<NodeProperty> nodeProperties() const;
    QList<NodeListProperty> nodeListProperties() const;
    QList<BindingProperty> bindingProperties() const;
    QStringList propertyNames() const;

    bool hasProperties() const;
    bool hasProperty(const QString &name) const;
    bool hasVariantProperty(const QString &name) const;
    bool hasBindingProperty(const QString &name) const;
    bool hasNodeAbstracProperty(const QString &name) const;
    bool hasNodeProperty(const QString &name) const;
    bool hasNodeListProperty(const QString &name) const;


    void setScriptFunctions(const QStringList &scriptFunctionList);
    QStringList scriptFunctions() const;

    //###
    void destroy();

    QString id() const;
    QString validId();
    void setId(const QString &id);
    static bool isValidId(const QString &id);

    Model *model() const;
    AbstractView *view() const;

    const NodeMetaInfo metaInfo() const;

    bool isSelected() const;
    bool isRootNode() const;

    bool isAncestorOf(const ModelNode &node) const;
    void selectNode();
    void deselectNode();

    static int variantUserType();
    QVariant toVariant() const;

    QVariant auxiliaryData(const QString &name) const;
    void setAuxiliaryData(const QString &name, const QVariant &data) const;
    bool hasAuxiliaryData(const QString &name) const;
    QHash<QString, QVariant> auxiliaryData() const;

    qint32 internalId() const;

    void setNodeSource(const QString&);
    QString nodeSource() const;

    QString convertTypeToImportAlias() const;

    NodeSourceType nodeSourceType() const;

private: // functions
    Internal::InternalNodePointer internalNode() const;
    QString generateNewId() const;

private: // variables
    Internal::InternalNodePointer m_internalNode;
    QWeakPointer<Model> m_model;
    QWeakPointer<AbstractView> m_view;
};

QMLDESIGNERCORE_EXPORT bool operator ==(const ModelNode &firstNode, const ModelNode &secondNode);
QMLDESIGNERCORE_EXPORT bool operator !=(const ModelNode &firstNode, const ModelNode &secondNode);
QMLDESIGNERCORE_EXPORT uint qHash(const ModelNode & node);
QMLDESIGNERCORE_EXPORT bool operator <(const ModelNode &firstNode, const ModelNode &secondNode);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const ModelNode &modelNode);
QMLDESIGNERCORE_EXPORT QTextStream& operator<<(QTextStream &stream, const ModelNode &modelNode);
}

Q_DECLARE_METATYPE(QmlDesigner::ModelNode)
Q_DECLARE_METATYPE(QList<QmlDesigner::ModelNode>)

#endif // DESIGNERNODE_H

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

#ifndef DESIGNERNODE_H
#define DESIGNERNODE_H

#include "corelib_global.h"
#include <QWeakPointer>
#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QVariant>

class QTextStream;

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
class NodeInstance;
class ModelNode;

CORESHARED_EXPORT QList<Internal::InternalNodePointer> toInternalNodeList(const QList<ModelNode> &nodeList);

typedef QList<QPair<QString, QVariant> > PropertyListType;

class CORESHARED_EXPORT ModelNode
{
    friend CORESHARED_EXPORT bool operator ==(const ModelNode &firstNode, const ModelNode &secondNode);
    friend CORESHARED_EXPORT bool operator !=(const ModelNode &firstNode, const ModelNode &secondNode);
    friend CORESHARED_EXPORT uint qHash(const ModelNode & node);
    friend CORESHARED_EXPORT QDebug operator<<(QDebug debug, const ModelNode &modelNode);
    friend CORESHARED_EXPORT bool operator <(const ModelNode &firstNode, const ModelNode &secondNode);
    friend CORESHARED_EXPORT QList<Internal::InternalNodePointer> QmlDesigner::toInternalNodeList(const QList<ModelNode> &nodeList);
    friend class QmlDesigner::Model;
    friend class QmlDesigner::AbstractView;
    friend class QmlDesigner::NodeListProperty;
    friend class QmlDesigner::Internal::ModelPrivate;
    friend class QmlDesigner::NodeAbstractProperty;
    friend class QmlDesigner::NodeProperty;

public:
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
    void setAuxiliaryData(const QString &name, const QVariant &data);
    bool hasAuxiliaryData(const QString &name) const;

private: // functions
    Internal::InternalNodePointer internalNode() const;
    QString generateNewId() const;

private: // variables
    Internal::InternalNodePointer m_internalNode;
    QWeakPointer<Model> m_model;
    QWeakPointer<AbstractView> m_view;
};

CORESHARED_EXPORT bool operator ==(const ModelNode &firstNode, const ModelNode &secondNode);
CORESHARED_EXPORT bool operator !=(const ModelNode &firstNode, const ModelNode &secondNode);
CORESHARED_EXPORT uint qHash(const ModelNode & node);
CORESHARED_EXPORT bool operator <(const ModelNode &firstNode, const ModelNode &secondNode);
CORESHARED_EXPORT QDebug operator<<(QDebug debug, const ModelNode &modelNode);
CORESHARED_EXPORT QTextStream& operator<<(QTextStream &stream, const ModelNode &modelNode);
}

Q_DECLARE_METATYPE(QmlDesigner::ModelNode)
Q_DECLARE_METATYPE(QList<QmlDesigner::ModelNode>)

#endif // DESIGNERNODE_H

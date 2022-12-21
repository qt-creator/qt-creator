// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "auxiliarydata.h"
#include "abstractproperty.h"
#include "qmldesignercorelib_global.h"

#include <QPointer>
#include <QList>
#include <QVector>
#include <QVariant>

#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {
    class InternalNode;
    class ModelPrivate;
    class InternalNode;
    class InternalProperty;

    using InternalNodePointer = std::shared_ptr<InternalNode>;
    using InternalPropertyPointer = QSharedPointer<InternalProperty>;
}
class NodeMetaInfo;
class BindingProperty;
class VariantProperty;
class SignalHandlerProperty;
class SignalDeclarationProperty;
class Model;
class AbstractView;
class NodeListProperty;
class NodeProperty;
class NodeAbstractProperty;
class ModelNode;
class GlobalAnnotationStatus;
class Comment;
class Annotation;

QMLDESIGNERCORE_EXPORT QList<Internal::InternalNodePointer> toInternalNodeList(const QList<ModelNode> &nodeList);

using PropertyListType = QList<QPair<PropertyName, QVariant> >;
using AuxiliaryPropertyListType = QList<QPair<AuxiliaryDataKey, QVariant>>;

inline constexpr AuxiliaryDataKeyView lockedProperty{AuxiliaryDataType::Document, "locked"};
inline constexpr AuxiliaryDataKeyView timelineExpandedProperty{AuxiliaryDataType::Document,
                                                               "timeline_expanded"};
inline constexpr AuxiliaryDataKeyView transitionExpandedPropery{AuxiliaryDataType::Document,
                                                                "transition_expanded"};

class QMLDESIGNERCORE_EXPORT ModelNode
{
    friend QMLDESIGNERCORE_EXPORT bool operator ==(const ModelNode &firstNode, const ModelNode &secondNode);
    friend QMLDESIGNERCORE_EXPORT bool operator !=(const ModelNode &firstNode, const ModelNode &secondNode);
    friend QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const ModelNode &modelNode);
    friend QMLDESIGNERCORE_EXPORT bool operator <(const ModelNode &firstNode, const ModelNode &secondNode);
    friend QMLDESIGNERCORE_EXPORT QList<Internal::InternalNodePointer> toInternalNodeList(const QList<ModelNode> &nodeList);
    friend Model;
    friend AbstractView;
    friend NodeListProperty;
    friend Internal::ModelPrivate;
    friend NodeAbstractProperty;
    friend NodeProperty;

public:
    enum NodeSourceType {
        NodeWithoutSource = 0,
        NodeWithCustomParserSource = 1,
        NodeWithComponentSource = 2
    };

    ModelNode();
    ModelNode(const Internal::InternalNodePointer &internalNode, Model *model, const AbstractView *view);
    ModelNode(const ModelNode &modelNode, AbstractView *view);
    ~ModelNode();

    TypeName type() const;
    QString simplifiedTypeName() const;
    QString displayName() const;
    int minorVersion() const;
    int majorVersion() const;

    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    bool isInHierarchy() const;


    NodeAbstractProperty parentProperty() const;
    void setParentProperty(NodeAbstractProperty parent);
    void changeType(const TypeName &typeName, int majorVersion, int minorVersion);
    void setParentProperty(const ModelNode &newParentNode, const PropertyName &propertyName);
    bool hasParentProperty() const;

    QList<ModelNode> directSubModelNodes() const;
    QList<ModelNode> directSubModelNodesOfType(const NodeMetaInfo &type) const;
    QList<ModelNode> subModelNodesOfType(const NodeMetaInfo &type) const;

    QList<ModelNode> allSubModelNodes() const;
    QList<ModelNode> allSubModelNodesAndThisNode() const;
    bool hasAnySubModelNodes() const;

    //###

    AbstractProperty property(const PropertyName &name) const;
    VariantProperty variantProperty(const PropertyName &name) const;
    BindingProperty bindingProperty(const PropertyName &name) const;
    SignalHandlerProperty signalHandlerProperty(const PropertyName &name) const;
    SignalDeclarationProperty signalDeclarationProperty(const PropertyName &name) const;
    NodeListProperty nodeListProperty(const PropertyName &name) const;
    NodeProperty nodeProperty(const PropertyName &name) const;
    NodeAbstractProperty nodeAbstractProperty(const PropertyName &name) const;
    NodeAbstractProperty defaultNodeAbstractProperty() const;
    NodeListProperty defaultNodeListProperty() const;
    NodeProperty defaultNodeProperty() const;

    void removeProperty(const PropertyName &name) const; //### also implement in AbstractProperty
    QList<AbstractProperty> properties() const;
    QList<VariantProperty> variantProperties() const;
    QList<NodeAbstractProperty> nodeAbstractProperties() const;
    QList<NodeProperty> nodeProperties() const;
    QList<NodeListProperty> nodeListProperties() const;
    QList<BindingProperty> bindingProperties() const;
    QList<SignalHandlerProperty> signalProperties() const;
    QList<AbstractProperty> dynamicProperties() const;
    PropertyNameList propertyNames() const;

    bool hasProperties() const;
    bool hasProperty(const PropertyName &name) const;
    bool hasVariantProperty(const PropertyName &name) const;
    bool hasBindingProperty(const PropertyName &name) const;
    bool hasSignalHandlerProperty(const PropertyName &name) const;
    bool hasNodeAbstractProperty(const PropertyName &name) const;
    bool hasDefaultNodeAbstractProperty() const;
    bool hasDefaultNodeListProperty() const;
    bool hasDefaultNodeProperty() const;
    bool hasNodeProperty(const PropertyName &name) const;
    bool hasNodeListProperty(const PropertyName &name) const;

    void setScriptFunctions(const QStringList &scriptFunctionList);
    QStringList scriptFunctions() const;

    //###
    void destroy();

    QString id() const;
    QString validId();
    void setIdWithRefactoring(const QString &id);
    void setIdWithoutRefactoring(const QString &id);
    static bool isValidId(const QString &id);
    static QString getIdValidityErrorMessage(const QString &id);

    bool hasId() const;

    Model *model() const;
    AbstractView *view() const;

    NodeMetaInfo metaInfo() const;
    bool hasMetaInfo() const;

    bool isSelected() const;
    bool isRootNode() const;

    bool isAncestorOf(const ModelNode &node) const;
    void selectNode();
    void deselectNode();

    static int variantUserType();
    QVariant toVariant() const;

    std::optional<QVariant> auxiliaryData(AuxiliaryDataKeyView key) const;
    std::optional<QVariant> auxiliaryData(AuxiliaryDataType type, Utils::SmallStringView name) const;
    QVariant auxiliaryDataWithDefault(AuxiliaryDataType type, Utils::SmallStringView name) const;
    QVariant auxiliaryDataWithDefault(AuxiliaryDataKeyView key) const;
    QVariant auxiliaryDataWithDefault(AuxiliaryDataKeyDefaultValue key) const;
    void setAuxiliaryData(AuxiliaryDataKeyView key, const QVariant &data) const;
    void setAuxiliaryData(AuxiliaryDataType type, Utils::SmallStringView name, const QVariant &data) const;
    void setAuxiliaryDataWithoutLock(AuxiliaryDataType type,
                                     Utils::SmallStringView name,
                                     const QVariant &data) const;
    void removeAuxiliaryData(AuxiliaryDataKeyView key) const;
    void removeAuxiliaryData(AuxiliaryDataType type, Utils::SmallStringView name) const;
    bool hasAuxiliaryData(AuxiliaryDataKeyView key) const;
    bool hasAuxiliaryData(AuxiliaryDataType type, Utils::SmallStringView name) const;
    AuxiliaryDatasForType auxiliaryData(AuxiliaryDataType type) const;
    AuxiliaryDatasView auxiliaryData() const;

    QString customId() const;
    bool hasCustomId() const;
    void setCustomId(const QString &str);
    void removeCustomId();

    QVector<Comment> comments() const;
    bool hasComments() const;
    void setComments(const QVector<Comment> &coms);
    void addComment(const Comment &com);
    bool updateComment(const Comment &com, int position);

    Annotation annotation() const;
    bool hasAnnotation() const;
    void setAnnotation(const Annotation &annotation);
    void removeAnnotation();

    Annotation globalAnnotation() const;
    bool hasGlobalAnnotation() const;
    void setGlobalAnnotation(const Annotation &annotation);
    void removeGlobalAnnotation();

    GlobalAnnotationStatus globalStatus() const;
    bool hasGlobalStatus() const;
    void setGlobalStatus(const GlobalAnnotationStatus &status);
    void removeGlobalStatus();

    bool locked() const;
    void setLocked(bool value);

    static bool isThisOrAncestorLocked(const ModelNode &node);
    static ModelNode lowestCommonAncestor(const QList<ModelNode> &nodes);

    qint32 internalId() const;

    void setNodeSource(const QString&);
    void setNodeSource(const QString &newNodeSource, NodeSourceType type);
    QString nodeSource() const;

    QString convertTypeToImportAlias() const;

    NodeSourceType nodeSourceType() const;

    bool isComponent() const;
    QIcon typeIcon() const;
    QString behaviorPropertyName() const;

    friend void swap(ModelNode &first, ModelNode &second) noexcept
    {
        using std::swap;

        swap(first.m_internalNode, second.m_internalNode);
        swap(first.m_model, second.m_model);
        swap(first.m_view, second.m_view);
    }

    friend auto qHash(const ModelNode &node) { return ::qHash(node.m_internalNode.get()); }

private: // functions
    Internal::InternalNodePointer internalNode() const;

    bool hasLocked() const;

private: // variables
    Internal::InternalNodePointer m_internalNode;
    QPointer<Model> m_model;
    QPointer<AbstractView> m_view;
};

QMLDESIGNERCORE_EXPORT bool operator ==(const ModelNode &firstNode, const ModelNode &secondNode);
QMLDESIGNERCORE_EXPORT bool operator !=(const ModelNode &firstNode, const ModelNode &secondNode);
QMLDESIGNERCORE_EXPORT bool operator <(const ModelNode &firstNode, const ModelNode &secondNode);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const ModelNode &modelNode);
QMLDESIGNERCORE_EXPORT QTextStream& operator<<(QTextStream &stream, const ModelNode &modelNode);
}

Q_DECLARE_METATYPE(QmlDesigner::ModelNode)

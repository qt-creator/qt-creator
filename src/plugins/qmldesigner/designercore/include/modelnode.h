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
    using InternalPropertyPointer = std::shared_ptr<InternalProperty>;
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
    friend QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const ModelNode &modelNode);
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

    ModelNode() = default;
    ModelNode(const Internal::InternalNodePointer &internalNode, Model *model, const AbstractView *view);
    ModelNode(const ModelNode &modelNode, AbstractView *view);
    ModelNode(const ModelNode &) = default;
    ModelNode &operator=(const ModelNode &) = default;
    ModelNode(ModelNode &&) = default;
    ModelNode &operator=(ModelNode &&) noexcept = default;
    ~ModelNode() = default;

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

    void removeProperty(PropertyNameView name) const; //### also implement in AbstractProperty
    QList<AbstractProperty> properties() const;
    QList<VariantProperty> variantProperties() const;
    QList<NodeAbstractProperty> nodeAbstractProperties() const;
    QList<NodeProperty> nodeProperties() const;
    QList<NodeListProperty> nodeListProperties() const;
    QList<BindingProperty> bindingProperties() const;
    QList<SignalHandlerProperty> signalProperties() const;
    QList<AbstractProperty> dynamicProperties() const;
    PropertyNameList propertyNames() const;

    bool hasProperty(PropertyNameView name) const;
    bool hasVariantProperty(PropertyNameView name) const;
    bool hasBindingProperty(PropertyNameView name) const;
    bool hasSignalHandlerProperty(PropertyNameView name) const;
    bool hasNodeAbstractProperty(PropertyNameView name) const;
    bool hasDefaultNodeAbstractProperty() const;
    bool hasDefaultNodeListProperty() const;
    bool hasDefaultNodeProperty() const;
    bool hasNodeProperty(PropertyNameView name) const;
    bool hasNodeListProperty(PropertyNameView name) const;
    bool hasProperty(PropertyNameView name, PropertyType propertyType) const;

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

    static int variantTypeId();
    QVariant toVariant() const;

    std::optional<QVariant> auxiliaryData(AuxiliaryDataKeyView key) const;
    std::optional<QVariant> auxiliaryData(AuxiliaryDataType type, Utils::SmallStringView name) const;
    QVariant auxiliaryDataWithDefault(AuxiliaryDataType type, Utils::SmallStringView name) const;
    QVariant auxiliaryDataWithDefault(AuxiliaryDataKeyView key) const;
    QVariant auxiliaryDataWithDefault(AuxiliaryDataKeyDefaultValue key) const;
    void setAuxiliaryData(AuxiliaryDataKeyView key, const QVariant &data) const;
    void setAuxiliaryDataWithoutLock(AuxiliaryDataKeyView key, const QVariant &data) const;
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

    friend bool operator==(const ModelNode &firstNode, const ModelNode &secondNode)
    {
        return firstNode.m_internalNode == secondNode.m_internalNode;
    }

    friend bool operator!=(const ModelNode &firstNode, const ModelNode &secondNode)
    {
        return !(firstNode == secondNode);
    }

    friend bool operator<(const ModelNode &firstNode, const ModelNode &secondNode)
    {
        return firstNode.m_internalNode < secondNode.m_internalNode;
    }

private: // functions
    Internal::InternalNodePointer internalNode() const { return m_internalNode; }

    template<typename Type, typename... PropertyType>
    QList<Type> properties(PropertyType... type) const;
    bool hasLocked() const;

private: // variables
    Internal::InternalNodePointer m_internalNode;
    QPointer<Model> m_model;
    QPointer<AbstractView> m_view;
};

QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const ModelNode &modelNode);
QMLDESIGNERCORE_EXPORT QTextStream& operator<<(QTextStream &stream, const ModelNode &modelNode);

using ModelNodes = QList<ModelNode>;
}

Q_DECLARE_METATYPE(QmlDesigner::ModelNode)

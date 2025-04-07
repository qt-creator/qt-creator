// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractproperty.h"
#include "auxiliarydata.h"

#include <tracing/qmldesignertracingsourcelocation.h>

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

template<typename Result = QVarLengthArray<Internal::InternalNodePointer, 1024>>
QMLDESIGNERCORE_EXPORT Result toInternalNodeList(const QList<ModelNode> &nodeList);

extern template QMLDESIGNERCORE_EXPORT QVarLengthArray<Internal::InternalNodePointer, 1024>
toInternalNodeList<QVarLengthArray<Internal::InternalNodePointer, 1024>>(const QList<ModelNode> &nodeList);
extern template QMLDESIGNERCORE_EXPORT QVarLengthArray<Internal::InternalNodePointer, 32>
toInternalNodeList<QVarLengthArray<Internal::InternalNodePointer, 32>>(const QList<ModelNode> &nodeList);

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
    template<typename Result>
    friend QMLDESIGNERCORE_EXPORT Result toInternalNodeList(const QList<ModelNode> &nodeList);

    friend Model;
    friend AbstractView;
    friend NodeListProperty;
    friend Internal::ModelPrivate;
    friend NodeAbstractProperty;
    friend NodeProperty;

    using SL = ModelTracing::SourceLocation;

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

    TypeName type(SL sl = {}) const;
    QString simplifiedTypeName(SL sl = {}) const;
    QString displayName(SL sl = {}) const;
    int minorVersion(SL sl = {}) const;
    int majorVersion(SL sl = {}) const;

    bool isValid() const;

    explicit operator bool() const { return isValid(); }

    bool isInHierarchy(SL sl = {}) const;

    NodeAbstractProperty parentProperty(SL sl = {}) const;
    void setParentProperty(NodeAbstractProperty parent, SL sl = {});
    void changeType(const TypeName &typeName, int majorVersion = -1, int minorVersion = -1, SL sl = {});
    void setParentProperty(const ModelNode &newParentNode,
                           const PropertyName &propertyName,
                           SL sl = {});
    bool hasParentProperty(SL sl = {}) const;

    QList<ModelNode> directSubModelNodes(SL sl = {}) const;
    QList<ModelNode> directSubModelNodesOfType(const NodeMetaInfo &type, SL sl = {}) const;
    QList<ModelNode> subModelNodesOfType(const NodeMetaInfo &type, SL sl = {}) const;

    QList<ModelNode> allSubModelNodes(SL sl = {}) const;
    QList<ModelNode> allSubModelNodesAndThisNode(SL sl = {}) const;
    bool hasAnySubModelNodes(SL sl = {}) const;

    //###

    AbstractProperty property(PropertyNameView name, SL sl = {}) const;
    VariantProperty variantProperty(PropertyNameView name, SL sl = {}) const;
    BindingProperty bindingProperty(PropertyNameView name, SL sl = {}) const;
    SignalHandlerProperty signalHandlerProperty(PropertyNameView name, SL sl = {}) const;
    SignalDeclarationProperty signalDeclarationProperty(PropertyNameView name, SL sl = {}) const;
    NodeListProperty nodeListProperty(PropertyNameView name, SL sl = {}) const;
    NodeProperty nodeProperty(PropertyNameView name, SL sl = {}) const;
    NodeAbstractProperty nodeAbstractProperty(PropertyNameView name, SL sl = {}) const;
    NodeAbstractProperty defaultNodeAbstractProperty(SL sl = {}) const;
    NodeListProperty defaultNodeListProperty(SL sl = {}) const;
    NodeProperty defaultNodeProperty(SL sl = {}) const;

    void removeProperty(PropertyNameView name, SL sl = {}) const; //### also implement in AbstractProperty
    QList<AbstractProperty> properties(SL sl = {}) const;
    QList<VariantProperty> variantProperties(SL sl = {}) const;
    QList<NodeAbstractProperty> nodeAbstractProperties(SL sl = {}) const;
    QList<NodeProperty> nodeProperties(SL sl = {}) const;
    QList<NodeListProperty> nodeListProperties(SL sl = {}) const;
    QList<BindingProperty> bindingProperties(SL sl = {}) const;
    QList<SignalHandlerProperty> signalProperties(SL sl = {}) const;
    QList<AbstractProperty> dynamicProperties(SL sl = {}) const;
    PropertyNameList propertyNames(SL sl = {}) const;

    bool hasProperty(PropertyNameView name, SL sl = {}) const;
    bool hasVariantProperty(PropertyNameView name, SL sl = {}) const;
    bool hasBindingProperty(PropertyNameView name, SL sl = {}) const;
    bool hasSignalHandlerProperty(PropertyNameView name, SL sl = {}) const;
    bool hasNodeAbstractProperty(PropertyNameView name, SL sl = {}) const;
    bool hasDefaultNodeAbstractProperty(SL sl = {}) const;
    bool hasDefaultNodeListProperty(SL sl = {}) const;
    bool hasDefaultNodeProperty(SL sl = {}) const;
    bool hasNodeProperty(PropertyNameView name, SL sl = {}) const;
    bool hasNodeListProperty(PropertyNameView name, SL sl = {}) const;
    bool hasProperty(PropertyNameView name, PropertyType propertyType, SL sl = {}) const;

    void setScriptFunctions(const QStringList &scriptFunctionList, SL sl = {});
    QStringList scriptFunctions(SL sl = {}) const;

    //###
    void destroy(SL sl = {});

    QString id(SL sl = {}) const;
    void ensureIdExists(SL sl = {}) const;
    [[nodiscard]] QString validId(SL sl = {}) const;
    void setIdWithRefactoring(const QString &id, SL sl = {}) const;
    void setIdWithoutRefactoring(const QString &id, SL sl = {}) const;
    static bool isValidId(const QString &id);
    static QString getIdValidityErrorMessage(const QString &id);

    bool hasId(SL sl = {}) const;

    Model *model() const;
    AbstractView *view() const;

    NodeMetaInfo metaInfo(SL sl = {}) const;
    bool hasMetaInfo(SL sl = {}) const;

    bool isSelected(SL sl = {}) const;
    bool isRootNode(SL sl = {}) const;

    bool isAncestorOf(const ModelNode &node, SL sl = {}) const;
    void selectNode(SL sl = {});
    void deselectNode(SL sl = {});

    static int variantTypeId();
    QVariant toVariant(SL sl = {}) const;

    std::optional<QVariant> auxiliaryData(AuxiliaryDataKeyView key, SL sl = {}) const;
    std::optional<QVariant> auxiliaryData(AuxiliaryDataType type,
                                          Utils::SmallStringView name,
                                          SL sl = {}) const;
    QVariant auxiliaryDataWithDefault(AuxiliaryDataType type,
                                      Utils::SmallStringView name,
                                      SL sl = {}) const;
    QVariant auxiliaryDataWithDefault(AuxiliaryDataKeyView key, SL sl = {}) const;
    QVariant auxiliaryDataWithDefault(AuxiliaryDataKeyDefaultValue key, SL sl = {}) const;
    void setAuxiliaryData(AuxiliaryDataKeyView key, const QVariant &data, SL sl = {}) const;
    void setAuxiliaryDataWithoutLock(AuxiliaryDataKeyView key, const QVariant &data, SL sl = {}) const;
    void setAuxiliaryData(AuxiliaryDataType type,
                          Utils::SmallStringView name,
                          const QVariant &data,
                          SL sl = {}) const;
    void setAuxiliaryDataWithoutLock(AuxiliaryDataType type,
                                     Utils::SmallStringView name,
                                     const QVariant &data,
                                     SL sl = {}) const;
    void removeAuxiliaryData(AuxiliaryDataKeyView key, SL sl = {}) const;
    void removeAuxiliaryData(AuxiliaryDataType type, Utils::SmallStringView name, SL sl = {}) const;
    bool hasAuxiliaryData(AuxiliaryDataKeyView key, SL sl = {}) const;
    bool hasAuxiliaryData(AuxiliaryDataType type, Utils::SmallStringView name, SL sl = {}) const;
    bool hasAuxiliaryData(AuxiliaryDataType type) const;
    AuxiliaryDatasForType auxiliaryData(AuxiliaryDataType type, SL sl = {}) const;
    AuxiliaryDatasView auxiliaryData(SL sl = {}) const;

    QString customId(SL sl = {}) const;
    bool hasCustomId(SL sl = {}) const;
    void setCustomId(const QString &str, SL sl = {});
    void removeCustomId(SL sl = {});

    QVector<Comment> comments(SL sl = {}) const;
    bool hasComments(SL sl = {}) const;
    void setComments(const QVector<Comment> &coms, SL sl = {});
    void addComment(const Comment &com, SL sl = {});
    bool updateComment(const Comment &com, int position, SL sl = {});

    Annotation annotation(SL sl = {}) const;
    bool hasAnnotation(SL sl = {}) const;
    void setAnnotation(const Annotation &annotation, SL sl = {});
    void removeAnnotation(SL sl = {});

    Annotation globalAnnotation(SL sl = {}) const;
    bool hasGlobalAnnotation(SL sl = {}) const;
    void setGlobalAnnotation(const Annotation &annotation, SL sl = {});
    void removeGlobalAnnotation(SL sl = {});

    GlobalAnnotationStatus globalStatus(SL sl = {}) const;
    bool hasGlobalStatus(SL sl = {}) const;
    void setGlobalStatus(const GlobalAnnotationStatus &status, SL sl = {});
    void removeGlobalStatus(SL sl = {});

    bool locked(SL sl = {}) const;
    void setLocked(bool value, SL sl = {});

    qint32 internalId(SL sl = {}) const;

    void setNodeSource(const QString &str, SL sl = {});
    void setNodeSource(const QString &newNodeSource, NodeSourceType type, SL sl = {});
    QString nodeSource(SL sl = {}) const;

    QString convertTypeToImportAlias(SL sl = {}) const;

    NodeSourceType nodeSourceType(SL sl = {}) const;

    bool isComponent(SL sl = {}) const;
    QIcon typeIcon(SL sl = {}) const;
    QString behaviorPropertyName(SL sl = {}) const;

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

    friend std::weak_ordering operator<=>(const ModelNode &firstNode, const ModelNode &secondNode)
    {
        return firstNode.m_internalNode <=> secondNode.m_internalNode;
    }

private: // functions
    Internal::InternalNodePointer internalNode() const { return m_internalNode; }

    template<typename Type, typename... PropertyType>
    QList<Type> properties(PropertyType... type) const;
    bool hasLocked(SL sl = {}) const;

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

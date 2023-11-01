// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectioneditorutils.h"

#include "abstractview.h"
#include "bindingproperty.h"
#include "nodemetainfo.h"
#include "propertymetainfo.h"

#include <variant>

#include <utils/qtcassert.h>

#include <QColor>
#include <QUrl>

namespace {

using CollectionDataVariant = std::variant<QString, bool, double, QUrl, QColor>;

inline bool operator<(const QColor &a, const QColor &b)
{
    return a.name(QColor::HexArgb) < b.name(QColor::HexArgb);
}

inline CollectionDataVariant valueToVariant(const QVariant &value,
                                            QmlDesigner::CollectionDetails::DataType type)
{
    using DataType = QmlDesigner::CollectionDetails::DataType;
    switch (type) {
    case DataType::String:
        return value.toString();
    case DataType::Number:
        return value.toDouble();
    case DataType::Boolean:
        return value.toBool();
    case DataType::Color:
        return value.value<QColor>();
    case DataType::Url:
        return value.value<QUrl>();
    default:
        return false;
    }
}

struct LessThanVisitor
{
    template<typename T1, typename T2>
    bool operator()(const T1 &a, const T2 &b) const
    {
        return CollectionDataVariant(a).index() < CollectionDataVariant(b).index();
    }

    template<typename T>
    bool operator()(const T &a, const T &b) const
    {
        return a < b;
    }
};

} // namespace

namespace QmlDesigner::CollectionEditor {

bool variantIslessThan(const QVariant &a, const QVariant &b, CollectionDetails::DataType type)
{
    return std::visit(LessThanVisitor{}, valueToVariant(a, type), valueToVariant(b, type));
}

QString getSourceCollectionType(const ModelNode &node)
{
    using namespace QmlDesigner;
    if (node.type() == CollectionEditor::JSONCOLLECTIONMODEL_TYPENAME)
        return "json";

    if (node.type() == CollectionEditor::CSVCOLLECTIONMODEL_TYPENAME)
        return "csv";

    return {};
}

void assignCollectionSourceToNode(AbstractView *view,
                                  const ModelNode &modelNode,
                                  const ModelNode &collectionSourceNode)
{
    QTC_ASSERT(modelNode.isValid() && collectionSourceNode.isValid(), return);

    if (collectionSourceNode.id().isEmpty() || !canAcceptCollectionAsModel(modelNode))
        return;

    BindingProperty modelProperty = modelNode.bindingProperty("model");

    view->executeInTransaction("CollectionEditor::assignCollectionSourceToNode",
                               [&modelProperty, &collectionSourceNode]() {
                                   modelProperty.setExpression(collectionSourceNode.id());
                               });
}

bool canAcceptCollectionAsModel(const ModelNode &node)
{
    const NodeMetaInfo nodeMetaInfo = node.metaInfo();
    if (!nodeMetaInfo.isValid())
        return false;

    const PropertyMetaInfo modelProperty = nodeMetaInfo.property("model");
    if (!modelProperty.isValid())
        return false;

    return modelProperty.isWritable() && !modelProperty.isPrivate()
           && modelProperty.propertyType().isVariant();
}

} // namespace QmlDesigner::CollectionEditor

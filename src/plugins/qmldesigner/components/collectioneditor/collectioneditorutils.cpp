// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectioneditorutils.h"

#include <variant>

#include <QColor>
#include <QUrl>

namespace {

using CollectionDataVariant = std::variant<QString, bool, double, QUrl, QColor>;

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

    template<>
    bool operator()(const QColor &a, const QColor &b) const
    {
        return a.name(QColor::HexArgb) < b.name(QColor::HexArgb);
    }
};

} // namespace

namespace QmlDesigner::CollectionEditor {

bool variantIslessThan(const QVariant &a, const QVariant &b, CollectionDetails::DataType type)
{
    return std::visit(LessThanVisitor{}, valueToVariant(a, type), valueToVariant(b, type));
}

} // namespace QmlDesigner::CollectionEditor

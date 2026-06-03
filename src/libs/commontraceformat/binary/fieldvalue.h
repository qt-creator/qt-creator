// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QtTypes>

#include <memory>
#include <utility>
#include <variant>

namespace CommonTraceFormat {

// Forward declarations for recursive types.
struct StructureValue;
struct ArrayValue;
struct VariantValue;

// Runtime value produced by FieldReader / consumed by FieldWriter.
// Compound types (StructureValue, ArrayValue, VariantValue) are wrapped in
// shared_ptr to break the circularity: std::variant<..., StructureValue>
// would require StructureValue to be complete, but StructureValue contains
// QHash<QString, FieldValue>. shared_ptr<T> is itself complete for any T.
using FieldValue = std::variant<
    std::monostate,
    bool,
    quint64,
    qint64,
    double,
    QString,
    QByteArray,
    std::shared_ptr<StructureValue>,
    std::shared_ptr<ArrayValue>,
    std::shared_ptr<VariantValue>>;

struct CMNTRCEFMT_EXPORT StructureValue
{
    QHash<QString, FieldValue> members;
    QList<QString> order; // insertion-order member names

    void set(const QString &name, FieldValue v)
    {
        if (!members.contains(name))
            order.append(name);
        members[name] = std::move(v);
    }

    const FieldValue *get(const QString &name) const
    {
        auto it = members.constFind(name);
        return it == members.constEnd() ? nullptr : &*it;
    }
};

struct CMNTRCEFMT_EXPORT ArrayValue
{
    QList<FieldValue> elements;
};

struct CMNTRCEFMT_EXPORT VariantValue
{
    // Name of the selected option (may be empty: option names are optional and
    // non-semantic, spec 5.3.23.1). For writing, prefer `selectedIndex`, which
    // identifies the option unambiguously even when names are absent/duplicated.
    QString selectedOption;
    // Index of the selected option within the variant field class's options,
    // or -1 if unknown. Set by FieldReader::readVariant.
    int selectedIndex = -1;
    std::unique_ptr<FieldValue> value;
};

// Convenience helpers to wrap/unwrap compound values.
inline FieldValue makeStructureValue(StructureValue sv)
{
    return std::make_shared<StructureValue>(std::move(sv));
}

inline FieldValue makeArrayValue(ArrayValue av)
{
    return std::make_shared<ArrayValue>(std::move(av));
}

inline bool isStructure(const FieldValue &fv)
{
    return std::holds_alternative<std::shared_ptr<StructureValue>>(fv)
           && std::get<std::shared_ptr<StructureValue>>(fv) != nullptr;
}

inline bool isArray(const FieldValue &fv)
{
    return std::holds_alternative<std::shared_ptr<ArrayValue>>(fv)
           && std::get<std::shared_ptr<ArrayValue>>(fv) != nullptr;
}

inline StructureValue &asStructure(FieldValue &fv)
{
    return *std::get<std::shared_ptr<StructureValue>>(fv);
}

inline const StructureValue &asStructure(const FieldValue &fv)
{
    return *std::get<std::shared_ptr<StructureValue>>(fv);
}

inline ArrayValue &asArray(FieldValue &fv)
{
    return *std::get<std::shared_ptr<ArrayValue>>(fv);
}

inline const ArrayValue &asArray(const FieldValue &fv)
{
    return *std::get<std::shared_ptr<ArrayValue>>(fv);
}

} // namespace CommonTraceFormat

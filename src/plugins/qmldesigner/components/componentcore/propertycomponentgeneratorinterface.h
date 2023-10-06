// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelfwd.h>
#include <nodemetainfo.h>

#include <qmljs/qmljssimplereader.h>

#include <optional>
#include <variant>
#include <vector>

namespace QmlDesigner {

class PropertyComponentGeneratorInterface
{
public:
    PropertyComponentGeneratorInterface() = default;
    PropertyComponentGeneratorInterface(const PropertyComponentGeneratorInterface &) = delete;
    PropertyComponentGeneratorInterface &operator=(const PropertyComponentGeneratorInterface &) = delete;

    struct BasicProperty
    {
        Utils::SmallString propertyName;
        QString component;
    };

    struct ComplexProperty
    {
        Utils::SmallString propertyName;
        QString component;
    };

    using Property = std::variant<std::monostate, BasicProperty, ComplexProperty>;

    virtual Property create(const PropertyMetaInfo &property) const = 0;

    virtual QStringList imports() const = 0;

protected:
    ~PropertyComponentGeneratorInterface() = default;
};

} // namespace QmlDesigner

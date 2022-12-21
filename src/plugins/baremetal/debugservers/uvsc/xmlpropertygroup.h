// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "xmlproperty.h"

#include <projectexplorer/abi.h>

#include <memory>

namespace BareMetal {

class ProductData;
class Project;

namespace Gen {
namespace Xml {

class PropertyGroup : public Property
{
public:
    explicit PropertyGroup(QByteArray name);
    PropertyGroup *appendPropertyGroup(QByteArray name);

    void accept(INodeVisitor *visitor) const final;
};

} // namespace Xml
} // namespace Gen
} // namespace BareMetal

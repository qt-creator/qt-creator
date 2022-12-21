// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "xmlproperty.h"

#include <memory>

namespace BareMetal::Gen::Xml {

// Project

class Project : public Property
{
public:
    void accept(INodeVisitor *visitor) const final;
};

// ProjectOptions

class ProjectOptions : public Property
{
public:
    void accept(INodeVisitor *visitor) const final;
};

} // BareMetal::Gen::Xml

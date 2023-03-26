// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace qmt {

class ShapeVisitor;
class ShapeConstVisitor;

class IShape
{
public:
    virtual ~IShape() { }

    virtual IShape *clone() const = 0;
    virtual void accept(ShapeVisitor *visitor) = 0;
    virtual void accept(ShapeConstVisitor *visitor) const = 0;
};

} // namespace qmt

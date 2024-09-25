// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "internalnodeabstractproperty.h"

namespace QmlDesigner {
namespace Internal  {

class InternalNodeProperty : public InternalNodeAbstractProperty
{
public:
    using Pointer = std::shared_ptr<InternalNodeProperty>;
    using ManyNodes = QVarLengthArray<InternalNodePointer, 1024>;

    static constexpr PropertyType type = PropertyType::Node;

    InternalNodeProperty(PropertyNameView name, const InternalNodePointer &node);

    bool isValid() const override;
    bool isEmpty() const override;
    int count() const override;
    int indexOf(const InternalNodePointer &node) const override;

    ManyNodes allSubNodes() const;
    void addSubNodes(ManyNodes &container) const;

    const InternalNodePointer &node() const { return m_node; }

protected:
    void add(const InternalNodePointer &node) override;
    void remove(const InternalNodePointer &node) override;

private:
    InternalNodePointer m_node;
};

} // namespace Internal
} // namespace QmlDesigner

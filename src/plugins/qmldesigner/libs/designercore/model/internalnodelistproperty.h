// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "internalnodeabstractproperty.h"

#include <QList>

namespace QmlDesigner {

namespace Internal {

class InternalNodeListProperty final : public InternalNodeAbstractProperty
{
public:
    using Pointer = std::shared_ptr<InternalNodeListProperty>;
    using FewNodes = QVarLengthArray<InternalNodePointer, 32>;
    using ManyNodes = QVarLengthArray<InternalNodePointer, 1024>;

    static constexpr PropertyType type = PropertyType::NodeList;

    InternalNodeListProperty(PropertyNameView name, const InternalNodePointer &propertyOwner);

    bool isValid() const override;

    bool isEmpty() const override;

    int size() const { return m_nodes.size(); }

    int count() const override;
    int indexOf(const InternalNodePointer &node) const override;

    const InternalNodePointer &at(int index) const
    {
        Q_ASSERT(index >= 0 && index < m_nodes.size());
        return m_nodes[index];
    }

    InternalNodePointer &at(int index)
    {
        Q_ASSERT(index >= 0 && index < m_nodes.size());
        return m_nodes[index];
    }

    InternalNodePointer &find(InternalNodePointer node)
    {
        auto found = std::find(m_nodes.begin(), m_nodes.end(), node);

        return *found;
    }

    ManyNodes allSubNodes() const;
    const FewNodes &nodeList() const;
    void slide(int from, int to);

    void addSubNodes(ManyNodes &container) const;

    FewNodes::iterator begin() { return m_nodes.begin(); }

    FewNodes::iterator end() { return m_nodes.end(); }

    const FewNodes &nodes() const { return m_nodes; }

protected:
    void add(const InternalNodePointer &node) override;
    void remove(const InternalNodePointer &node) override;

private:
    FewNodes m_nodes;
};

} // namespace Internal
} // namespace QmlDesigner

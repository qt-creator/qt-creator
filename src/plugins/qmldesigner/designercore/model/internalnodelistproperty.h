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
    static constexpr PropertyType type = PropertyType::NodeList;

    InternalNodeListProperty(const PropertyName &name, const InternalNodePointer &propertyOwner);

    bool isValid() const override;

    bool isEmpty() const override;
    int size() const { return m_nodeList.size(); }
    int count() const override;
    int indexOf(const InternalNodePointer &node) const override;
    const InternalNodePointer &at(int index) const
    {
        Q_ASSERT(index >= 0 && index < m_nodeList.size());
        return m_nodeList[index];
    }

    InternalNodePointer &at(int index)
    {
        Q_ASSERT(index >= 0 && index < m_nodeList.size());
        return m_nodeList[index];
    }

    InternalNodePointer &find(InternalNodePointer node)
    {
        auto found = std::find(m_nodeList.begin(), m_nodeList.end(), node);

        return *found;
    }

    QList<InternalNodePointer> allSubNodes() const;
    const QList<InternalNodePointer> &nodeList() const;
    void slide(int from, int to);

    void addSubNodes(QList<InternalNodePointer> &container) const;

    QList<InternalNodePointer>::iterator begin() { return m_nodeList.begin(); }

    QList<InternalNodePointer>::iterator end() { return m_nodeList.end(); }

    const QList<InternalNodePointer> &nodes() const { return m_nodeList; }

protected:
    void add(const InternalNodePointer &node) override;
    void remove(const InternalNodePointer &node) override;

private:
    QList<InternalNodePointer> m_nodeList;
};

} // namespace Internal
} // namespace QmlDesigner

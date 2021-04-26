/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "modelnode.h"
#include "nodeabstractproperty.h"
#include "qmldesignercorelib_global.h"

#include <QList>

#include <iterator>

namespace QmlDesigner {

namespace Internal {
class ModelPrivate;
class InternalNodeListProperty;
using InternalNodeListPropertyPointer = QSharedPointer<InternalNodeListProperty>;

class NodeListPropertyIterator
{
    friend NodeListProperty;

public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = qsizetype;
    using value_type = ModelNode;
    using pointer = InternalNodeListPropertyPointer;
    using reference = ModelNode;

    NodeListPropertyIterator() = default;
    NodeListPropertyIterator(difference_type currentIndex,
                             class InternalNodeListProperty *nodeListProperty,
                             Model *model,
                             AbstractView *view)
        : m_nodeListProperty{nodeListProperty}
        , m_model{model}
        , m_view{view}
        , m_currentIndex{currentIndex}
    {}

    NodeListPropertyIterator &operator++()
    {
        ++m_currentIndex;
        return *this;
    }

    NodeListPropertyIterator operator++(int)
    {
        auto tmp = *this;
        ++m_currentIndex;
        return tmp;
    }

    NodeListPropertyIterator &operator--()
    {
        --m_currentIndex;
        return *this;
    }

    NodeListPropertyIterator operator--(int)
    {
        auto tmp = *this;
        --m_currentIndex;
        return tmp;
    }

    NodeListPropertyIterator &operator+=(difference_type index)
    {
        m_currentIndex += index;
        return *this;
    }

    NodeListPropertyIterator &operator-=(difference_type index)
    {
        m_currentIndex -= index;
        return *this;
    }

    NodeListPropertyIterator operator[](difference_type index) const
    {
        return {index, m_nodeListProperty, m_model, m_view};
    }

    friend NodeListPropertyIterator operator+(const NodeListPropertyIterator &first,
                                              difference_type index)
    {
        return {first.m_currentIndex + index, first.m_nodeListProperty, first.m_model, first.m_view};
    }

    friend NodeListPropertyIterator operator+(difference_type index,
                                              const NodeListPropertyIterator &second)
    {
        return second + index;
    }

    friend NodeListPropertyIterator operator-(const NodeListPropertyIterator &first,
                                              difference_type index)
    {
        return first + (-index);
    }

    friend difference_type operator-(const NodeListPropertyIterator &first,
                                     const NodeListPropertyIterator &second)
    {
        return first.m_currentIndex - second.m_currentIndex;
    }

    friend bool operator==(const NodeListPropertyIterator &first,
                           const NodeListPropertyIterator &second)
    {
        return first.m_currentIndex == second.m_currentIndex;
    }

    friend bool operator!=(const NodeListPropertyIterator &first,
                           const NodeListPropertyIterator &second)
    {
        return !(first == second);
    }

    friend bool operator<(const NodeListPropertyIterator &first, const NodeListPropertyIterator &second)
    {
        return first.m_currentIndex < second.m_currentIndex;
    }

    friend bool operator<=(const NodeListPropertyIterator &first,
                           const NodeListPropertyIterator &second)
    {
        return first.m_currentIndex <= second.m_currentIndex;
    }
    friend bool operator>(const NodeListPropertyIterator &first, const NodeListPropertyIterator &second)
    {
        return first.m_currentIndex > second.m_currentIndex;
    }

    friend bool operator>=(const NodeListPropertyIterator &first,
                           const NodeListPropertyIterator &second)
    {
        return first.m_currentIndex >= second.m_currentIndex;
    }

    value_type operator*() const;

private:
    InternalNodeListProperty *m_nodeListProperty{};
    Model *m_model{};
    AbstractView *m_view{};
    difference_type m_currentIndex = -1;
};

} // namespace Internal

class QMLDESIGNERCORE_EXPORT NodeListProperty : public NodeAbstractProperty
{
    friend ModelNode;
    friend AbstractProperty;
    friend Internal::ModelPrivate;

public:
    using value_type = ModelNode;
    using iterator = Internal::NodeListPropertyIterator;
    using const_iterator = iterator;
    using size_type = int;
    using difference_type = int;
    using reference = ModelNode;

    NodeListProperty();
    NodeListProperty(const NodeListProperty &nodeListProperty, AbstractView *view);
    QList<ModelNode> toModelNodeList() const;
    QList<QmlObjectNode> toQmlObjectNodeList() const;
    void slide(int, int) const;
    void swap(int, int) const;
    void reparentHere(const ModelNode &modelNode);
    ModelNode at(int index) const;
    void iterSwap(iterator &first, iterator &second);
    iterator rotate(iterator first, iterator newFirst, iterator last);
    template<typename Range>
    iterator rotate(Range &range, iterator newFirst)
    {
        return rotate(range.begin(), newFirst, range.end());
    }
    void reverse(iterator first, iterator last);
    template<typename Range>
    void reverse(Range &range)
    {
        reverse(range.begin(), range.end());
    }

    static void reverseModelNodes(const QList<ModelNode> &nodes);

    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;

protected:
    NodeListProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view);
    NodeListProperty(const Internal::InternalNodeListPropertyPointer &internalNodeListProperty,
                     Model *model,
                     AbstractView *view);

    Internal::InternalNodeListPropertyPointer &internalNodeListProperty() const;

private:
    mutable Internal::InternalNodeListPropertyPointer m_internalNodeListProperty{};
};
}

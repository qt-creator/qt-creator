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

#include "internalnodeabstractproperty.h"

#include <QList>

namespace QmlDesigner {

namespace Internal {

class InternalNodeListProperty final : public InternalNodeAbstractProperty
{
public:
    using Pointer = QSharedPointer<InternalNodeListProperty>;

    static Pointer create(const PropertyName &name, const InternalNodePointer &propertyOwner);

    bool isValid() const override;

    bool isEmpty() const override;
    int size() const { return m_nodeList.size(); }
    int count() const override;
    int indexOf(const InternalNodePointer &node) const override;
    const InternalNodePointer &at(int index) const
    {
        Q_ASSERT(index >= 0 && index < m_nodeList.count());
        return m_nodeList[index];
    }

    InternalNodePointer &at(int index)
    {
        Q_ASSERT(index >= 0 && index < m_nodeList.count());
        return m_nodeList[index];
    }

    InternalNodePointer &find(InternalNodePointer node)
    {
        auto found = std::find(m_nodeList.begin(), m_nodeList.end(), node);

        return *found;
    }

    bool isNodeListProperty() const override;

    QList<InternalNodePointer> allSubNodes() const override;
    QList<InternalNodePointer> directSubNodes() const override;
    const QList<InternalNodePointer> &nodeList() const;
    void slide(int from, int to);

    QList<InternalNodePointer>::iterator begin() { return m_nodeList.begin(); }
    QList<InternalNodePointer>::iterator end() { return m_nodeList.end(); }

protected:
    InternalNodeListProperty(const PropertyName &name, const InternalNodePointer &propertyOwner);
    void add(const InternalNodePointer &node) override;
    void remove(const InternalNodePointer &node) override;

private:
    QList<InternalNodePointer> m_nodeList;
};

} // namespace Internal
} // namespace QmlDesigner

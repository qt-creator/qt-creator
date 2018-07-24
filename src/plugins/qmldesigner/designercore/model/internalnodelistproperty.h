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

class InternalNodeListProperty : public InternalNodeAbstractProperty
{
public:
    using Pointer = QSharedPointer<InternalNodeListProperty>;

    static Pointer create(const PropertyName &name, const InternalNodePointer &propertyOwner);

    bool isValid() const override;

    bool isEmpty() const override;
    int count() const override;
    int indexOf(const InternalNodePointer &node) const override;
    InternalNodePointer at(int index) const;

    bool isNodeListProperty() const override;

    QList<InternalNodePointer> allSubNodes() const override;
    QList<InternalNodePointer> directSubNodes() const override;
    const QList<InternalNodePointer> &nodeList() const;
    void slide(int from, int to);

protected:
    InternalNodeListProperty(const PropertyName &name, const InternalNodePointer &propertyOwner);
    void add(const InternalNodePointer &node) override;
    void remove(const InternalNodePointer &node) override;

private:
    QList<InternalNodePointer> m_nodeList;
};

} // namespace Internal
} // namespace QmlDesigner

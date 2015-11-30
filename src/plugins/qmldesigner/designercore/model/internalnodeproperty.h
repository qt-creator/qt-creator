/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef INTERNALNODEPROPERTY_H
#define INTERNALNODEPROPERTY_H

#include "internalnodeabstractproperty.h"

namespace QmlDesigner {
namespace Internal  {

class InternalNodeProperty : public InternalNodeAbstractProperty
{
public:
    typedef QSharedPointer<InternalNodeProperty> Pointer;

    static Pointer create(const PropertyName &name, const InternalNodePointer &propertyOwner);

    bool isValid() const;
    bool isEmpty() const;
    int count() const;
    int indexOf(const InternalNodePointer &node) const;
    bool isNodeProperty() const;

    QList<InternalNodePointer> allSubNodes() const;
    QList<InternalNodePointer> directSubNodes() const;

    InternalNodePointer node() const;


protected:
    InternalNodeProperty(const PropertyName &name, const InternalNodePointer &node);
    void add(const InternalNodePointer &node);
    void remove(const InternalNodePointer &node);

private:
    InternalNodePointer m_node;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // INTERNALNODEPROPERTY_H

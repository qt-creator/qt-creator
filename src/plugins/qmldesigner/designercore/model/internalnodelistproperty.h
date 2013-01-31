/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef INTERNALNODELISTPROPERTY_H
#define INTERNALNODELISTPROPERTY_H

#include "internalnodeabstractproperty.h"

#include <QList>

namespace QmlDesigner {

namespace Internal {

class InternalNodeListProperty : public InternalNodeAbstractProperty
{
public:
    typedef QSharedPointer<InternalNodeListProperty> Pointer;

    static Pointer create(const QString &name, const InternalNodePointer &propertyOwner);

    bool isValid() const;

    bool isEmpty() const;
    int count() const;
    int indexOf(const InternalNodePointer &node) const;
    InternalNodePointer at(int index) const;

    bool isNodeListProperty() const;

    QList<InternalNodePointer> allSubNodes() const;
    QList<InternalNodePointer> allDirectSubNodes() const;
    const QList<InternalNodePointer> &nodeList() const;
    void slide(int from, int to);

protected:
    InternalNodeListProperty(const QString &name, const InternalNodePointer &propertyOwner);
    void add(const InternalNodePointer &node);
    void remove(const InternalNodePointer &node);

private:
    QList<InternalNodePointer> m_nodeList;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // INTERNALNODELISTPROPERTY_H

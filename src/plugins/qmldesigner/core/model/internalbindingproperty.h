/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef INTERNALBINDINGPROPERTY_H
#define INTERNALBINDINGPROPERTY_H

#include "internalproperty.h"

namespace QmlDesigner {
namespace Internal {

class InternalBindingProperty : public InternalProperty
{
public:
    typedef QSharedPointer<InternalBindingProperty> Pointer;

    static Pointer create(const QString &name, const InternalNodePointer &propertyOwner);

    bool isValid() const;

    QString expression() const;
    void setExpression(const QString &expression);

    void setDynamicExpression(const QString &type, const QString &expression);


    bool isBindingProperty() const;

protected:
    InternalBindingProperty(const QString &name, const InternalNodePointer &propertyOwner);

private:
    QString m_expression;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // INTERNALBINDINGPROPERTY_H

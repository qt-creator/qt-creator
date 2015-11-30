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

#ifndef ADDPROPERTYVISITOR_H
#define ADDPROPERTYVISITOR_H

#include "qmlrefactoring.h"
#include "qmlrewriter.h"

namespace QmlDesigner {
namespace Internal {

class AddPropertyVisitor: public QMLRewriter
{
public:
public:
    AddPropertyVisitor(TextModifier &modifier,
                       quint32 parentLocation,
                       const PropertyName &name,
                       const QString &value,
                       QmlRefactoring::PropertyType propertyType,
                       const PropertyNameList &propertyOrder,
                       const TypeName &dynamicTypeName);

protected:
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);
    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);

private:
    void addInMembers(QmlJS::AST::UiObjectInitializer *initializer);

private:
    quint32 m_parentLocation;
    PropertyName m_name;
    QString m_value;
    QmlRefactoring::PropertyType m_propertyType;
    PropertyNameList m_propertyOrder;
    TypeName m_dynamicTypeName;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // ADDPROPERTYVISITOR_H

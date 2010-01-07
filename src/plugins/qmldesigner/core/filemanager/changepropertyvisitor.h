/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CHANGEPROPERTYVISITOR_H
#define CHANGEPROPERTYVISITOR_H

#include "qmlrefactoring.h"
#include "qmlrewriter.h"

namespace QmlEditor {
namespace Internal {

class ChangePropertyVisitor: public QMLRewriter
{
public:
    ChangePropertyVisitor(QmlDesigner::TextModifier &modifier,
                          quint32 parentLocation,
                          const QString &name,
                          const QString &value,
                          QmlRefactoring::PropertyType propertyType);

protected:
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);
    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);

private:
    void replaceInMembers(QmlJS::AST::UiObjectInitializer *initializer);
    void replaceMemberValue(QmlJS::AST::UiObjectMember *propertyMember, bool needsSemicolon);
    bool isMatchingPropertyMember(QmlJS::AST::UiObjectMember *member) const;
    static bool nextMemberOnSameLine(QmlJS::AST::UiObjectMemberList *members);

    void insertIntoArray(QmlJS::AST::UiArrayBinding* ast);

private:
    quint32 m_parentLocation;
    QString m_name;
    QString m_value;
    QmlRefactoring::PropertyType m_propertyType;
};

} // namespace Internal
} // namespace QmlEditor

#endif // CHANGEPROPERTYVISITOR_H

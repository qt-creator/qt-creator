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

#ifndef ADDARRAYMEMBERVISITOR_H
#define ADDARRAYMEMBERVISITOR_H

#include "qmlrewriter.h"

namespace QmlDesigner {
namespace Internal {

class AddArrayMemberVisitor: public QMLRewriter
{
public:
    AddArrayMemberVisitor(QmlDesigner::TextModifier &modifier,
                          quint32 parentLocation,
                          const QString &propertyName,
                          const QString &content);

    bool willConvertObjectBindingIntoArrayBinding() const
    { return m_convertObjectBindingIntoArrayBinding; }

    void setConvertObjectBindingIntoArrayBinding(bool convertObjectBindingIntoArrayBinding)
    { m_convertObjectBindingIntoArrayBinding = convertObjectBindingIntoArrayBinding; }

protected:
    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);

private:
    void findArrayBindingAndInsert(const QString &m_propertyName, QmlJS::AST::UiObjectMemberList *ast);

    void insertInto(QmlJS::AST::UiArrayBinding *arrayBinding);
    void convertAndAdd(QmlJS::AST::UiObjectBinding *objectBinding);

private:
    quint32 m_parentLocation;
    QString m_propertyName;
    QString m_content;
    bool m_convertObjectBindingIntoArrayBinding;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // ADDARRAYMEMBERVISITOR_H

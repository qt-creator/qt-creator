/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
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

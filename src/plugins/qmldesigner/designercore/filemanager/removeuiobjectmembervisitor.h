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

#ifndef REMOVEUIOBJECTMEMBERVISITOR_H
#define REMOVEUIOBJECTMEMBERVISITOR_H

#include <QStack>

#include "qmlrewriter.h"

namespace QmlDesigner {
namespace Internal {

class RemoveUIObjectMemberVisitor: public QMLRewriter
{
public:
    RemoveUIObjectMemberVisitor(QmlDesigner::TextModifier &modifier,
                                quint32 objectLocation);

protected:
    virtual bool preVisit(QmlJS::AST::Node *ast);
    virtual void postVisit(QmlJS::AST::Node *);

    virtual bool visit(QmlJS::AST::UiPublicMember *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);
    virtual bool visit(QmlJS::AST::UiSourceElement *ast);
    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QmlJS::AST::UiScriptBinding *ast);
    virtual bool visit(QmlJS::AST::UiArrayBinding *ast);

private:
    bool visitObjectMember(QmlJS::AST::UiObjectMember *ast);

    QmlJS::AST::UiArrayBinding *containingArray() const;

    void extendToLeadingOrTrailingComma(QmlJS::AST::UiArrayBinding *parentArray,
                                        QmlJS::AST::UiObjectMember *ast,
                                        int &start,
                                        int &end) const;

private:
    quint32 objectLocation;
    QStack<QmlJS::AST::Node*> parents;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // REMOVEUIOBJECTMEMBERVISITOR_H

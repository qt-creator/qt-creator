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

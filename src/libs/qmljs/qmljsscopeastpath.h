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

#include "qmljs_global.h"
#include "parser/qmljsastvisitor_p.h"
#include "qmljsdocument.h"

namespace QmlJS {

class QMLJS_EXPORT ScopeAstPath: protected AST::Visitor
{
public:
    ScopeAstPath(Document::Ptr doc);

    QList<AST::Node *> operator()(quint32 offset);

protected:
    void accept(AST::Node *node);

    using Visitor::visit;

    bool preVisit(AST::Node *node) override;
    bool visit(AST::UiPublicMember *node) override;
    bool visit(AST::UiScriptBinding *node) override;
    bool visit(AST::UiObjectDefinition *node) override;
    bool visit(AST::UiObjectBinding *node) override;
    bool visit(AST::FunctionDeclaration *node) override;
    bool visit(AST::FunctionExpression *node) override;

private:
    bool containsOffset(AST::SourceLocation start, AST::SourceLocation end);

    QList<AST::Node *> _result;
    Document::Ptr _doc;
    quint32 _offset = 0;
};

} // namespace QmlJS

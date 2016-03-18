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

#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsstaticanalysismessage.h>
#include <qmljs/parser/qmljsastvisitor_p.h>

#include <QCoreApplication>
#include <QSet>
#include <QStack>

namespace QmlJS {

class Imports;

class QMLJS_EXPORT Check: protected AST::Visitor
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::Check)

    typedef QSet<QString> StringSet;

public:
    // prefer taking root scope chain?
    Check(Document::Ptr doc, const ContextPtr &context);
    ~Check();

    QList<StaticAnalysis::Message> operator()();

    void enableMessage(StaticAnalysis::Type type);
    void disableMessage(StaticAnalysis::Type type);

    void enableQmlDesignerChecks();
    void disableQmlDesignerChecks();

    void enableQmlDesignerUiFileChecks();
    void disableQmlDesignerUiFileChecks();

protected:
    bool preVisit(AST::Node *ast) override;
    void postVisit(AST::Node *ast) override;

    bool visit(AST::UiProgram *ast) override;
    bool visit(AST::UiObjectDefinition *ast) override;
    bool visit(AST::UiObjectBinding *ast) override;
    bool visit(AST::UiScriptBinding *ast) override;
    bool visit(AST::UiArrayBinding *ast) override;
    bool visit(AST::UiPublicMember *ast) override;
    bool visit(AST::IdentifierExpression *ast) override;
    bool visit(AST::FieldMemberExpression *ast) override;
    bool visit(AST::FunctionDeclaration *ast) override;
    bool visit(AST::FunctionExpression *ast) override;
    bool visit(AST::UiObjectInitializer *) override;

    bool visit(AST::BinaryExpression *ast) override;
    bool visit(AST::Block *ast) override;
    bool visit(AST::WithStatement *ast) override;
    bool visit(AST::VoidExpression *ast) override;
    bool visit(AST::Expression *ast) override;
    bool visit(AST::ExpressionStatement *ast) override;
    bool visit(AST::IfStatement *ast) override;
    bool visit(AST::ForStatement *ast) override;
    bool visit(AST::LocalForStatement *ast) override;
    bool visit(AST::WhileStatement *ast) override;
    bool visit(AST::DoWhileStatement *ast) override;
    bool visit(AST::CaseBlock *ast) override;
    bool visit(AST::NewExpression *ast) override;
    bool visit(AST::NewMemberExpression *ast) override;
    bool visit(AST::CallExpression *ast) override;
    bool visit(AST::StatementList *ast) override;
    bool visit(AST::ReturnStatement *ast) override;
    bool visit(AST::ThrowStatement *ast) override;
    bool visit(AST::DeleteExpression *ast) override;
    bool visit(AST::TypeOfExpression *ast) override;

    void endVisit(QmlJS::AST::UiObjectInitializer *) override;

private:
    void visitQmlObject(AST::Node *ast, AST::UiQualifiedId *typeId,
                        AST::UiObjectInitializer *initializer);
    const Value *checkScopeObjectMember(const AST::UiQualifiedId *id);
    void checkAssignInCondition(AST::ExpressionNode *condition);
    void checkCaseFallthrough(AST::StatementList *statements, AST::SourceLocation errorLoc, AST::SourceLocation nextLoc);
    void checkProperty(QmlJS::AST::UiQualifiedId *);
    void checkNewExpression(AST::ExpressionNode *node);
    void checkBindingRhs(AST::Statement *statement);
    void checkExtraParentheses(AST::ExpressionNode *expression);

    void addMessages(const QList<StaticAnalysis::Message> &messages);
    void addMessage(const StaticAnalysis::Message &message);
    void addMessage(StaticAnalysis::Type type, const AST::SourceLocation &location,
                    const QString &arg1 = QString(), const QString &arg2 = QString());

    void scanCommentsForAnnotations();
    void warnAboutUnnecessarySuppressions();

    bool isQtQuick2() const;
    bool isQtQuick2Ui() const;

    AST::Node *parent(int distance = 0);

    Document::Ptr _doc;

    ContextPtr _context;
    ScopeChain _scopeChain;
    ScopeBuilder _scopeBuilder;

    QList<StaticAnalysis::Message> _messages;
    QSet<StaticAnalysis::Type> _enabledMessages;

    QList<AST::Node *> _chain;
    QStack<StringSet> m_idStack;
    QStack<StringSet> m_propertyStack;
    QStack<QString> m_typeStack;

    class MessageTypeAndSuppression
    {
    public:
        AST::SourceLocation suppressionSource;
        StaticAnalysis::Type type;
        bool wasSuppressed;
    };

    QHash< int, QList<MessageTypeAndSuppression> > m_disabledMessageTypesByLine;

    bool _importsOk;
    bool _inStatementBinding;
    const Imports *_imports;
    bool _isQtQuick2;
};

} // namespace QmlJS

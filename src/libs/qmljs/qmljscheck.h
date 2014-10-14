/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QMLJSCHECK_H
#define QMLJSCHECK_H

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
    bool preVisit(AST::Node *ast) Q_DECL_OVERRIDE;
    void postVisit(AST::Node *ast) Q_DECL_OVERRIDE;

    bool visit(AST::UiProgram *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiObjectDefinition *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiObjectBinding *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiScriptBinding *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiArrayBinding *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiPublicMember *ast) Q_DECL_OVERRIDE;
    bool visit(AST::IdentifierExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::FieldMemberExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::FunctionDeclaration *ast) Q_DECL_OVERRIDE;
    bool visit(AST::FunctionExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::UiObjectInitializer *) Q_DECL_OVERRIDE;

    bool visit(AST::BinaryExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::Block *ast) Q_DECL_OVERRIDE;
    bool visit(AST::WithStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::VoidExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::Expression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ExpressionStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::IfStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ForStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::LocalForStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::WhileStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::DoWhileStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::CaseBlock *ast) Q_DECL_OVERRIDE;
    bool visit(AST::NewExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::NewMemberExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::CallExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::StatementList *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ReturnStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::ThrowStatement *ast) Q_DECL_OVERRIDE;
    bool visit(AST::DeleteExpression *ast) Q_DECL_OVERRIDE;
    bool visit(AST::TypeOfExpression *ast) Q_DECL_OVERRIDE;

    void endVisit(QmlJS::AST::UiObjectInitializer *) Q_DECL_OVERRIDE;

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

#endif // QMLJSCHECK_H

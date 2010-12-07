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

#ifndef QMLJSCHECK_H
#define QMLJSCHECK_H

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/parser/qmljsastvisitor_p.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QSet>
#include <QtCore/QStack>
#include <QtGui/QColor>

namespace QmlJS {

class QMLJS_EXPORT Check: protected AST::Visitor
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::Check)

    typedef QSet<QString> StringSet;

public:
    Check(Document::Ptr doc, const Snapshot &snapshot, const Interpreter::Context *linkedContextNoScope);
    virtual ~Check();

    QList<DiagnosticMessage> operator()();


    void setIgnoreTypeErrors(bool ignore)
    { _ignoreTypeErrors = ignore; }

    enum Option {
        WarnDangerousNonStrictEqualityChecks = 1 << 0,
        WarnAllNonStrictEqualityChecks       = 1 << 1,
        WarnBlocks                           = 1 << 2,
        WarnWith                             = 1 << 3,
        WarnVoid                             = 1 << 4,
        WarnCommaExpression                  = 1 << 5,
        WarnExpressionStatement              = 1 << 6,
        WarnAssignInCondition                = 1 << 7,
        WarnUseBeforeDeclaration             = 1 << 8,
        WarnDuplicateDeclaration             = 1 << 9,
        WarnDeclarationsNotStartOfFunction   = 1 << 10,
        WarnCaseWithoutFlowControlEnd        = 1 << 11,
    };
    Q_DECLARE_FLAGS(Options, Option);

protected:
    virtual bool preVisit(AST::Node *ast);
    virtual void postVisit(AST::Node *ast);

    virtual bool visit(AST::UiProgram *ast);
    virtual bool visit(AST::UiObjectDefinition *ast);
    virtual bool visit(AST::UiObjectBinding *ast);
    virtual bool visit(AST::UiScriptBinding *ast);
    virtual bool visit(AST::UiArrayBinding *ast);
    virtual bool visit(AST::IdentifierExpression *ast);
    virtual bool visit(AST::FieldMemberExpression *ast);
    virtual bool visit(AST::FunctionDeclaration *ast);
    virtual bool visit(AST::FunctionExpression *ast);
    virtual bool visit(AST::UiObjectInitializer *);

    virtual bool visit(AST::BinaryExpression *ast);
    virtual bool visit(AST::Block *ast);
    virtual bool visit(AST::WithStatement *ast);
    virtual bool visit(AST::VoidExpression *ast);
    virtual bool visit(AST::Expression *ast);
    virtual bool visit(AST::ExpressionStatement *ast);
    virtual bool visit(AST::IfStatement *ast);
    virtual bool visit(AST::ForStatement *ast);
    virtual bool visit(AST::LocalForStatement *ast);
    virtual bool visit(AST::WhileStatement *ast);
    virtual bool visit(AST::DoWhileStatement *ast);
    virtual bool visit(AST::CaseClause *ast);
    virtual bool visit(AST::DefaultClause *ast);

    virtual void endVisit(QmlJS::AST::UiObjectInitializer *);

private:
    void visitQmlObject(AST::Node *ast, AST::UiQualifiedId *typeId,
                        AST::UiObjectInitializer *initializer);
    const Interpreter::Value *checkScopeObjectMember(const AST::UiQualifiedId *id);
    void checkAssignInCondition(AST::ExpressionNode *condition);
    void checkEndsWithControlFlow(AST::StatementList *statements, AST::SourceLocation errorLoc);
    void checkProperty(QmlJS::AST::UiQualifiedId *);

    void warning(const AST::SourceLocation &loc, const QString &message);
    void error(const AST::SourceLocation &loc, const QString &message);

    AST::Node *parent(int distance = 0);

    Document::Ptr _doc;
    Snapshot _snapshot;

    Interpreter::Context _context;
    ScopeBuilder _scopeBuilder;

    QList<DiagnosticMessage> _messages;

    bool _ignoreTypeErrors;
    Options _options;

    const Interpreter::Value *_lastValue;
    QList<AST::Node *> _chain;
    QSet<QString> m_ids;
    QStack<StringSet> m_propertyStack;
};

QMLJS_EXPORT QColor toQColor(const QString &qmlColorString);

QMLJS_EXPORT AST::SourceLocation locationFromRange(const AST::SourceLocation &start,
                                                   const AST::SourceLocation &end);

QMLJS_EXPORT AST::SourceLocation fullLocationForQualifiedId(AST::UiQualifiedId *);

QMLJS_EXPORT DiagnosticMessage errorMessage(const AST::SourceLocation &loc,
                                            const QString &message);

template <class T>
DiagnosticMessage errorMessage(const T *node, const QString &message)
{
    return DiagnosticMessage(DiagnosticMessage::Error,
                             locationFromRange(node->firstSourceLocation(),
                                               node->lastSourceLocation()),
                             message);
}

} // namespace QmlJS

#endif // QMLJSCHECK_H

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsstaticanalysismessage.h>
#include <qmljs/parser/qmljsastvisitor_p.h>

#include <QCoreApplication>
#include <QSet>
#include <QStack>

namespace Utils { class QtcSettings; }

namespace QmlJS {

class Imports;

class QMLJS_EXPORT Check: protected AST::Visitor
{
    typedef QSet<QString> StringSet;

public:
    // prefer taking root scope chain?
    Check(Document::Ptr doc, const ContextPtr &context, Utils::QtcSettings *qtcSettings = nullptr);
    ~Check();

    QList<StaticAnalysis::Message> operator()();

    void enableMessage(StaticAnalysis::Type type);
    void disableMessage(StaticAnalysis::Type type);

    void enableQmlDesignerChecks();

    void enableQmlDesignerUiFileChecks();
    void disableQmlDesignerUiFileChecks();

    static QList<StaticAnalysis::Type> defaultDisabledMessages();
    static QList<StaticAnalysis::Type> defaultDisabledMessagesForNonQuickUi();
    static bool incompatibleDesignerQmlId(const QString &id);

protected:
    bool preVisit(AST::Node *ast) override;
    void postVisit(AST::Node *ast) override;

    bool visit(AST::UiProgram *ast) override;
    bool visit(AST::UiImport *ast) override;
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

    bool visit(AST::TemplateLiteral *ast) override;
    bool visit(AST::BinaryExpression *ast) override;
    bool visit(AST::Block *ast) override;
    bool visit(AST::WithStatement *ast) override;
    bool visit(AST::VoidExpression *ast) override;
    bool visit(AST::Expression *ast) override;
    bool visit(AST::ExpressionStatement *ast) override;
    bool visit(AST::IfStatement *ast) override;
    bool visit(AST::ForStatement *ast) override;
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

    void throwRecursionDepthError() override;
private:
    void visitQmlObject(AST::Node *ast, AST::UiQualifiedId *typeId,
                        AST::UiObjectInitializer *initializer);
    const Value *checkScopeObjectMember(const AST::UiQualifiedId *id);
    void checkAssignInCondition(AST::ExpressionNode *condition);
    void checkCaseFallthrough(AST::StatementList *statements, SourceLocation errorLoc, SourceLocation nextLoc);
    void checkProperty(QmlJS::AST::UiQualifiedId *);
    void checkNewExpression(AST::ExpressionNode *node);
    void checkBindingRhs(AST::Statement *statement);
    void checkExtraParentheses(AST::ExpressionNode *expression);

    void addMessages(const QList<StaticAnalysis::Message> &messages);
    void addMessage(const StaticAnalysis::Message &message);
    void addMessage(StaticAnalysis::Type type, const SourceLocation &location,
                    const QString &arg1 = QString(), const QString &arg2 = QString());

    void scanCommentsForAnnotations();
    void warnAboutUnnecessarySuppressions();

    bool isQtQuick2() const;
    bool isQtQuick2Ui() const;

    bool isCaseOrDefault(AST::Node *n);
    bool hasVarStatement(AST::Block *b) const;

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

    using ShortImportInfo = QPair<QString, LanguageUtils::ComponentVersion>;
    QList<ShortImportInfo> m_importInfo;

    class MessageTypeAndSuppression
    {
    public:
        SourceLocation suppressionSource;
        StaticAnalysis::Type type;
        bool wasSuppressed;
    };

    enum TranslationFunction { qsTr, qsTrId, qsTranslate, noTranslationfunction };

    QHash< int, QList<MessageTypeAndSuppression> > m_disabledMessageTypesByLine;

    bool _importsOk;
    bool _inStatementBinding;
    int _componentChildCount = 0;
    const Imports *_imports;
    TranslationFunction lastTransLationfunction = noTranslationfunction;
};

} // namespace QmlJS

// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsstaticanalysismessage.h>

#include <languageutils/componentversion.h>

#include <QCoreApplication>
#include <QSet>
#include <QStack>

namespace Utils { class QtcSettings; }

namespace QmlDesigner {

class AstCheck : protected QmlJS::AST::Visitor
{
    typedef QSet<QString> StringSet;

public:
    // prefer taking root scope chain?
    AstCheck(QmlJS::Document::Ptr doc);
    ~AstCheck();

    QList<QmlJS::StaticAnalysis::Message> operator()();

    void enableMessage(QmlJS::StaticAnalysis::Type type);
    void disableMessage(QmlJS::StaticAnalysis::Type type);

    void enableQmlDesignerChecks();

    void enableQmlDesignerUiFileChecks();
    void disableQmlDesignerUiFileChecks();

    static QList<QmlJS::StaticAnalysis::Type> defaultDisabledMessages();
    static QList<QmlJS::StaticAnalysis::Type> defaultDisabledMessagesForNonQuickUi();
    static bool incompatibleDesignerQmlId(const QString &id);

protected:
    bool preVisit(QmlJS::AST::Node *ast) override;
    void postVisit(QmlJS::AST::Node *ast) override;

    bool visit(QmlJS::AST::UiProgram *ast) override;
    bool visit(QmlJS::AST::UiImport *ast) override;
    bool visit(QmlJS::AST::UiObjectDefinition *ast) override;
    bool visit(QmlJS::AST::UiObjectBinding *ast) override;
    bool visit(QmlJS::AST::UiScriptBinding *ast) override;
    bool visit(QmlJS::AST::UiArrayBinding *ast) override;
    bool visit(QmlJS::AST::UiPublicMember *ast) override;
    bool visit(QmlJS::AST::IdentifierExpression *ast) override;
    bool visit(QmlJS::AST::FieldMemberExpression *ast) override;
    bool visit(QmlJS::AST::FunctionDeclaration *ast) override;
    bool visit(QmlJS::AST::FunctionExpression *ast) override;
    bool visit(QmlJS::AST::UiObjectInitializer *) override;
    bool visit(QmlJS::AST::UiEnumDeclaration *ast) override;
    bool visit(QmlJS::AST::UiEnumMemberList *ast) override;

    bool visit(QmlJS::AST::TemplateLiteral *ast) override;
    bool visit(QmlJS::AST::BinaryExpression *ast) override;
    bool visit(QmlJS::AST::Block *ast) override;
    bool visit(QmlJS::AST::WithStatement *ast) override;
    bool visit(QmlJS::AST::VoidExpression *ast) override;
    bool visit(QmlJS::AST::Expression *ast) override;
    bool visit(QmlJS::AST::ExpressionStatement *ast) override;
    bool visit(QmlJS::AST::IfStatement *ast) override;
    bool visit(QmlJS::AST::ForStatement *ast) override;
    bool visit(QmlJS::AST::WhileStatement *ast) override;
    bool visit(QmlJS::AST::DoWhileStatement *ast) override;
    bool visit(QmlJS::AST::CaseBlock *ast) override;
    bool visit(QmlJS::AST::NewExpression *ast) override;
    bool visit(QmlJS::AST::NewMemberExpression *ast) override;
    bool visit(QmlJS::AST::CallExpression *ast) override;
    bool visit(QmlJS::AST::StatementList *ast) override;
    bool visit(QmlJS::AST::ReturnStatement *ast) override;
    bool visit(QmlJS::AST::ThrowStatement *ast) override;
    bool visit(QmlJS::AST::DeleteExpression *ast) override;
    bool visit(QmlJS::AST::TypeOfExpression *ast) override;

    void endVisit(QmlJS::AST::UiObjectInitializer *) override;

    void throwRecursionDepthError() override;
private:
    void visitQmlObject(QmlJS::AST::Node *ast,
                        QmlJS::AST::UiQualifiedId *typeId,
                        QmlJS::AST::UiObjectInitializer *initializer);

    //void checkAssignInCondition(const Value *checkScopeObjectMember(const AST::UiQualifiedId *id);
    //                            AST::ExpressionNode * condition);
    void checkAssignInCondition(QmlJS::AST::ExpressionNode *condition);

    void checkProperty(QmlJS::AST::UiQualifiedId *);
    void checkNewExpression(QmlJS::AST::ExpressionNode *node);
    void checkBindingRhs(QmlJS::AST::Statement *statement);
    void checkExtraParentheses(QmlJS::AST::ExpressionNode *expression);

    void addMessages(const QList<QmlJS::StaticAnalysis::Message> &messages);
    void addMessage(const QmlJS::StaticAnalysis::Message &message);
    void addMessage(QmlJS::StaticAnalysis::Type type,
                    const QmlJS::SourceLocation &location,
                    const QString &arg1 = QString(),
                    const QString &arg2 = QString());

    bool isQtQuickUi() const;

    bool isCaseOrDefault(QmlJS::AST::Node *n);
    bool hasVarStatement(QmlJS::AST::Block *b) const;

    QmlJS::AST::Node *parent(int distance = 0);

    QmlJS::Document::Ptr m_document;

    QList<QmlJS::StaticAnalysis::Message> m_messages;
    QSet<QmlJS::StaticAnalysis::Type> m_enabledMessages;

    QList<QmlJS::AST::Node *> m_chain;
    QStack<StringSet> m_idStack;
    QStack<StringSet> m_propertyStack;
    QStack<QString> m_typeStack;

    using ShortImportInfo = QPair<QString, LanguageUtils::ComponentVersion>;
    QList<ShortImportInfo> m_importInfo;

    class MessageTypeAndSuppression
    {
    public:
        QmlJS::SourceLocation suppressionSource;
        QmlJS::StaticAnalysis::Type type;
        bool wasSuppressed;
    };

    enum TranslationFunction { qsTr, qsTrId, qsTranslate, noTranslationfunction };

    QHash< int, QList<MessageTypeAndSuppression> > m_disabledMessageTypesByLine;

    bool m_importsOk = false;
    bool m_inStatementBinding = false;
    int m_componentChildCount = 0;
    TranslationFunction m_lastTransLationfunction = noTranslationfunction;
};

} // namespace QmlDesigner

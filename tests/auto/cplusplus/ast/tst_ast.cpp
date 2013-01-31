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

#include <QtTest>
#include <QDebug>

#include <Control.h>
#include <Literals.h>
#include <Parser.h>
#include <AST.h>

//TESTED_COMPONENT=src/libs/cplusplus
using namespace CPlusPlus;

class tst_AST: public QObject
{
    Q_OBJECT

    Control control;

public:

    TranslationUnit *parse(const QByteArray &source,
                           TranslationUnit::ParseMode mode,
                           bool blockErrors = false,
                           bool qtMocRun = false)
    {
        const StringLiteral *fileId = control.stringLiteral("<stdin>");
        TranslationUnit *unit = new TranslationUnit(&control, fileId);
        unit->setObjCEnabled(true);
        unit->setQtMocRunEnabled(qtMocRun);
        unit->setSource(source.constData(), source.length());
        unit->blockErrors(blockErrors);
        unit->parse(mode);
        return unit;
    }

    TranslationUnit *parseDeclaration(const QByteArray &source, bool blockErrors = false, bool qtMocRun = false)
    { return parse(source, TranslationUnit::ParseDeclaration, blockErrors, qtMocRun); }

    TranslationUnit *parseExpression(const QByteArray &source)
    { return parse(source, TranslationUnit::ParseExpression); }

    TranslationUnit *parseStatement(const QByteArray &source)
    { return parse(source, TranslationUnit::ParseStatement); }
    
    class Diagnostic: public DiagnosticClient {
    public:
        int errorCount;

        Diagnostic()
            : errorCount(0)
        { }

        virtual void report(int /*level*/,
                            const StringLiteral *fileName,
                            unsigned line, unsigned column,
                            const char *format, va_list ap)
        {
            ++errorCount;

            qDebug() << fileName->chars()<<':'<<line<<':'<<column<<' '<<QString().vsprintf(format, ap);
        }
    };

    Diagnostic diag;    
    

private slots:
    void initTestCase();
    // declarations
    void gcc_attributes_1();
    void gcc_attributes_2();
    void gcc_attributes_3();
    void crash_test_1();

    // expressions
    void simple_name_1();
    void template_id_1();
    void new_expression_1();
    void new_expression_2();
    void condition_1();
    void init_1();
    void conditional_1();
    void throw_1();
    void templated_dtor_1();
    void templated_dtor_2();
    void templated_dtor_3();
    void templated_dtor_4();
    void templated_dtor_5();

    // possible declaration-or-expression statements
    void call_call_1();

    // statements
    void if_statement_1();
    void if_statement_2();
    void if_statement_3();
    void if_else_statement();
    void while_statement();
    void while_condition_statement();
    void for_statement();
    void cpp_initializer_or_function_declaration();
    void simple_declaration_1();
    void function_call_1();
    void function_call_2();
    void function_call_3();
    void function_call_4();
    void nested_deref_expression();
    void assignment_1();
    void assignment_2();

    // constructor declarations
    void cpp_constructor_one_unamed_arg();
    void cpp_constructor_one_unamed_arg_namespace();
    void cpp_constructor_one_knowntype_arg();
    void cpp_constructor_one_const_arg();
    void cpp_constructor_one_ref_arg();
    void cpp_constructor_one_named_arg();
    void cpp_constructor_no_arg();
    void cpp_constructor_multiple_args();

    // objc++
    void objc_simple_class();
    void objc_attributes_followed_by_at_keyword();
    void objc_protocol_forward_declaration_1();
    void objc_protocol_definition_1();
    void objc_method_attributes_1();
    void objc_selector_error_recovery_1();
    void objc_selector_error_recovery_2();

    // expressions with (square) brackets
    void normal_array_access();
    void array_access_with_nested_expression();
    void objc_msg_send_expression();
    void objc_msg_send_expression_without_selector();

    // Qt "keywords"
    void q_enum_1();
};

void tst_AST::gcc_attributes_1()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n"
"static inline void *__attribute__((__always_inline__)) _mm_malloc(size_t size, size_t align);"
                                                          ));
}

void tst_AST::gcc_attributes_2()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration(
                                             "\nnamespace std __attribute__ ((__visibility__ (\"default\"))) {\n}\n"
                                             ));
    AST *ast = unit->ast();
    QVERIFY(ast);
    NamespaceAST* ns = ast->asNamespace();
    QVERIFY(ns);
    QCOMPARE(unit->spell(ns->identifier_token), "std");
    QVERIFY(ns->attribute_list);
    QVERIFY(!ns->attribute_list->next);
    QVERIFY(ns->attribute_list->value);
    AttributeSpecifierAST *attrSpec = ns->attribute_list->value->asAttributeSpecifier();
    QVERIFY(attrSpec);
    QVERIFY(attrSpec->attribute_list);
    QVERIFY(!attrSpec->attribute_list->next);
    QVERIFY(attrSpec->attribute_list->value);
    AttributeAST *attr = attrSpec->attribute_list->value->asAttribute();
    QVERIFY(attr);
    QCOMPARE(unit->spell(attr->identifier_token), "__visibility__");
    QVERIFY(attr->expression_list);
    QVERIFY(!attr->expression_list->next);
    QVERIFY(attr->expression_list->value);
    StringLiteralAST *e = attr->expression_list->value->asStringLiteral();
    QVERIFY(e);
    QVERIFY(!e->next);
    QCOMPARE(unit->spell(e->literal_token), "default");
}

void tst_AST::gcc_attributes_3()
{
    const char *inp = "\nnamespace std X {\n}\n";
    QSharedPointer<TranslationUnit> unit(parseDeclaration(inp));
    AST *ast = unit->ast();
    QVERIFY(ast);
    NamespaceAST* ns = ast->asNamespace();
    QVERIFY(ns);
    QCOMPARE(unit->spell(ns->identifier_token), "std");
    QVERIFY(!ns->attribute_list);
    QVERIFY(ns->linkage_body);
    LinkageBodyAST *link = ns->linkage_body->asLinkageBody();
    QVERIFY(link);
    QCOMPARE(unit->tokenKind(link->lbrace_token), (int) T_LBRACE);
    QVERIFY(!link->declaration_list);
    QCOMPARE(unit->tokenKind(link->rbrace_token), (int) T_RBRACE);
}

void tst_AST::crash_test_1()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("decltype auto\n"));
    AST *ast = unit->ast();
    QVERIFY(ast);
}

void tst_AST::simple_declaration_1()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("\n"
"a * b = 10;"
    ));

    AST *ast = unit->ast();
    QVERIFY(ast);

    DeclarationStatementAST *declStmt = ast->asDeclarationStatement();
    QVERIFY(declStmt);
}

void tst_AST::simple_name_1()
{
    QSharedPointer<TranslationUnit> unit(parseExpression("a"));
    AST *ast = unit->ast();

    QVERIFY(ast != 0);
    QVERIFY(ast->asIdExpression()->name->asSimpleName() != 0);
    QCOMPARE(ast->asIdExpression()->name->asSimpleName()->identifier_token, 1U);
}

void tst_AST::template_id_1()
{
    QSharedPointer<TranslationUnit> unit(parseExpression("list<10>"));
    AST *ast = unit->ast();

    QVERIFY(ast != 0);
    QVERIFY(ast->asIdExpression()->name->asTemplateId() != 0);
    QCOMPARE(ast->asIdExpression()->name->asTemplateId()->identifier_token, 1U);
    QCOMPARE(ast->asIdExpression()->name->asTemplateId()->less_token, 2U);
    QVERIFY(ast->asIdExpression()->name->asTemplateId()->template_argument_list != 0);
    QVERIFY(ast->asIdExpression()->name->asTemplateId()->template_argument_list->value != 0);
    QVERIFY(ast->asIdExpression()->name->asTemplateId()->template_argument_list->value->asNumericLiteral() != 0);
    QCOMPARE(ast->asIdExpression()->name->asTemplateId()->template_argument_list->value->asNumericLiteral()->literal_token, 3U);
    QVERIFY(ast->asIdExpression()->name->asTemplateId()->template_argument_list->next == 0);
    QCOMPARE(ast->asIdExpression()->name->asTemplateId()->greater_token, 4U);
}

void tst_AST::new_expression_1()
{
    QSharedPointer<TranslationUnit> unit(parseExpression("\n"
"new char"
    ));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    NewExpressionAST *expr = ast->asNewExpression();
    QVERIFY(expr != 0);

    QCOMPARE(expr->scope_token, 0U);
    QCOMPARE(expr->new_token, 1U);
    QVERIFY(expr->new_placement == 0);
    QCOMPARE(expr->lparen_token, 0U);
    QVERIFY(expr->type_id == 0);
    QCOMPARE(expr->rparen_token, 0U);
    QVERIFY(expr->new_type_id != 0);
    QVERIFY(expr->new_initializer == 0);

    QVERIFY(expr->new_type_id->type_specifier_list != 0);
    QVERIFY(expr->new_type_id->ptr_operator_list == 0);
    QVERIFY(expr->new_type_id->new_array_declarator_list == 0);
}

void tst_AST::new_expression_2()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("\n"
"::new(__p) _Tp(__val);"
    ));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    ExpressionStatementAST *stmt = ast->asExpressionStatement();
    QVERIFY(stmt != 0);
    QVERIFY(stmt->expression != 0);
    QVERIFY(stmt->semicolon_token != 0);

    NewExpressionAST *expr = stmt->expression->asNewExpression();
    QVERIFY(expr != 0);

    QCOMPARE(expr->scope_token, 1U);
    QCOMPARE(expr->new_token, 2U);
    QVERIFY(expr->new_placement != 0);
    QCOMPARE(expr->lparen_token, 0U);
    QVERIFY(expr->type_id == 0);
    QCOMPARE(expr->rparen_token, 0U);
    QVERIFY(expr->new_type_id != 0);
    QVERIFY(expr->new_initializer != 0);
}

void tst_AST::condition_1()
{
    QSharedPointer<TranslationUnit> unit(parseExpression("\n"
                                                         "(x < 0 && y > (int) a)"
    ));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);
    NestedExpressionAST *nestedExpr = ast->asNestedExpression();
    QVERIFY(nestedExpr);
    QVERIFY(nestedExpr->expression);
    BinaryExpressionAST *andExpr = nestedExpr->expression->asBinaryExpression();
    QVERIFY(andExpr);
    QCOMPARE(unit->tokenKind(andExpr->binary_op_token), (int) T_AMPER_AMPER);
    QVERIFY(andExpr->left_expression);
    QVERIFY(andExpr->right_expression);

    BinaryExpressionAST *ltExpr = andExpr->left_expression->asBinaryExpression();
    QVERIFY(ltExpr);
    QCOMPARE(unit->tokenKind(ltExpr->binary_op_token), (int) T_LESS);
    QVERIFY(ltExpr->left_expression);
    QVERIFY(ltExpr->right_expression);

    SimpleNameAST *x = ltExpr->left_expression->asIdExpression()->name->asSimpleName();
    QVERIFY(x);
    QCOMPARE(unit->spell(x->identifier_token), "x");

    NumericLiteralAST *zero = ltExpr->right_expression->asNumericLiteral();
    QVERIFY(zero);
    QCOMPARE(unit->spell(zero->literal_token), "0");

    BinaryExpressionAST *gtExpr = andExpr->right_expression->asBinaryExpression();
    QVERIFY(gtExpr);
    QCOMPARE(unit->tokenKind(gtExpr->binary_op_token), (int) T_GREATER);
    QVERIFY(gtExpr->left_expression);
    QVERIFY(gtExpr->right_expression);

    SimpleNameAST *y = gtExpr->left_expression->asIdExpression()->name->asSimpleName();
    QVERIFY(y);
    QCOMPARE(unit->spell(y->identifier_token), "y");

    CastExpressionAST *cast = gtExpr->right_expression->asCastExpression();
    QVERIFY(cast);
    QVERIFY(cast->type_id);
    QVERIFY(cast->expression);

    TypeIdAST *intType = cast->type_id->asTypeId();
    QVERIFY(intType);
    // ### here we could check if the type is an actual int

    SimpleNameAST *a = cast->expression->asIdExpression()->name->asSimpleName();
    QVERIFY(a);
    QCOMPARE(unit->spell(a->identifier_token), "a");
}

void tst_AST::init_1()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n"
                                                          "x y[] = { X<10>, y };"
    ));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);
}

void tst_AST::conditional_1()
{
    QSharedPointer<TranslationUnit> unit(parseExpression("\n"
                                                         "(x < 0 && y > (int) a) ? x == 1 : y = 1"
    ));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    ConditionalExpressionAST *conditional = ast->asConditionalExpression();
    QVERIFY(conditional);
    QVERIFY(conditional->condition);
    QVERIFY(conditional->left_expression);
    QVERIFY(conditional->right_expression);

    NestedExpressionAST *nestedExpr = conditional->condition->asNestedExpression();
    QVERIFY(nestedExpr);
    QVERIFY(nestedExpr->expression);
    BinaryExpressionAST *andExpr = nestedExpr->expression->asBinaryExpression();
    QVERIFY(andExpr);
    QCOMPARE(unit->tokenKind(andExpr->binary_op_token), (int) T_AMPER_AMPER);
    QVERIFY(andExpr->left_expression);
    QVERIFY(andExpr->right_expression);

    BinaryExpressionAST *ltExpr = andExpr->left_expression->asBinaryExpression();
    QVERIFY(ltExpr);
    QCOMPARE(unit->tokenKind(ltExpr->binary_op_token), (int) T_LESS);
    QVERIFY(ltExpr->left_expression);
    QVERIFY(ltExpr->right_expression);

    SimpleNameAST *x = ltExpr->left_expression->asIdExpression()->name->asSimpleName();
    QVERIFY(x);
    QCOMPARE(unit->spell(x->identifier_token), "x");

    NumericLiteralAST *zero = ltExpr->right_expression->asNumericLiteral();
    QVERIFY(zero);
    QCOMPARE(unit->spell(zero->literal_token), "0");

    BinaryExpressionAST *gtExpr = andExpr->right_expression->asBinaryExpression();
    QVERIFY(gtExpr);
    QCOMPARE(unit->tokenKind(gtExpr->binary_op_token), (int) T_GREATER);
    QVERIFY(gtExpr->left_expression);
    QVERIFY(gtExpr->right_expression);

    SimpleNameAST *y = gtExpr->left_expression->asIdExpression()->name->asSimpleName();
    QVERIFY(y);
    QCOMPARE(unit->spell(y->identifier_token), "y");

    CastExpressionAST *cast = gtExpr->right_expression->asCastExpression();
    QVERIFY(cast);
    QVERIFY(cast->type_id);
    QVERIFY(cast->expression);

    TypeIdAST *intType = cast->type_id->asTypeId();
    QVERIFY(intType);
    QVERIFY(! (intType->declarator));
    QVERIFY(intType->type_specifier_list);
    QVERIFY(! (intType->type_specifier_list->next));
    QVERIFY(intType->type_specifier_list->value);
    SimpleSpecifierAST *intSpec = intType->type_specifier_list->value->asSimpleSpecifier();
    QVERIFY(intSpec);
    QCOMPARE(unit->spell(intSpec->specifier_token), "int");

    SimpleNameAST *a = cast->expression->asIdExpression()->name->asSimpleName();
    QVERIFY(a);
    QCOMPARE(unit->spell(a->identifier_token), "a");

    BinaryExpressionAST *equals = conditional->left_expression->asBinaryExpression();
    QVERIFY(equals);
    QCOMPARE(unit->tokenKind(equals->binary_op_token), (int) T_EQUAL_EQUAL);

    x = equals->left_expression->asIdExpression()->name->asSimpleName();
    QVERIFY(x);
    QCOMPARE(unit->spell(x->identifier_token), "x");

    NumericLiteralAST *one = equals->right_expression->asNumericLiteral();
    QVERIFY(one);
    QCOMPARE(unit->spell(one->literal_token), "1");

    BinaryExpressionAST *assignment = conditional->right_expression->asBinaryExpression();
    QVERIFY(assignment);
    QCOMPARE(unit->tokenKind(assignment->binary_op_token), (int) T_EQUAL);

    y = assignment->left_expression->asIdExpression()->name->asSimpleName();
    QVERIFY(y);
    QCOMPARE(unit->spell(y->identifier_token), "y");

    one = assignment->right_expression->asNumericLiteral();
    QVERIFY(one);
    QCOMPARE(unit->spell(one->literal_token), "1");
}

void tst_AST::throw_1()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("throw 1;"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);
    QVERIFY(ast->asExpressionStatement());
}

void tst_AST::templated_dtor_1()
{
    QSharedPointer<TranslationUnit> unit(parseStatement(
                                             "\n"
                                             "a.b::~b<c>();"
                                             ));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);
    ExpressionStatementAST *e = ast->asExpressionStatement();
    QVERIFY(e);
    QVERIFY(e->expression);
    CallAST *call = e->expression->asCall();
    QVERIFY(call);
    QVERIFY(!call->expression_list);
    QVERIFY(call->base_expression);
    MemberAccessAST *mem = call->base_expression->asMemberAccess();
    QVERIFY(mem);
    QCOMPARE(unit->spell(mem->base_expression->asIdExpression()->name->asSimpleName()->identifier_token), "a");

    QualifiedNameAST *qName = mem->member_name->asQualifiedName();
    QVERIFY(qName);
    QVERIFY(qName->nested_name_specifier_list);
    QVERIFY(!qName->nested_name_specifier_list->next);
    QVERIFY(qName->nested_name_specifier_list->value);
    QVERIFY(qName->nested_name_specifier_list->value->class_or_namespace_name);
    SimpleNameAST *bName = qName->nested_name_specifier_list->value->class_or_namespace_name->asSimpleName();
    QVERIFY(bName);
    QCOMPARE(unit->spell(bName->identifier_token), "b");

    QVERIFY(qName->unqualified_name);
    DestructorNameAST *dtor = qName->unqualified_name->asDestructorName();
    QVERIFY(dtor);
    QVERIFY(dtor->unqualified_name);
    TemplateIdAST *tid = dtor->unqualified_name->asTemplateId();
    QVERIFY(tid);
    QCOMPARE(unit->spell(tid->identifier_token), "b");

    QVERIFY(tid->template_argument_list);
    QVERIFY(!tid->template_argument_list->next);
    QVERIFY(tid->template_argument_list->value);
    TypeIdAST *typeId = tid->template_argument_list->value->asTypeId();
    QVERIFY(typeId);
    QVERIFY(!typeId->declarator);
    QVERIFY(typeId->type_specifier_list);
    QVERIFY(!typeId->type_specifier_list->next);
    QVERIFY(typeId->type_specifier_list->value);
    NamedTypeSpecifierAST *nts = typeId->type_specifier_list->value->asNamedTypeSpecifier();
    QVERIFY(nts);
    QVERIFY(nts->name);
    SimpleNameAST *cName = nts->name->asSimpleName();
    QVERIFY(cName);
    QCOMPARE(unit->spell(cName->identifier_token), "c");
}

void tst_AST::templated_dtor_2()
{
    QSharedPointer<TranslationUnit> unit(parseStatement(
                                             "\n"
                                             "a.~b<c>();"
                                             ));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);
    ExpressionStatementAST *e = ast->asExpressionStatement();
    QVERIFY(e);
    QVERIFY(e->expression);
    CallAST *call = e->expression->asCall();
    QVERIFY(call);
    QVERIFY(!call->expression_list);
    QVERIFY(call->base_expression);
    MemberAccessAST *mem = call->base_expression->asMemberAccess();
    QVERIFY(mem);
    QCOMPARE(unit->spell(mem->base_expression->asIdExpression()->name->asSimpleName()->identifier_token), "a");

    QVERIFY(mem->member_name);
    DestructorNameAST *dtor = mem->member_name->asDestructorName();
    QVERIFY(dtor);
    QVERIFY(dtor->unqualified_name);
    TemplateIdAST *tid = dtor->unqualified_name->asTemplateId();
    QVERIFY(tid);
    QCOMPARE(unit->spell(tid->identifier_token), "b");

    QVERIFY(tid->template_argument_list);
    QVERIFY(!tid->template_argument_list->next);
    QVERIFY(tid->template_argument_list->value);
    TypeIdAST *typeId = tid->template_argument_list->value->asTypeId();
    QVERIFY(typeId);
    QVERIFY(!typeId->declarator);
    QVERIFY(typeId->type_specifier_list);
    QVERIFY(!typeId->type_specifier_list->next);
    QVERIFY(typeId->type_specifier_list->value);
    NamedTypeSpecifierAST *nts = typeId->type_specifier_list->value->asNamedTypeSpecifier();
    QVERIFY(nts);
    QVERIFY(nts->name);
    SimpleNameAST *cName = nts->name->asSimpleName();
    QVERIFY(cName);
    QCOMPARE(unit->spell(cName->identifier_token), "c");
}

void tst_AST::templated_dtor_3()
{
    QSharedPointer<TranslationUnit> unit(parseStatement(
                                             "\n"
                                             "a::~b<c>();"
                                             ));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);
    ExpressionStatementAST *e = ast->asExpressionStatement();
    QVERIFY(e);
    QVERIFY(e->expression);
    CallAST *call = e->expression->asCall();
    QVERIFY(call);
    QVERIFY(!call->expression_list);
    QVERIFY(call->base_expression);
    IdExpressionAST *idExpr = call->base_expression->asIdExpression();
    QVERIFY(idExpr);
    QVERIFY(idExpr->name);
    QualifiedNameAST *qName = idExpr->name->asQualifiedName();
    QVERIFY(qName);
    QVERIFY(qName->nested_name_specifier_list);
    QVERIFY(!qName->nested_name_specifier_list->next);
    QVERIFY(qName->nested_name_specifier_list->value);
    QVERIFY(qName->nested_name_specifier_list->value->class_or_namespace_name);
    SimpleNameAST *bName = qName->nested_name_specifier_list->value->class_or_namespace_name->asSimpleName();
    QVERIFY(bName);
    QCOMPARE(unit->spell(bName->identifier_token), "a");

    QVERIFY(qName->unqualified_name);
    DestructorNameAST *dtor = qName->unqualified_name->asDestructorName();
    QVERIFY(dtor);
    QVERIFY(dtor->unqualified_name);
    TemplateIdAST *tid = dtor->unqualified_name->asTemplateId();
    QVERIFY(tid);
    QCOMPARE(unit->spell(tid->identifier_token), "b");

    QVERIFY(tid->template_argument_list);
    QVERIFY(!tid->template_argument_list->next);
    QVERIFY(tid->template_argument_list->value);
    TypeIdAST *typeId = tid->template_argument_list->value->asTypeId();
    QVERIFY(typeId);
    QVERIFY(!typeId->declarator);
    QVERIFY(typeId->type_specifier_list);
    QVERIFY(!typeId->type_specifier_list->next);
    QVERIFY(typeId->type_specifier_list->value);
    NamedTypeSpecifierAST *nts = typeId->type_specifier_list->value->asNamedTypeSpecifier();
    QVERIFY(nts);
    QVERIFY(nts->name);
    SimpleNameAST *cName = nts->name->asSimpleName();
    QVERIFY(cName);
    QCOMPARE(unit->spell(cName->identifier_token), "c");
}

void tst_AST::templated_dtor_4()
{
    QSharedPointer<TranslationUnit> unit(parseStatement(
                                             "\n"
                                             "~b<c>();"
                                             ));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);
    ExpressionStatementAST *e = ast->asExpressionStatement();
    QVERIFY(e);
    QVERIFY(e->expression);
    UnaryExpressionAST *u = e->expression->asUnaryExpression();
    QVERIFY(u);
    QCOMPARE(unit->tokenKind(u->unary_op_token), (int) T_TILDE);
    QVERIFY(u->expression);
    CallAST *call = u->expression->asCall();
    QVERIFY(call);
    QVERIFY(call->base_expression);
    IdExpressionAST *idExpr = call->base_expression->asIdExpression();
    QVERIFY(idExpr);
    QVERIFY(idExpr->name);
    TemplateIdAST *tid = idExpr->name->asTemplateId();
    QVERIFY(tid);
    QCOMPARE(unit->spell(tid->identifier_token), "b");
    QVERIFY(tid->template_argument_list);
    QVERIFY(!tid->template_argument_list->next);
    QVERIFY(tid->template_argument_list->value);
    TypeIdAST *typeId = tid->template_argument_list->value->asTypeId();
    QVERIFY(typeId);
    QVERIFY(!typeId->declarator);
    QVERIFY(typeId->type_specifier_list);
    QVERIFY(!typeId->type_specifier_list->next);
    QVERIFY(typeId->type_specifier_list->value);
    NamedTypeSpecifierAST *nts = typeId->type_specifier_list->value->asNamedTypeSpecifier();
    QVERIFY(nts);
    QVERIFY(nts->name);
    SimpleNameAST *cName = nts->name->asSimpleName();
    QVERIFY(cName);
    QCOMPARE(unit->spell(cName->identifier_token), "c");
}

void tst_AST::templated_dtor_5()
{
    QSharedPointer<TranslationUnit> unit(parseExpression(
                                             "\n"
                                             "~a < b()"
                                             ));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    BinaryExpressionAST *binExpr = ast->asBinaryExpression();
    QVERIFY(binExpr);
    QVERIFY(binExpr->left_expression);
    UnaryExpressionAST *notExpr = binExpr->left_expression->asUnaryExpression();
    QVERIFY(notExpr);
    QCOMPARE(unit->tokenKind(notExpr->unary_op_token), (int) T_TILDE);

    CallAST *call = binExpr->right_expression->asCall();
    QVERIFY(call->base_expression);
    QVERIFY(!call->expression_list);
    IdExpressionAST *bExpr = call->base_expression->asIdExpression();
    QVERIFY(bExpr);
    SimpleNameAST *bName = bExpr->name->asSimpleName();
    QVERIFY(bName);
    QCOMPARE(unit->spell(bName->identifier_token), "b");
}

void tst_AST::call_call_1()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("method()->call();"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    ExpressionStatementAST *exprStmt = ast->asExpressionStatement();
    QVERIFY(exprStmt);

    ExpressionAST *expr = exprStmt->expression;
    QVERIFY(expr);

    CallAST *memberCall = expr->asCall();
    QVERIFY(memberCall);
    QVERIFY(memberCall->base_expression);
    QVERIFY(!memberCall->expression_list);

    MemberAccessAST *member_xs = memberCall->base_expression->asMemberAccess();
    QVERIFY(member_xs);
    QVERIFY(member_xs->member_name);
    SimpleNameAST *member_name = member_xs->member_name->asSimpleName();
    QVERIFY(member_name);
    QVERIFY(member_name->identifier_token);
    QCOMPARE(unit->spell(member_name->identifier_token), "call");

    QCOMPARE(unit->tokenKind(member_xs->access_token), (int) T_ARROW);

    QVERIFY(member_xs->base_expression);
    CallAST *method_call = member_xs->base_expression->asCall();
    QVERIFY(method_call);
    QVERIFY(!method_call->expression_list);
    QVERIFY(method_call->base_expression);
    IdExpressionAST *member_expr = method_call->base_expression->asIdExpression();
    QVERIFY(member_expr);
    QVERIFY(member_expr->name);
    SimpleNameAST *member_name2 = member_expr->name->asSimpleName();
    QVERIFY(member_name2);
    QVERIFY(member_name2->identifier_token);
    QCOMPARE(unit->spell(member_name2->identifier_token), "method");
}

void tst_AST::function_call_1()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("retranslateUi(blah);"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);
    QVERIFY(ast->asExpressionStatement());
}

void tst_AST::function_call_2()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("retranslateUi(10);"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);
    QVERIFY(ast->asExpressionStatement());
}

void tst_AST::function_call_3()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("advance();"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);
    QVERIFY(ast->asExpressionStatement());
}

void tst_AST::function_call_4()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("checkPropertyAttribute(attrAst, propAttrs, ReadWrite);"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);
    QVERIFY(ast->asExpressionStatement());
}

void tst_AST::nested_deref_expression()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("(*blah);"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);
    QVERIFY(ast->asExpressionStatement());
}

void tst_AST::assignment_1()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("a(x) = 3;"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);
    QVERIFY(ast->asExpressionStatement());
}

void tst_AST::assignment_2()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("(*blah) = 10;"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);
    QVERIFY(ast->asExpressionStatement());
}

void tst_AST::if_statement_1()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("if (a) b;"));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    IfStatementAST *stmt = ast->asIfStatement();
    QVERIFY(stmt != 0);
    QCOMPARE(stmt->if_token, 1U);
    QCOMPARE(stmt->lparen_token, 2U);
    QVERIFY(stmt->condition != 0);
    QCOMPARE(stmt->rparen_token, 4U);
    QVERIFY(stmt->statement != 0);
    QCOMPARE(stmt->else_token, 0U);
    QVERIFY(stmt->else_statement == 0);

    // check the `then' statement1
    ExpressionStatementAST *then_stmt = stmt->statement->asExpressionStatement();
    QVERIFY(then_stmt != 0);
    QVERIFY(then_stmt->expression != 0);
    QCOMPARE(then_stmt->semicolon_token, 6U);

    SimpleNameAST *id_expr = then_stmt->expression->asIdExpression()->name->asSimpleName();
    QVERIFY(id_expr != 0);
    QCOMPARE(id_expr->identifier_token, 5U);
}

void tst_AST::if_statement_2()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("if (x<0 && y>a);"));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    IfStatementAST *stmt = ast->asIfStatement();
    QVERIFY(stmt != 0);

    QVERIFY(stmt->condition);
    QVERIFY(stmt->condition->asBinaryExpression());
    QCOMPARE(unit->tokenKind(stmt->condition->asBinaryExpression()->binary_op_token), int(T_AMPER_AMPER));
}

void tst_AST::if_statement_3()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("if (x<0 && x<0 && x<0 && x<0 && x<0 && x<0 && x<0);"));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    IfStatementAST *stmt = ast->asIfStatement();
    QVERIFY(stmt != 0);

    QVERIFY(stmt->condition);
}

void tst_AST::if_else_statement()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("if (a) b; else c;"));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    IfStatementAST *stmt = ast->asIfStatement();
    QVERIFY(stmt != 0);
    QCOMPARE(stmt->if_token, 1U);
    QCOMPARE(stmt->lparen_token, 2U);
    QVERIFY(stmt->condition != 0);
    QCOMPARE(stmt->rparen_token, 4U);
    QVERIFY(stmt->statement != 0);
    QCOMPARE(stmt->else_token, 7U);
    QVERIFY(stmt->else_statement != 0);

    // check the `then' statement
    ExpressionStatementAST *then_stmt = stmt->statement->asExpressionStatement();
    QVERIFY(then_stmt != 0);
    QVERIFY(then_stmt->expression != 0);
    QCOMPARE(then_stmt->semicolon_token, 6U);

    SimpleNameAST *a_id_expr = then_stmt->expression->asIdExpression()->name->asSimpleName();
    QVERIFY(a_id_expr != 0);
    QCOMPARE(a_id_expr->identifier_token, 5U);

    // check the `then' statement
    ExpressionStatementAST *else_stmt = stmt->else_statement->asExpressionStatement();
    QVERIFY(else_stmt != 0);
    QVERIFY(else_stmt->expression != 0);
    QCOMPARE(else_stmt->semicolon_token, 9U);

    SimpleNameAST *b_id_expr = else_stmt->expression->asIdExpression()->name->asSimpleName();
    QVERIFY(b_id_expr != 0);
    QCOMPARE(b_id_expr->identifier_token, 8U);
}

void tst_AST::while_statement()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("while (a) { }"));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    WhileStatementAST *stmt = ast->asWhileStatement();
    QVERIFY(stmt != 0);
    QCOMPARE(stmt->while_token, 1U);
    QCOMPARE(stmt->lparen_token, 2U);
    QVERIFY(stmt->condition != 0);
    QCOMPARE(stmt->rparen_token, 4U);
    QVERIFY(stmt->statement != 0);

    // check condition
    QVERIFY(stmt->condition->asIdExpression()->name->asSimpleName() != 0);
    QCOMPARE(stmt->condition->asIdExpression()->name->asSimpleName()->identifier_token, 3U);

    // check the `body' statement
    CompoundStatementAST *body_stmt = stmt->statement->asCompoundStatement();
    QVERIFY(body_stmt != 0);
    QCOMPARE(body_stmt->lbrace_token, 5U);
    QVERIFY(body_stmt->statement_list == 0);
    QCOMPARE(body_stmt->rbrace_token, 6U);
}

void tst_AST::while_condition_statement()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("while (int a = foo) { }"));

    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    WhileStatementAST *stmt = ast->asWhileStatement();
    QVERIFY(stmt != 0);
    QCOMPARE(stmt->while_token, 1U);
    QCOMPARE(stmt->lparen_token, 2U);
    QVERIFY(stmt->condition != 0);
    QCOMPARE(stmt->rparen_token, 7U);
    QVERIFY(stmt->statement != 0);

    // check condition
    ConditionAST *condition = stmt->condition->asCondition();
    QVERIFY(condition != 0);
    QVERIFY(condition->type_specifier_list != 0);
    QVERIFY(condition->type_specifier_list->value->asSimpleSpecifier() != 0);
    QCOMPARE(condition->type_specifier_list->value->asSimpleSpecifier()->specifier_token, 3U);
    QVERIFY(condition->type_specifier_list->next == 0);
    QVERIFY(condition->declarator != 0);
    QVERIFY(condition->declarator->core_declarator != 0);
    QVERIFY(condition->declarator->core_declarator->asDeclaratorId() != 0);
    QVERIFY(condition->declarator->core_declarator->asDeclaratorId()->name != 0);
    QVERIFY(condition->declarator->core_declarator->asDeclaratorId()->name->asSimpleName() != 0);
    QCOMPARE(condition->declarator->core_declarator->asDeclaratorId()->name->asSimpleName()->identifier_token, 4U);
    QVERIFY(condition->declarator->postfix_declarator_list == 0);
    QVERIFY(condition->declarator->initializer != 0);
    QVERIFY(condition->declarator->initializer->asIdExpression()->name->asSimpleName() != 0);
    QCOMPARE(condition->declarator->initializer->asIdExpression()->name->asSimpleName()->identifier_token, 6U);

    // check the `body' statement
    CompoundStatementAST *body_stmt = stmt->statement->asCompoundStatement();
    QVERIFY(body_stmt != 0);
    QCOMPARE(body_stmt->lbrace_token, 8U);
    QVERIFY(body_stmt->statement_list == 0);
    QCOMPARE(body_stmt->rbrace_token, 9U);
}

void tst_AST::for_statement()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("for (;;) {}"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    ForStatementAST *stmt = ast->asForStatement();
    QVERIFY(stmt != 0);
    QCOMPARE(stmt->for_token, 1U);
    QCOMPARE(stmt->lparen_token, 2U);
    QVERIFY(stmt->initializer != 0);
    QVERIFY(stmt->initializer->asExpressionStatement() != 0);
    QCOMPARE(stmt->initializer->asExpressionStatement()->semicolon_token, 3U);
    QVERIFY(stmt->condition == 0);
    QCOMPARE(stmt->semicolon_token, 4U);
    QVERIFY(stmt->expression == 0);
    QCOMPARE(stmt->rparen_token, 5U);
    QVERIFY(stmt->statement != 0);
    QVERIFY(stmt->statement->asCompoundStatement() != 0);
    QCOMPARE(stmt->statement->asCompoundStatement()->lbrace_token, 6U);
    QVERIFY(stmt->statement->asCompoundStatement()->statement_list == 0);
    QCOMPARE(stmt->statement->asCompoundStatement()->rbrace_token, 7U);
}

void tst_AST::cpp_initializer_or_function_declaration()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("QFileInfo fileInfo(foo);"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    DeclarationStatementAST *stmt = ast->asDeclarationStatement();
    QVERIFY(stmt != 0);

    QVERIFY(stmt->declaration != 0);

    SimpleDeclarationAST *simple_decl = stmt->declaration->asSimpleDeclaration();
    QVERIFY(simple_decl != 0);

    QVERIFY(simple_decl->decl_specifier_list != 0);
    QVERIFY(simple_decl->decl_specifier_list->next == 0);
    QVERIFY(simple_decl->declarator_list != 0);
    QVERIFY(simple_decl->declarator_list->next == 0);
    QCOMPARE(simple_decl->semicolon_token, 6U);

    NamedTypeSpecifierAST *named_ty = simple_decl->decl_specifier_list->value->asNamedTypeSpecifier();
    QVERIFY(named_ty != 0);
    QVERIFY(named_ty->name != 0);

    SimpleNameAST *simple_named_ty = named_ty->name->asSimpleName();
    QVERIFY(simple_named_ty != 0);
    QCOMPARE(simple_named_ty->identifier_token, 1U);

    DeclaratorAST *declarator = simple_decl->declarator_list->value;
    QVERIFY(declarator != 0);
    QVERIFY(declarator->core_declarator != 0);
    QVERIFY(declarator->postfix_declarator_list != 0);
    QVERIFY(declarator->postfix_declarator_list->next == 0);
    QVERIFY(declarator->initializer == 0);

    DeclaratorIdAST *decl_id = declarator->core_declarator->asDeclaratorId();
    QVERIFY(decl_id != 0);
    QVERIFY(decl_id->name != 0);
    QVERIFY(decl_id->name->asSimpleName() != 0);
    QCOMPARE(decl_id->name->asSimpleName()->identifier_token, 2U);

    FunctionDeclaratorAST *fun_declarator = declarator->postfix_declarator_list->value->asFunctionDeclarator();
    QVERIFY(fun_declarator != 0);
    QCOMPARE(fun_declarator->lparen_token, 3U);
    QVERIFY(fun_declarator->parameter_declaration_clause != 0);
    QCOMPARE(fun_declarator->rparen_token, 5U);

    // check the formal arguments
    ParameterDeclarationClauseAST *param_clause = fun_declarator->parameter_declaration_clause;
    QVERIFY(param_clause->parameter_declaration_list != 0);
    QVERIFY(param_clause->parameter_declaration_list->next == 0);
    QCOMPARE(param_clause->dot_dot_dot_token, 0U);

    // check the parameter
    ParameterDeclarationListAST *declarations = param_clause->parameter_declaration_list;
    QVERIFY(declarations);
    QVERIFY(declarations->value);
    QVERIFY(! declarations->next);

    ParameterDeclarationAST *param = declarations->value->asParameterDeclaration();
    QVERIFY(param);
    QVERIFY(param->type_specifier_list != 0);
    QVERIFY(param->type_specifier_list->next == 0);
    QVERIFY(param->type_specifier_list->value->asNamedTypeSpecifier() != 0);
    QVERIFY(param->type_specifier_list->value->asNamedTypeSpecifier()->name != 0);
    QVERIFY(param->type_specifier_list->value->asNamedTypeSpecifier()->name->asSimpleName() != 0);
    QCOMPARE(param->type_specifier_list->value->asNamedTypeSpecifier()->name->asSimpleName()->identifier_token, 4U);
}

void tst_AST::cpp_constructor_one_unamed_arg()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("QFileInfo::QFileInfo(QString /*name*/) {}"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    FunctionDefinitionAST *funDef = ast->asFunctionDefinition();
    QVERIFY(funDef != 0);
    QVERIFY(funDef->declarator != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list->lastValue() != 0);

    FunctionDeclaratorAST *funDecl = funDef->declarator->postfix_declarator_list->lastValue()->asFunctionDeclarator();
    QVERIFY(funDecl != 0);
    QVERIFY(funDecl->parameter_declaration_clause != 0);
    QVERIFY(funDecl->parameter_declaration_clause->parameter_declaration_list != 0);
}

void tst_AST::cpp_constructor_one_unamed_arg_namespace()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("Foo::QFileInfo::QFileInfo(QString /*name*/) {}"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    FunctionDefinitionAST *funDef = ast->asFunctionDefinition();
    QVERIFY(funDef != 0);
    QVERIFY(funDef->declarator != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list->lastValue() != 0);

    FunctionDeclaratorAST *funDecl = funDef->declarator->postfix_declarator_list->lastValue()->asFunctionDeclarator();
    QVERIFY(funDecl != 0);
    QVERIFY(funDecl->parameter_declaration_clause != 0);
    QVERIFY(funDecl->parameter_declaration_clause->parameter_declaration_list != 0);
}

void tst_AST::cpp_constructor_one_named_arg()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("QFileInfo::QFileInfo(QString name) {}"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    FunctionDefinitionAST *funDef = ast->asFunctionDefinition();
    QVERIFY(funDef != 0);
    QVERIFY(funDef->declarator != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list->lastValue() != 0);

    FunctionDeclaratorAST *funDecl = funDef->declarator->postfix_declarator_list->lastValue()->asFunctionDeclarator();
    QVERIFY(funDecl != 0);
    QVERIFY(funDecl->parameter_declaration_clause != 0);
    QVERIFY(funDecl->parameter_declaration_clause->parameter_declaration_list != 0);
}

void tst_AST::cpp_constructor_one_knowntype_arg()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("QFileInfo::QFileInfo(int /*name*/) {}"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    FunctionDefinitionAST *funDef = ast->asFunctionDefinition();
    QVERIFY(funDef != 0);
    QVERIFY(funDef->declarator != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list->lastValue() != 0);

    FunctionDeclaratorAST *funDecl = funDef->declarator->postfix_declarator_list->lastValue()->asFunctionDeclarator();
    QVERIFY(funDecl != 0);
    QVERIFY(funDecl->parameter_declaration_clause != 0);
    QVERIFY(funDecl->parameter_declaration_clause->parameter_declaration_list != 0);
}


void tst_AST::cpp_constructor_one_const_arg()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("QFileInfo::QFileInfo(const QString /*name*/) {}"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    FunctionDefinitionAST *funDef = ast->asFunctionDefinition();
    QVERIFY(funDef != 0);
    QVERIFY(funDef->declarator != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list->lastValue() != 0);

    FunctionDeclaratorAST *funDecl = funDef->declarator->postfix_declarator_list->lastValue()->asFunctionDeclarator();
    QVERIFY(funDecl != 0);
    QVERIFY(funDecl->parameter_declaration_clause != 0);
    QVERIFY(funDecl->parameter_declaration_clause->parameter_declaration_list != 0);
}

void tst_AST::cpp_constructor_one_ref_arg()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("QFileInfo::QFileInfo(QString & /*name*/) {}"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    FunctionDefinitionAST *funDef = ast->asFunctionDefinition();
    QVERIFY(funDef != 0);
    QVERIFY(funDef->declarator != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list->lastValue() != 0);

    FunctionDeclaratorAST *funDecl = funDef->declarator->postfix_declarator_list->lastValue()->asFunctionDeclarator();
    QVERIFY(funDecl != 0);
    QVERIFY(funDecl->parameter_declaration_clause != 0);
    QVERIFY(funDecl->parameter_declaration_clause->parameter_declaration_list != 0);
}

void tst_AST::cpp_constructor_no_arg()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("QFileInfo::QFileInfo() {}"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    FunctionDefinitionAST *funDef = ast->asFunctionDefinition();
    QVERIFY(funDef != 0);
    QVERIFY(funDef->declarator != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list->lastValue() != 0);

    FunctionDeclaratorAST *funDecl = funDef->declarator->postfix_declarator_list->lastValue()->asFunctionDeclarator();
    QVERIFY(funDecl != 0);
    QVERIFY(funDecl->parameter_declaration_clause == 0);
}

void tst_AST::cpp_constructor_multiple_args()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("QFileInfo::QFileInfo(QString /*name*/, QString /*type*/) {}"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);

    FunctionDefinitionAST *funDef = ast->asFunctionDefinition();
    QVERIFY(funDef != 0);
    QVERIFY(funDef->declarator != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list != 0);
    QVERIFY(funDef->declarator->postfix_declarator_list->lastValue() != 0);

    FunctionDeclaratorAST *funDecl = funDef->declarator->postfix_declarator_list->lastValue()->asFunctionDeclarator();
    QVERIFY(funDecl != 0);
    QVERIFY(funDecl->parameter_declaration_clause != 0);
    QVERIFY(funDecl->parameter_declaration_clause->parameter_declaration_list != 0);
}

void tst_AST::objc_simple_class()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n"
                                                          "@interface Zoo {} +(id)alloc;-(id)init;@end\n"
                                                          "@implementation Zoo\n"
                                                          "+(id)alloc{}\n"
                                                          "-(id)init{}\n"
                                                          "@end\n"
                                                          ));

    AST *ast = unit->ast();
    QVERIFY(ast);
}

void tst_AST::objc_attributes_followed_by_at_keyword()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n"
"__attribute__((deprecated)) @interface foo <bar>\n"
"{\n"
" int a, b;\n"
"}\n"
"+ (id) init;\n"
"- (id) init:(int)a foo:(int)b, c;\n"
"@end\n"
    ));
    AST *ast = unit->ast();
    QVERIFY(ast);
}

void tst_AST::objc_protocol_forward_declaration_1()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n@protocol foo;"));
    AST *ast = unit->ast();
    QVERIFY(ast);
}

void tst_AST::objc_protocol_definition_1()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n@protocol foo <ciao, bar> @end"));
    AST *ast = unit->ast();
    QVERIFY(ast);
}

void tst_AST::objc_method_attributes_1()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n"
                                                          "@interface Zoo\n"
                                                          "- (void) foo __attribute__((deprecated));\n"
                                                          "+ (void) bar __attribute__((unavailable));\n"
                                                          "@end\n"
                                                          ));
    AST *ast = unit->ast();
    QVERIFY(ast);
    ObjCClassDeclarationAST *zoo = ast->asObjCClassDeclaration();
    QVERIFY(zoo);
    QVERIFY(zoo->interface_token); QVERIFY(! (zoo->implementation_token));
    QVERIFY(zoo->class_name); QVERIFY(zoo->class_name->asSimpleName());
    QCOMPARE(unit->spell(zoo->class_name->asSimpleName()->identifier_token), "Zoo");

    DeclarationListAST *decls = zoo->member_declaration_list;
    QVERIFY(decls->value);
    QVERIFY(decls->next);
    QVERIFY(decls->next->value);
    QVERIFY(! (decls->next->next));

    ObjCMethodDeclarationAST *fooDecl = decls->value->asObjCMethodDeclaration();
    QVERIFY(fooDecl);
    QVERIFY(! (fooDecl->function_body));
    QVERIFY(fooDecl->semicolon_token);

    ObjCMethodPrototypeAST *foo = fooDecl->method_prototype;
    QVERIFY(foo);
    QCOMPARE(unit->tokenKind(foo->method_type_token), (int) T_MINUS);
    QVERIFY(foo->type_name);
    QVERIFY(foo->selector);
    QVERIFY(foo->selector->selector_argument_list->value);
    QVERIFY(!foo->selector->selector_argument_list->next);
    QCOMPARE(unit->spell(foo->selector->selector_argument_list->value->name_token), "foo");
    QVERIFY(foo->attribute_list);
    QVERIFY(foo->attribute_list->value);
    QVERIFY(! (foo->attribute_list->next));
    AttributeSpecifierAST *deprecatedSpec = foo->attribute_list->value->asAttributeSpecifier();
    QVERIFY(deprecatedSpec);
    QCOMPARE(unit->tokenKind(deprecatedSpec->attribute_token), (int) T___ATTRIBUTE__);
    QVERIFY(deprecatedSpec->attribute_list);
    QVERIFY(deprecatedSpec->attribute_list->value);
    QVERIFY(! (deprecatedSpec->attribute_list->next));
    AttributeAST *deprecatedAttr = deprecatedSpec->attribute_list->value->asAttribute();
    QVERIFY(deprecatedAttr);
    QVERIFY(! deprecatedAttr->expression_list);
    QCOMPARE(unit->spell(deprecatedAttr->identifier_token), "deprecated");

    ObjCMethodDeclarationAST *barDecl = decls->next->value->asObjCMethodDeclaration();
    QVERIFY(barDecl);
    QVERIFY(! (barDecl->function_body));
    QVERIFY(barDecl->semicolon_token);

    ObjCMethodPrototypeAST *bar = barDecl->method_prototype;
    QVERIFY(bar);
    QCOMPARE(unit->tokenKind(bar->method_type_token), (int) T_PLUS);
    QVERIFY(bar->type_name);
    QVERIFY(bar->selector);
    QVERIFY(bar->selector->selector_argument_list);
    QVERIFY(bar->selector->selector_argument_list->value);
    QVERIFY(!bar->selector->selector_argument_list->next);
    QCOMPARE(unit->spell(bar->selector->selector_argument_list->value->name_token), "bar");
    QVERIFY(bar->attribute_list);
    QVERIFY(bar->attribute_list->value);
    QVERIFY(! (bar->attribute_list->next));
    AttributeSpecifierAST *unavailableSpec = bar->attribute_list->value->asAttributeSpecifier();
    QVERIFY(unavailableSpec);
    QCOMPARE(unit->tokenKind(unavailableSpec->attribute_token), (int) T___ATTRIBUTE__);
    QVERIFY(unavailableSpec->attribute_list);
    QVERIFY(unavailableSpec->attribute_list->value);
    QVERIFY(! (unavailableSpec->attribute_list->next));
    AttributeAST *unavailableAttr = unavailableSpec->attribute_list->value->asAttribute();
    QVERIFY(unavailableAttr);
    QVERIFY(! unavailableAttr->expression_list);
    QCOMPARE(unit->spell(unavailableAttr->identifier_token), "unavailable");
}

/*
  @selector(foo)
  @selector(foo:)
  @selector(foo:bar:)
  ...
 */
void tst_AST::objc_selector_error_recovery_1()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n"
                                                          "void tst() {\n"
                                                          "    @selector(foo:\n"
                                                          "    int i = 1;\n"
                                                          "}\n"
                                                          ));
    AST *ast = unit->ast();
    QVERIFY(ast);
}

void tst_AST::objc_selector_error_recovery_2()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n"
                                                          "void tst() {\n"
                                                          "    @selector(foo:bar);\n"
                                                          "}\n"
                                                          ));
    AST *ast = unit->ast();
    QVERIFY(ast);
}

void tst_AST::normal_array_access()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n"
                                                          "int f() {\n"
                                                          "  int a[10];\n"
                                                          "  int b = 1;\n"
                                                          "  return a[b];\n"
                                                          "}"
                                                          ));
    AST *ast = unit->ast();
    QVERIFY(ast);

    FunctionDefinitionAST *func = ast->asFunctionDefinition();
    QVERIFY(func);

    StatementListAST *bodyStatements = func->function_body->asCompoundStatement()->statement_list;
    QVERIFY(bodyStatements);
    QVERIFY(bodyStatements->next);
    QVERIFY(bodyStatements->next->next);
    QVERIFY(bodyStatements->next->next->value);
    ExpressionAST *expr = bodyStatements->next->next->value->asReturnStatement()->expression;
    QVERIFY(expr);

    ArrayAccessAST *arrayExpr = expr->asArrayAccess();
    QVERIFY(arrayExpr);

    { // check the left-hand side:
        ExpressionAST *lhs = arrayExpr->base_expression;
        QVERIFY(lhs);
        SimpleNameAST *a = lhs->asIdExpression()->name->asSimpleName();
        QVERIFY(a);
        QCOMPARE(QLatin1String(unit->identifier(a->identifier_token)->chars()), QLatin1String("a"));
    }

    { // check the right-hand side:
        QVERIFY(arrayExpr->expression);
        SimpleNameAST *b = arrayExpr->expression->asIdExpression()->name->asSimpleName();
        QVERIFY(b);
        QCOMPARE(QLatin1String(unit->identifier(b->identifier_token)->chars()), QLatin1String("b"));
    }
}

void tst_AST::array_access_with_nested_expression()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n"
                                                          "int f() {\n"
                                                          "  int a[15];\n"
                                                          "  int b = 1;\n"
                                                          "  return (a)[b];\n"
                                                          "}"
                                                          ));
    AST *ast = unit->ast();
    QVERIFY(ast);

    FunctionDefinitionAST *func = ast->asFunctionDefinition();
    QVERIFY(func);

    StatementListAST *bodyStatements = func->function_body->asCompoundStatement()->statement_list;
    QVERIFY(bodyStatements && bodyStatements->next && bodyStatements->next->next && bodyStatements->next->next->value);
    ExpressionAST *expr = bodyStatements->next->next->value->asReturnStatement()->expression;
    QVERIFY(expr);

    CastExpressionAST *castExpr = expr->asCastExpression();
    QVERIFY(!castExpr);

    ArrayAccessAST *arrayExpr = expr->asArrayAccess();
    QVERIFY(arrayExpr);

    { // check the LHS:
        ExpressionAST *lhs = arrayExpr->base_expression;
        QVERIFY(lhs);
        NestedExpressionAST *nested_a = lhs->asNestedExpression();
        QVERIFY(nested_a && nested_a->expression);
        SimpleNameAST *a = nested_a->expression->asIdExpression()->name->asSimpleName();
        QVERIFY(a);
        QCOMPARE(QLatin1String(unit->identifier(a->identifier_token)->chars()), QLatin1String("a"));
    }

    { // check the RHS:
        QVERIFY(arrayExpr->expression);
        SimpleNameAST *b = arrayExpr->expression->asIdExpression()->name->asSimpleName();
        QVERIFY(b);
        QCOMPARE(QLatin1String(unit->identifier(b->identifier_token)->chars()), QLatin1String("b"));
    }
}

void tst_AST::objc_msg_send_expression()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n"
                                                          "int f() {\n"
                                                          "  NSObject *obj = [[[NSObject alloc] init] autorelease];\n"
                                                          "  return [obj description];\n"
                                                          "}"
                                                          ));
    AST *ast = unit->ast();
    QVERIFY(ast);

    FunctionDefinitionAST *func = ast->asFunctionDefinition();
    QVERIFY(func);

    StatementListAST *bodyStatements = func->function_body->asCompoundStatement()->statement_list;
    QVERIFY(bodyStatements && bodyStatements->next && !bodyStatements->next->next && bodyStatements->next->value);

    {// check the NSObject declaration
        DeclarationStatementAST *firstStatement = bodyStatements->value->asDeclarationStatement();
        QVERIFY(firstStatement);
        DeclarationAST *objDecl = firstStatement->declaration;
        QVERIFY(objDecl);
        SimpleDeclarationAST *simpleDecl = objDecl->asSimpleDeclaration();
        QVERIFY(simpleDecl);

        {// check the type (NSObject)
            QVERIFY(simpleDecl->decl_specifier_list && !simpleDecl->decl_specifier_list->next);
            NamedTypeSpecifierAST *namedType = simpleDecl->decl_specifier_list->value->asNamedTypeSpecifier();
            QVERIFY(namedType && namedType->name);
            SimpleNameAST *typeName = namedType->name->asSimpleName();
            QVERIFY(typeName);
            QCOMPARE(QLatin1String(unit->identifier(typeName->identifier_token)->chars()), QLatin1String("NSObject"));
        }

        {// check the assignment
            QVERIFY(simpleDecl->declarator_list && !simpleDecl->declarator_list->next);
            DeclaratorAST *declarator = simpleDecl->declarator_list->value;
            QVERIFY(declarator);
            QVERIFY(!declarator->attribute_list);

            QVERIFY(declarator->ptr_operator_list && !declarator->ptr_operator_list->next
                    &&   declarator->ptr_operator_list->value->asPointer()
                    && ! declarator->ptr_operator_list->value->asPointer()->cv_qualifier_list);

            QVERIFY(declarator->core_declarator && declarator->core_declarator->asDeclaratorId());
            NameAST *objNameId = declarator->core_declarator->asDeclaratorId()->name;
            QVERIFY(objNameId && objNameId->asSimpleName());
            QCOMPARE(QLatin1String(unit->identifier(objNameId->asSimpleName()->identifier_token)->chars()), QLatin1String("obj"));

            QVERIFY(!declarator->postfix_declarator_list);
            QVERIFY(!declarator->post_attribute_list);
            ExpressionAST *initializer = declarator->initializer;
            QVERIFY(initializer);

            ObjCMessageExpressionAST *expr1 = initializer->asObjCMessageExpression();
            QVERIFY(expr1 && expr1->receiver_expression && expr1->selector && !expr1->argument_list);

            ObjCMessageExpressionAST *expr2 = expr1->receiver_expression->asObjCMessageExpression();
            QVERIFY(expr2 && expr2->receiver_expression && expr2->selector && !expr2->argument_list);

            ObjCMessageExpressionAST *expr3 = expr2->receiver_expression->asObjCMessageExpression();
            QVERIFY(expr3 && expr3->receiver_expression && expr3->selector && !expr3->argument_list);
        }
    }

    {// check the return statement
        ExpressionAST *expr = bodyStatements->next->value->asReturnStatement()->expression;
        QVERIFY(expr);

        ObjCMessageExpressionAST *msgExpr = expr->asObjCMessageExpression();
        QVERIFY(msgExpr);

        QVERIFY(msgExpr->receiver_expression);
        SimpleNameAST *receiver = msgExpr->receiver_expression->asIdExpression()->name->asSimpleName();
        QVERIFY(receiver);
        QCOMPARE(QLatin1String(unit->identifier(receiver->identifier_token)->chars()), QLatin1String("obj"));

        QVERIFY(msgExpr->argument_list == 0);

        QVERIFY(msgExpr->selector);
        ObjCSelectorArgumentListAST *args = msgExpr->selector->selector_argument_list;
        QVERIFY(args);
        QVERIFY(args->value);
        QVERIFY(! args->next);
        QCOMPARE(QLatin1String(unit->identifier(args->value->name_token)->chars()), QLatin1String("description"));
    }
}

void tst_AST::objc_msg_send_expression_without_selector()
{
    // This test is to verify that no ObjCMessageExpressionAST element is created as the expression for the return statement.
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n"
                                                          "int f() {\n"
                                                          "  NSObject *obj = [[[NSObject alloc] init] autorelease];\n"
                                                          "  return [obj];\n"
                                                          "}",
                                                          true));
    AST *ast = unit->ast();
    QVERIFY(ast);

    FunctionDefinitionAST *func = ast->asFunctionDefinition();
    QVERIFY(func);

    StatementListAST *bodyStatements = func->function_body->asCompoundStatement()->statement_list;
    QVERIFY(bodyStatements && bodyStatements->next);
    QVERIFY(bodyStatements->next->value);
    QVERIFY(bodyStatements->next->value->asReturnStatement());
    QVERIFY(!bodyStatements->next->value->asReturnStatement()->expression);
}

void tst_AST::q_enum_1()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n"
                                                          "class Tst {\n"
                                                          "Q_ENUMS(e)\n"
                                                          "public:\n"
                                                          "enum e { x, y };\n"
                                                          "};\n",
                                                          false, true));
    QVERIFY(unit->ast());
    SimpleDeclarationAST *tstDecl = unit->ast()->asSimpleDeclaration();
    QVERIFY(tstDecl);
    QVERIFY(! tstDecl->declarator_list);
    QVERIFY(tstDecl->decl_specifier_list);
    QVERIFY(tstDecl->decl_specifier_list->value);
    QVERIFY(! tstDecl->decl_specifier_list->next);
    ClassSpecifierAST *tst = tstDecl->decl_specifier_list->value->asClassSpecifier();
    QVERIFY(tst);

    QVERIFY(tst->member_specifier_list);
    QVERIFY(tst->member_specifier_list->value);
    QtEnumDeclarationAST *qtEnum = tst->member_specifier_list->value->asQtEnumDeclaration();
    QVERIFY(qtEnum);
    QVERIFY(qtEnum->enumerator_list);
    QVERIFY(qtEnum->enumerator_list->value);
    QVERIFY(! qtEnum->enumerator_list->next);

    SimpleNameAST *e = qtEnum->enumerator_list->value->asSimpleName();
    QVERIFY(e);
    QCOMPARE(unit->spell(e->identifier_token), "e");
}

void tst_AST::initTestCase()
{
    control.setDiagnosticClient(&diag);
}

QTEST_APPLESS_MAIN(tst_AST)
#include "tst_ast.moc"

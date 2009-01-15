
#include <QtTest>
#include <QtDebug>

#include <Control.h>
#include <Parser.h>
#include <AST.h>

CPLUSPLUS_USE_NAMESPACE

class tst_AST: public QObject
{
    Q_OBJECT

    Control control;

public:
    TranslationUnit *parse(const QByteArray &source,
                           TranslationUnit::ParseMode mode)
    {
        StringLiteral *fileId = control.findOrInsertFileName("<stdin>");
        TranslationUnit *unit = new TranslationUnit(&control, fileId);
        unit->setObjCEnabled(true);
        unit->setSource(source.constData(), source.length());
        unit->parse(mode);
        return unit;
    }

    TranslationUnit *parseDeclaration(const QByteArray &source)
    { return parse(source, TranslationUnit::ParseDeclaration); }

    TranslationUnit *parseExpression(const QByteArray &source)
    { return parse(source, TranslationUnit::ParseExpression); }

    TranslationUnit *parseStatement(const QByteArray &source)
    { return parse(source, TranslationUnit::ParseStatement); }

private slots:
    // declarations
    void gcc_attributes_1();

    // expressions
    void simple_name();
    void template_id();

    // statements
    void if_statement();
    void if_else_statement();
    void while_statement();
    void while_condition_statement();
    void for_statement();
    void cpp_initializer_or_function_declaration();

    // objc++
    void objc_attributes_followed_by_at_keyword();
    void objc_protocol_forward_declaration_1();
    void objc_protocol_definition_1();
};

void tst_AST::gcc_attributes_1()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n"
"static inline void *__attribute__((__always_inline__)) _mm_malloc(size_t size, size_t align);"
    ));
}

void tst_AST::simple_name()
{
    QSharedPointer<TranslationUnit> unit(parseExpression("a"));
    AST *ast = unit->ast();

    QVERIFY(ast != 0);
    QVERIFY(ast->asSimpleName() != 0);
    QCOMPARE(ast->asSimpleName()->identifier_token, 1U);
}

void tst_AST::template_id()
{
    QSharedPointer<TranslationUnit> unit(parseExpression("list<10>"));
    AST *ast = unit->ast();

    QVERIFY(ast != 0);
    QVERIFY(ast->asTemplateId() != 0);
    QCOMPARE(ast->asTemplateId()->identifier_token, 1U);
    QCOMPARE(ast->asTemplateId()->less_token, 2U);
    QVERIFY(ast->asTemplateId()->template_arguments != 0);
    QVERIFY(ast->asTemplateId()->template_arguments->template_argument != 0);
    QVERIFY(ast->asTemplateId()->template_arguments->template_argument->asNumericLiteral() != 0);
    QCOMPARE(ast->asTemplateId()->template_arguments->template_argument->asNumericLiteral()->token, 3U);
    QVERIFY(ast->asTemplateId()->template_arguments->next == 0);
    QCOMPARE(ast->asTemplateId()->greater_token, 4U);
}

void tst_AST::if_statement()
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

    // check the `then' statement
    ExpressionStatementAST *then_stmt = stmt->statement->asExpressionStatement();
    QVERIFY(then_stmt != 0);
    QVERIFY(then_stmt->expression != 0);
    QCOMPARE(then_stmt->semicolon_token, 6U);

    SimpleNameAST *id_expr = then_stmt->expression->asSimpleName();
    QVERIFY(id_expr != 0);
    QCOMPARE(id_expr->identifier_token, 5U);
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

    SimpleNameAST *a_id_expr = then_stmt->expression->asSimpleName();
    QVERIFY(a_id_expr != 0);
    QCOMPARE(a_id_expr->identifier_token, 5U);

    // check the `then' statement
    ExpressionStatementAST *else_stmt = stmt->else_statement->asExpressionStatement();
    QVERIFY(else_stmt != 0);
    QVERIFY(else_stmt->expression != 0);
    QCOMPARE(else_stmt->semicolon_token, 9U);

    SimpleNameAST *b_id_expr = else_stmt->expression->asSimpleName();
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
    QVERIFY(stmt->condition->asSimpleName() != 0);
    QCOMPARE(stmt->condition->asSimpleName()->identifier_token, 3U);

    // check the `body' statement
    CompoundStatementAST *body_stmt = stmt->statement->asCompoundStatement();
    QVERIFY(body_stmt != 0);
    QCOMPARE(body_stmt->lbrace_token, 5U);
    QVERIFY(body_stmt->statements == 0);
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
    QVERIFY(condition->type_specifier != 0);
    QVERIFY(condition->type_specifier->asSimpleSpecifier() != 0);
    QCOMPARE(condition->type_specifier->asSimpleSpecifier()->specifier_token, 3U);
    QVERIFY(condition->type_specifier->next == 0);
    QVERIFY(condition->declarator != 0);
    QVERIFY(condition->declarator->core_declarator != 0);
    QVERIFY(condition->declarator->core_declarator->asDeclaratorId() != 0);
    QVERIFY(condition->declarator->core_declarator->asDeclaratorId()->name != 0);
    QVERIFY(condition->declarator->core_declarator->asDeclaratorId()->name->asSimpleName() != 0);
    QCOMPARE(condition->declarator->core_declarator->asDeclaratorId()->name->asSimpleName()->identifier_token, 4U);
    QVERIFY(condition->declarator->postfix_declarators == 0);
    QVERIFY(condition->declarator->initializer != 0);
    QVERIFY(condition->declarator->initializer->asSimpleName() != 0);
    QCOMPARE(condition->declarator->initializer->asSimpleName()->identifier_token, 6U);

    // check the `body' statement
    CompoundStatementAST *body_stmt = stmt->statement->asCompoundStatement();
    QVERIFY(body_stmt != 0);
    QCOMPARE(body_stmt->lbrace_token, 8U);
    QVERIFY(body_stmt->statements == 0);
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
    QVERIFY(stmt->statement->asCompoundStatement()->statements == 0);
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

    QVERIFY(simple_decl->decl_specifier_seq != 0);
    QVERIFY(simple_decl->decl_specifier_seq->next == 0);
    QVERIFY(simple_decl->declarators != 0);
    QVERIFY(simple_decl->declarators->next == 0);
    QCOMPARE(simple_decl->semicolon_token, 6U);

    NamedTypeSpecifierAST *named_ty = simple_decl->decl_specifier_seq->asNamedTypeSpecifier();
    QVERIFY(named_ty != 0);
    QVERIFY(named_ty->name != 0);

    SimpleNameAST *simple_named_ty = named_ty->name->asSimpleName();
    QVERIFY(simple_named_ty != 0);
    QCOMPARE(simple_named_ty->identifier_token, 1U);

    DeclaratorAST *declarator = simple_decl->declarators->declarator;
    QVERIFY(declarator != 0);
    QVERIFY(declarator->core_declarator != 0);
    QVERIFY(declarator->postfix_declarators != 0);
    QVERIFY(declarator->postfix_declarators->next == 0);
    QVERIFY(declarator->initializer == 0);

    DeclaratorIdAST *decl_id = declarator->core_declarator->asDeclaratorId();
    QVERIFY(decl_id != 0);
    QVERIFY(decl_id->name != 0);
    QVERIFY(decl_id->name->asSimpleName() != 0);
    QCOMPARE(decl_id->name->asSimpleName()->identifier_token, 2U);

    FunctionDeclaratorAST *fun_declarator = declarator->postfix_declarators->asFunctionDeclarator();
    QVERIFY(fun_declarator != 0);
    QCOMPARE(fun_declarator->lparen_token, 3U);
    QVERIFY(fun_declarator->parameters != 0);
    QCOMPARE(fun_declarator->rparen_token, 5U);

    // check the formal arguments
    ParameterDeclarationClauseAST *param_clause = fun_declarator->parameters;
    QVERIFY(param_clause->parameter_declarations != 0);
    QVERIFY(param_clause->parameter_declarations->next == 0);
    QCOMPARE(param_clause->dot_dot_dot_token, 0U);

    // check the parameter
    ParameterDeclarationAST *param = param_clause->parameter_declarations->asParameterDeclaration();
    QVERIFY(param->type_specifier != 0);
    QVERIFY(param->type_specifier->next == 0);
    QVERIFY(param->type_specifier->asNamedTypeSpecifier() != 0);
    QVERIFY(param->type_specifier->asNamedTypeSpecifier()->name != 0);
    QVERIFY(param->type_specifier->asNamedTypeSpecifier()->name->asSimpleName() != 0);
    QCOMPARE(param->type_specifier->asNamedTypeSpecifier()->name->asSimpleName()->identifier_token, 4U);
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
}

void tst_AST::objc_protocol_forward_declaration_1()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n@protocol foo;"));
    AST *ast = unit->ast();
}

void tst_AST::objc_protocol_definition_1()
{
    QSharedPointer<TranslationUnit> unit(parseDeclaration("\n@protocol foo <ciao, bar> @end"));
    AST *ast = unit->ast();
}

QTEST_APPLESS_MAIN(tst_AST)
#include "tst_ast.moc"

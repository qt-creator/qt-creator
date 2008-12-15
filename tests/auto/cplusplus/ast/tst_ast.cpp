
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
        unit->setSource(source.constData(), source.length());
        unit->parse(mode);
        return unit;
    }

    TranslationUnit *parseStatement(const QByteArray &source)
    { return parse(source, TranslationUnit::ParseStatement); }

private slots:
    void if_statement();
    void if_else_statement();
    void cpp_initializer_or_function_declaration();
};

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


QTEST_APPLESS_MAIN(tst_AST)
#include "tst_ast.moc"

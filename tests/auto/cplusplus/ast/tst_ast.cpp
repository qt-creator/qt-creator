
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
    void cpp_initializer();
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
}

void tst_AST::cpp_initializer()
{
    QSharedPointer<TranslationUnit> unit(parseStatement("QFileInfo fileInfo(foo);"));
    AST *ast = unit->ast();
    QVERIFY(ast != 0);
}


QTEST_APPLESS_MAIN(tst_AST)
#include "tst_ast.moc"

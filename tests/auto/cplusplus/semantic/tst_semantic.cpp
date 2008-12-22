
#include <QtTest>
#include <QtDebug>

#include <Control.h>
#include <Parser.h>
#include <AST.h>
#include <Semantic.h>
#include <Scope.h>
#include <Symbols.h>
#include <Names.h>
#include <Literals.h>

CPLUSPLUS_USE_NAMESPACE

class tst_Semantic: public QObject
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

    class Document {
        Q_DISABLE_COPY(Document)

    public:
        Document(TranslationUnit *unit)
            : unit(unit), globals(new Scope())
        { }

        ~Document()
        { delete globals; }

        void check()
        {
            QVERIFY(unit);
            QVERIFY(unit->ast());
            Semantic sem(unit->control());
            TranslationUnitAST *ast = unit->ast()->asTranslationUnit();
            QVERIFY(ast);
            for (DeclarationAST *decl = ast->declarations; decl; decl = decl->next) {
                sem.check(decl, globals);
            }
        }

        TranslationUnit *unit;
        Scope *globals;
    };

    QSharedPointer<Document> document(const QByteArray &source)
    {
        TranslationUnit *unit = parse(source, TranslationUnit::ParseTranlationUnit);
        QSharedPointer<Document> doc(new Document(unit));
        doc->check();
        return doc;
    }

private slots:
    void function_declarations();
};

void tst_Semantic::function_declarations()
{
    QSharedPointer<Document> doc = document("void foo();");
    QCOMPARE(doc->globals->symbolCount(), 1U);

    Declaration *decl = doc->globals->symbolAt(0)->asDeclaration();
    QVERIFY(decl);

    FullySpecifiedType declTy = decl->type();
    Function *funTy = declTy->asFunction();
    QVERIFY(funTy);
    QVERIFY(funTy->returnType()->isVoidType());
    QCOMPARE(funTy->argumentCount(), 0U);

    QVERIFY(decl->name()->isNameId());
    Identifier *funId = decl->name()->asNameId()->identifier();
    QVERIFY(funId);

    const QByteArray foo(funId->chars(), funId->size());
    QCOMPARE(foo, QByteArray("foo"));
}


QTEST_APPLESS_MAIN(tst_Semantic)
#include "tst_semantic.moc"

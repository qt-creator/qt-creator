
#include <QtTest>
#include <QObject>

#include <AST.h>
#include <ASTVisitor.h>
#include <TranslationUnit.h>
#include <CppDocument.h>
#include <LookupContext.h>
#include <Symbols.h>
#include <Overview.h>

using namespace CPlusPlus;

template <template <typename, typename> class _Map, typename _T1, typename _T2>
_Map<_T2, _T1> invert(const _Map<_T1, _T2> &m)
{
    _Map<_T2, _T1> i;
    typename _Map<_T1, _T2>::const_iterator it = m.constBegin();
    for (; it != m.constEnd(); ++it) {
        i.insertMulti(it.value(), it.key());
    }
    return i;
}

class ClassSymbols: protected ASTVisitor,
    public QMap<ClassSpecifierAST *, Class *>
{
public:
    ClassSymbols(Control *control)
        : ASTVisitor(control)
    { }

    QMap<ClassSpecifierAST *, Class *> asMap() const
    { return *this; }

    void operator()(AST *ast)
    { accept(ast); }

protected:
    virtual bool visit(ClassSpecifierAST *ast)
    {
        Class *classSymbol = ast->symbol;
        Q_ASSERT(classSymbol != 0);

        insert(ast, classSymbol);

        return true;
    }
};

class tst_Lookup: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void base_class_defined_1();
};

void tst_Lookup::base_class_defined_1()
{
    Overview overview;

    const QByteArray source = "\n"
        "class base {};\n"
        "class derived: public base {};\n";

    Document::Ptr doc = Document::create("base_class_defined_1");
    doc->setSource(source);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 2U);

    Snapshot snapshot;
    snapshot.insert(doc->fileName(), doc);

    Document::Ptr emptyDoc = Document::create("<empty>");

    Class *baseClass = doc->globalSymbolAt(0)->asClass();
    QVERIFY(baseClass);

    Class *derivedClass = doc->globalSymbolAt(1)->asClass();
    QVERIFY(derivedClass);

    LookupContext ctx(derivedClass, emptyDoc, doc, snapshot);

    const QList<Symbol *> candidates =
        ctx.resolveClass(derivedClass->baseClassAt(0)->name());

    QCOMPARE(candidates.size(), 1);
    QCOMPARE(candidates.at(0), baseClass);

    TranslationUnit *unit = doc->translationUnit();
    QVERIFY(unit != 0);

    TranslationUnitAST *ast = unit->ast()->asTranslationUnit();
    QVERIFY(ast != 0);

    ClassSymbols classSymbols(doc->control());
    classSymbols(ast);

    QCOMPARE(classSymbols.size(), 2);

    const QMap<Class *, ClassSpecifierAST *> classToAST =
            invert(classSymbols.asMap());

    QVERIFY(classToAST.value(baseClass) != 0);
    QVERIFY(classToAST.value(derivedClass) != 0);
}

QTEST_APPLESS_MAIN(tst_Lookup)
#include "tst_lookup.moc"

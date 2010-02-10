
#include <QtTest>
#include <QObject>

#include <AST.h>
#include <ASTVisitor.h>
#include <TranslationUnit.h>
#include <CppDocument.h>
#include <Literals.h>
#include <LookupContext.h>
#include <Name.h>
#include <ResolveExpression.h>
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
    ClassSymbols(TranslationUnit *translationUnit)
        : ASTVisitor(translationUnit)
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

    // Objective-C
    void simple_class_1();
    void class_with_baseclass();
    void class_with_protocol_with_protocol();
    void iface_impl_scoping();
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
    snapshot.insert(doc);

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

    ClassSymbols classSymbols(unit);
    classSymbols(ast);

    QCOMPARE(classSymbols.size(), 2);

    const QMap<Class *, ClassSpecifierAST *> classToAST =
            invert(classSymbols.asMap());

    QVERIFY(classToAST.value(baseClass) != 0);
    QVERIFY(classToAST.value(derivedClass) != 0);
}

void tst_Lookup::simple_class_1()
{
    const QByteArray source = "\n"
        "@interface Zoo {} +(id)alloc; -(id)init; @end\n"
        "@implementation Zoo +(id)alloc{} -(id)init{} -(void)dealloc{} @end\n";

    Document::Ptr doc = Document::create("simple_class_1");
    doc->setSource(source);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 2U);

    Snapshot snapshot;
    snapshot.insert(doc);

    Document::Ptr emptyDoc = Document::create("<empty>");

    ObjCClass *iface = doc->globalSymbolAt(0)->asObjCClass();
    QVERIFY(iface);
    QVERIFY(iface->isInterface());
    QCOMPARE(iface->memberCount(), 2U);

    ObjCClass *impl = doc->globalSymbolAt(1)->asObjCClass();
    QVERIFY(impl);
    QVERIFY(!impl->isInterface());
    QCOMPARE(impl->memberCount(), 3U);

    ObjCMethod *allocMethod = impl->memberAt(0)->asObjCMethod();
    QVERIFY(allocMethod);
    QVERIFY(allocMethod->name() && allocMethod->name()->identifier());
    QCOMPARE(QLatin1String(allocMethod->name()->identifier()->chars()), QLatin1String("alloc"));

    ObjCMethod *deallocMethod = impl->memberAt(2)->asObjCMethod();
    QVERIFY(deallocMethod);
    QVERIFY(deallocMethod->name() && deallocMethod->name()->identifier());
    QCOMPARE(QLatin1String(deallocMethod->name()->identifier()->chars()), QLatin1String("dealloc"));

    const LookupContext ctxt(impl, emptyDoc, doc, snapshot);

    // check class resolving:
    const QList<Symbol *> candidates = ctxt.resolveObjCClass(impl->name());
    QCOMPARE(candidates.size(), 2);
    QVERIFY(candidates.contains(iface));
    QVERIFY(candidates.contains(impl));

    // check scope expansion:
    QList<Scope *> expandedScopes;
    ctxt.expand(impl->members(), ctxt.visibleScopes(), &expandedScopes);
    QCOMPARE(expandedScopes.size(), 2);

    const ResolveExpression resolver(ctxt);

    // check method resolving:
    QList<LookupItem> results = resolver.resolveMember(allocMethod->name(), impl);
    QCOMPARE(results.size(), 2);
    QVERIFY(results.at(0).lastVisibleSymbol() == allocMethod || results.at(1).lastVisibleSymbol() == allocMethod);
    QVERIFY(results.at(0).lastVisibleSymbol()->asDeclaration() || results.at(1).lastVisibleSymbol()->asDeclaration());

    results = resolver.resolveMember(deallocMethod->name(), impl);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0).lastVisibleSymbol(), deallocMethod);
}

void tst_Lookup::class_with_baseclass()
{
    const QByteArray source = "\n"
                              "@implementation BaseZoo {} -(void)baseDecl; -(void)baseMethod{} @end\n"
                              "@interface Zoo: BaseZoo {} +(id)alloc; -(id)init; @end\n"
                              "@implementation Zoo +(id)alloc{} -(id)init{} -(void)dealloc{} @end\n";

    Document::Ptr doc = Document::create("class_with_baseclass");
    doc->setSource(source);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 3U);

    Snapshot snapshot;
    snapshot.insert(doc);

    Document::Ptr emptyDoc = Document::create("<empty>");

    ObjCClass *baseZoo = doc->globalSymbolAt(0)->asObjCClass();
    QVERIFY(baseZoo);
    QVERIFY(!baseZoo->isInterface());
    QCOMPARE(baseZoo->memberCount(), 2U);

    ObjCClass *zooIface = doc->globalSymbolAt(1)->asObjCClass();
    QVERIFY(zooIface);
    QVERIFY(zooIface->isInterface());
    QVERIFY(zooIface->baseClass()->name() == baseZoo->name());

    ObjCClass *zooImpl = doc->globalSymbolAt(2)->asObjCClass();
    QVERIFY(zooImpl);
    QVERIFY(!zooImpl->isInterface());
    QCOMPARE(zooImpl->memberCount(), 3U);

    Declaration *baseDecl = baseZoo->memberAt(0)->asDeclaration();
    QVERIFY(baseDecl);
    QVERIFY(baseDecl->name() && baseDecl->name()->identifier());
    QCOMPARE(QLatin1String(baseDecl->name()->identifier()->chars()), QLatin1String("baseDecl"));

    ObjCMethod *baseMethod = baseZoo->memberAt(1)->asObjCMethod();
    QVERIFY(baseMethod);
    QVERIFY(baseMethod->name() && baseMethod->name()->identifier());
    QCOMPARE(QLatin1String(baseMethod->name()->identifier()->chars()), QLatin1String("baseMethod"));

    const LookupContext ctxt(zooImpl, emptyDoc, doc, snapshot);

    const QList<Symbol *> candidates = ctxt.resolveObjCClass(baseZoo->name());
    QCOMPARE(candidates.size(), 1);
    QVERIFY(candidates.contains(baseZoo));

    QList<Scope *> expandedScopes;
    ctxt.expand(zooImpl->members(), ctxt.visibleScopes(), &expandedScopes);
    QCOMPARE(expandedScopes.size(), 3);

    const ResolveExpression resolver(ctxt);

    QList<LookupItem> results = resolver.resolveMember(baseDecl->name(), zooImpl);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0).lastVisibleSymbol(), baseDecl);

    results = resolver.resolveMember(baseMethod->name(), zooImpl);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0).lastVisibleSymbol(), baseMethod);
}

void tst_Lookup::class_with_protocol_with_protocol()
{
    const QByteArray source = "\n"
                              "@protocol P1 -(void)p1method; @end\n"
                              "@protocol P2 <P1> -(void)p2method; @end\n"
                              "@interface Zoo <P2> {} +(id)alloc; -(id)init; @end\n"
                              "@implementation Zoo +(id)alloc{} -(id)init{} -(void)dealloc{} @end\n";

    Document::Ptr doc = Document::create("class_with_protocol_with_protocol");
    doc->setSource(source);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 4U);

    Snapshot snapshot;
    snapshot.insert(doc);

    Document::Ptr emptyDoc = Document::create("<empty>");

    ObjCProtocol *P1 = doc->globalSymbolAt(0)->asObjCProtocol();
    QVERIFY(P1);
    QCOMPARE(P1->memberCount(), 1U);
    QCOMPARE(P1->protocolCount(), 0U);

    Declaration *p1method = P1->memberAt(0)->asDeclaration();
    QVERIFY(p1method);
    QCOMPARE(QLatin1String(p1method->name()->identifier()->chars()), QLatin1String("p1method"));

    ObjCProtocol *P2 = doc->globalSymbolAt(1)->asObjCProtocol();
    QVERIFY(P2);
    QCOMPARE(P2->memberCount(), 1U);
    QCOMPARE(P2->protocolCount(), 1U);
    QCOMPARE(QLatin1String(P2->protocolAt(0)->name()->identifier()->chars()), QLatin1String("P1"));

    ObjCClass *zooImpl = doc->globalSymbolAt(3)->asObjCClass();
    QVERIFY(zooImpl);

    const LookupContext ctxt(zooImpl, emptyDoc, doc, snapshot);

    {
        const QList<Symbol *> candidates = ctxt.resolveObjCProtocol(P1->name());
        QCOMPARE(candidates.size(), 1);
        QVERIFY(candidates.contains(P1));
    }

    {
        const QList<Symbol *> candidates = ctxt.resolveObjCProtocol(P2->protocolAt(0)->name());
        QCOMPARE(candidates.size(), 1);
        QVERIFY(candidates.contains(P1));
    }

    QList<Scope *> expandedScopes;
    ctxt.expand(zooImpl->members(), ctxt.visibleScopes(), &expandedScopes);
    QCOMPARE(expandedScopes.size(), 4);

    const ResolveExpression resolver(ctxt);

    QList<LookupItem> results = resolver.resolveMember(p1method->name(), zooImpl);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0).lastVisibleSymbol(), p1method);
}

void tst_Lookup::iface_impl_scoping()
{
    const QByteArray source = "\n"
                              "@interface Scooping{}-(int)method1:(int)arg;-(void)method2;@end\n"
                              "@implementation Scooping-(int)method1:(int)arg{return arg;}@end\n";

    Document::Ptr doc = Document::create("class_with_protocol_with_protocol");
    doc->setSource(source);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 2U);

    Snapshot snapshot;
    snapshot.insert(doc);

    Document::Ptr emptyDoc = Document::create("<empty>");
    ObjCClass *iface = doc->globalSymbolAt(0)->asObjCClass();
    QVERIFY(iface);
    QVERIFY(iface->isInterface());
    ObjCClass *impl = doc->globalSymbolAt(1)->asObjCClass();
    QVERIFY(impl);
    QVERIFY(!impl->isInterface());

    QCOMPARE(iface->memberCount(), 2U);
    QCOMPARE(impl->memberCount(), 1U);

    ObjCMethod *method1Impl = impl->memberAt(0)->asObjCMethod();
    QVERIFY(method1Impl);
    QCOMPARE(method1Impl->identifier()->chars(), "method1");

    // get the body of method1
    QCOMPARE(method1Impl->memberCount(), 1U);
    Block *method1Body = method1Impl->memberAt(0)->asBlock();
    QVERIFY(method1Body);

    const LookupContext ctxt(method1Body, emptyDoc, doc, snapshot);

    { // verify if we can resolve "arg" in the body
        QCOMPARE(method1Impl->argumentCount(), 1U);
        Argument *arg = method1Impl->argumentAt(0)->asArgument();
        QVERIFY(arg);
        QVERIFY(arg->name());
        QVERIFY(arg->name()->identifier());
        QCOMPARE(arg->name()->identifier()->chars(), "arg");

        const QList<Symbol *> candidates = ctxt.resolve(arg->name());
        QCOMPARE(candidates.size(), 1);
        QVERIFY(candidates.at(0)->type()->asIntegerType());
    }

    Declaration *method2 = iface->memberAt(1)->asDeclaration();
    QVERIFY(method2);
    QCOMPARE(method2->identifier()->chars(), "method2");

    { // verify if we can resolve "method2" in the body
        const QList<Symbol *> candidates = ctxt.resolve(method2->name());
        QCOMPARE(candidates.size(), 1);
        QCOMPARE(candidates.at(0), method2);
    }

    { // now let's see if the resolver can do the same for method2
        const ResolveExpression resolver(ctxt);

        const QList<LookupItem> results = resolver.resolveMember(method2->name(),
                                                                 impl);
        QCOMPARE(results.size(), 1);
        QCOMPARE(results.at(0).lastVisibleSymbol(), method2);
    }
}

QTEST_APPLESS_MAIN(tst_Lookup)
#include "tst_lookup.moc"

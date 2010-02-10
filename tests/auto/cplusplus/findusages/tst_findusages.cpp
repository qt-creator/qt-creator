
#include <QtTest>
#include <QObject>
#include <QList>

#include <AST.h>
#include <ASTVisitor.h>
#include <TranslationUnit.h>
#include <CppBindings.h>
#include <CppDocument.h>
#include <FindUsages.h>
#include <Literals.h>
#include <LookupContext.h>
#include <Name.h>
#include <ResolveExpression.h>
#include <Symbols.h>
#include <Overview.h>

using namespace CPlusPlus;

class CollectNames: public ASTVisitor
{
public:
    CollectNames(TranslationUnit *xUnit): ASTVisitor(xUnit) {}
    QList<NameAST*> operator()(const char *name) {
        _name = name;
        _exprs.clear();

        accept(translationUnit()->ast());

        return _exprs;
    }

    virtual bool preVisit(AST *ast) {
        if (NameAST *nameAst = ast->asName())
            if (!qstrcmp(_name, nameAst->name->identifier()->chars()))
                _exprs.append(nameAst);
        return true;
    }

private:
    QList<NameAST*> _exprs;
    const char *_name;
};

class tst_FindUsages: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void inlineMethod();

    // Objective-C
    void objc_args();
//    void objc_methods();
//    void objc_fields();
//    void objc_classes();
};

void tst_FindUsages::inlineMethod()
{
    const QByteArray src = "\n"
                           "class Tst {\n"
                           "  int method(int arg) {\n"
                           "    return arg;\n"
                           "  }\n"
                           "};\n";
    Document::Ptr doc = Document::create("inlineMethod");
    doc->setSource(src);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 1U);

    Snapshot snapshot;
    snapshot.insert(doc);

    Class *tst = doc->globalSymbolAt(0)->asClass();
    QVERIFY(tst);
    QCOMPARE(tst->memberCount(), 1U);
    Function *method = tst->memberAt(0)->asFunction();
    QVERIFY(method);
    QCOMPARE(method->argumentCount(), 1U);
    Argument *arg = method->argumentAt(0)->asArgument();
    QVERIFY(arg);
    QCOMPARE(arg->identifier()->chars(), "arg");

    FindUsages findUsages(doc, snapshot);
    findUsages.setGlobalNamespaceBinding(bind(doc, snapshot));
    findUsages(arg);
    QCOMPARE(findUsages.usages().size(), 2);
    QCOMPARE(findUsages.references().size(), 2);
}

#if 0
@interface Clazz {} +(void)method:(int)arg; @end
@implementation Clazz +(void)method:(int)arg {
  [Clazz method:arg];
}
@end
#endif
const QByteArray objcSource = "\n"
                              "@interface Clazz {} +(void)method:(int)arg; @end\n"
                              "@implementation Clazz +(void)method:(int)arg {\n"
                              "  [Clazz method:arg];\n"
                              "}\n"
                              "@end\n";

void tst_FindUsages::objc_args()
{
    Document::Ptr doc = Document::create("objc_args");
    doc->setSource(objcSource);
    doc->parse();
    doc->check();

    QVERIFY(doc->diagnosticMessages().isEmpty());
    QCOMPARE(doc->globalSymbolCount(), 2U);

    Snapshot snapshot;
    snapshot.insert(doc);

    TranslationUnit *xUnit = doc->translationUnit();
    QList<NameAST*>exprs = CollectNames(xUnit)("arg");
    QCOMPARE(exprs.size(), 3);

    ObjCClass *iface = doc->globalSymbolAt(0)->asObjCClass();
    QVERIFY(iface);
    QVERIFY(iface->isInterface());
    QCOMPARE(iface->memberCount(), 1U);

    Declaration *methodIface = iface->memberAt(0)->asDeclaration();
    QVERIFY(methodIface);
    QCOMPARE(methodIface->identifier()->chars(), "method");
    QVERIFY(methodIface->type()->isObjCMethodType());

    ObjCClass *impl = doc->globalSymbolAt(1)->asObjCClass();
    QVERIFY(impl);
    QVERIFY(!impl->isInterface());
    QCOMPARE(impl->memberCount(), 1U);

    ObjCMethod *methodImpl = impl->memberAt(0)->asObjCMethod();
    QVERIFY(methodImpl);
    QCOMPARE(methodImpl->identifier()->chars(), "method");
    QCOMPARE(methodImpl->argumentCount(), 1U);
    Argument *arg = methodImpl->argumentAt(0)->asArgument();
    QCOMPARE(arg->identifier()->chars(), "arg");

    FindUsages findUsages(doc, snapshot);
    findUsages.setGlobalNamespaceBinding(bind(doc, snapshot));
    findUsages(arg);
    QCOMPARE(findUsages.usages().size(), 3);
    QCOMPARE(findUsages.references().size(), 3);
}

QTEST_APPLESS_MAIN(tst_FindUsages)
#include "tst_findusages.moc"

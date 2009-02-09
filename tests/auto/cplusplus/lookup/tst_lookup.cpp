
#include <QtTest>
#include <QObject>
#include <CppDocument.h>
#include <LookupContext.h>
#include <Symbols.h>
#include <Overview.h>

CPLUSPLUS_USE_NAMESPACE

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

    Document::Ptr emptyDoc = Document::create("empty");

    Class *baseClass = doc->globalSymbolAt(0)->asClass();
    QVERIFY(baseClass);

    Class *derivedClass = doc->globalSymbolAt(1)->asClass();
    QVERIFY(derivedClass);

    LookupContext ctx(derivedClass, emptyDoc, doc, snapshot);

    const QList<Symbol *> candidates =
        ctx.resolveClass(derivedClass->baseClassAt(0)->name());

    QCOMPARE(candidates.size(), 1);
    QCOMPARE(candidates.at(0), baseClass);
}

QTEST_APPLESS_MAIN(tst_Lookup)
#include "tst_lookup.moc"

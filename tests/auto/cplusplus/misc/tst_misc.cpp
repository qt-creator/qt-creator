#include <CppDocument.h>

#include <QtTest>
#include <QtDebug>

using namespace CPlusPlus;

class tst_Misc: public QObject
{
    Q_OBJECT

private slots:
    void diagnosticClient_error();
    void diagnosticClient_warning();
};

void tst_Misc::diagnosticClient_error()
{
    const QByteArray src("\n"
                         "class Foo {}\n"
                         );
    Document::Ptr doc = Document::create("diagnosticClient_error");
    QVERIFY(!doc.isNull());
    doc->setSource(src);
    bool success = doc->parse(Document::ParseTranlationUnit);
    QVERIFY(success);

    QList<Document::DiagnosticMessage> diagnostics = doc->diagnosticMessages();
    QVERIFY(diagnostics.size() == 1);

    const Document::DiagnosticMessage &msg = diagnostics.at(0);
    QCOMPARE(msg.level(), (int) Document::DiagnosticMessage::Error);
    QCOMPARE(msg.line(), 2U);
    QCOMPARE(msg.column(), 1U);
}

void tst_Misc::diagnosticClient_warning()
{
    const QByteArray src("\n"
                         "using namespace ;\n"
                         );
    Document::Ptr doc = Document::create("diagnosticClient_warning");
    QVERIFY(!doc.isNull());
    doc->setSource(src);
    bool success = doc->parse(Document::ParseTranlationUnit);
    QVERIFY(success);

    QList<Document::DiagnosticMessage> diagnostics = doc->diagnosticMessages();
    QVERIFY(diagnostics.size() == 1);

    const Document::DiagnosticMessage &msg = diagnostics.at(0);
    QCOMPARE(msg.level(), (int) Document::DiagnosticMessage::Warning);
    QCOMPARE(msg.line(), 1U);
    QCOMPARE(msg.column(), 17U);
}

QTEST_MAIN(tst_Misc)
#include "tst_misc.moc"

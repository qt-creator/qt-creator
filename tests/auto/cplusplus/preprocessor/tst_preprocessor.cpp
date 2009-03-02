#include <QtTest>
#include <pp.h>

CPLUSPLUS_USE_NAMESPACE

class tst_Preprocessor: public QObject
{
Q_OBJECT

private Q_SLOTS:
    void pp_with_no_client();
};

void tst_Preprocessor::pp_with_no_client()
{
    using namespace CPlusPlus;

    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, env);
    QByteArray preprocessed = preprocess("#define foo(a,b) a + b\nfoo(1, 2)\n");
    QByteArray expected = "1 + 2";
    QCOMPARE(preprocessed.trimmed(), expected);
}

QTEST_APPLESS_MAIN(tst_Preprocessor)
#include "tst_preprocessor.moc"

#include <QtTest>
#include <pp.h>

using namespace CPlusPlus;

class tst_Preprocessor: public QObject
{
Q_OBJECT

private Q_SLOTS:
    void pp_with_no_client();

    void unfinished_function_like_macro_call();
};

void tst_Preprocessor::pp_with_no_client()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess(QLatin1String("<stdin>"),
                                         QByteArray("\n#define foo(a,b) a + b"
                                         "\nfoo(1, 2)\n"));
    QByteArray expected = "1 + 2";
    QCOMPARE(preprocessed.trimmed(), expected);
}

void tst_Preprocessor::unfinished_function_like_macro_call()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess(QLatin1String("<stdin>"),
                                         QByteArray("\n#define foo(a,b) a + b"
                                         "\nfoo(1, 2\n"));
    QByteArray expected = "foo";
    QCOMPARE(preprocessed.trimmed(), expected);
}

QTEST_APPLESS_MAIN(tst_Preprocessor)
#include "tst_preprocessor.moc"

#include <QtTest>
#include <pp.h>

using namespace CPlusPlus;

class tst_Preprocessor: public QObject
{
Q_OBJECT

private Q_SLOTS:
    void unfinished_function_like_macro_call();
};

void tst_Preprocessor::unfinished_function_like_macro_call()
{
    Client *client = 0; // no client.
    Environment env;

    Preprocessor preprocess(client, &env);
    QByteArray preprocessed = preprocess(QLatin1String("<stdin>"),
                                         QByteArray("\n#define foo(a,b) a + b"
                                         "\nfoo(1, 2\n"));

    QCOMPARE(preprocessed.trimmed(), QByteArray("foo"));
}

QTEST_APPLESS_MAIN(tst_Preprocessor)
#include "tst_preprocessor.moc"

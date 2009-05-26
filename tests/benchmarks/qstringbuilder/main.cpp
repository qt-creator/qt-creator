
#include "qstringbuilder.h"

#include <QDebug>
#include <QString>

#include <qtest.h>


#define COMPARE(a, b) QCOMPARE(a, b)
//#define COMPARE(a, b)

#define SEP(s) qDebug() << "\n\n-------- " s "  ---------";
#define L(s) QLatin1String(s)

class tst_qstringbuilder : public QObject
{
    Q_OBJECT

public:
    tst_qstringbuilder()
      : l1literal("some string literal"),
        l1string("some string literal"),
        ba("some string literal"),
        string(l1string),
        stringref(&string, 2, 10),
        achar('c')
    {}


public:
    enum { N = 10000 };

    int run_traditional()
    {
        int s = 0;
        for (int i = 0; i < N; ++i) {
#if 0
            s += QString(l1string + l1string).size();
            s += QString(l1string + l1string + l1string).size();
            s += QString(l1string + l1string + l1string + l1string).size();
            s += QString(l1string + l1string + l1string + l1string + l1string).size();
#endif
            s += QString(achar + l1string + achar).size();
        }
        return s;
    }

    int run_builder()
    {
        int s = 0;
        for (int i = 0; i < N; ++i) {
#if 0
            s += QString(l1literal % l1literal).size();
            s += QString(l1literal % l1literal % l1literal).size();
            s += QString(l1literal % l1literal % l1literal % l1literal).size();
            s += QString(l1literal % l1literal % l1literal % l1literal % l1literal).size();
#endif
            s += QString(achar % l1literal % achar).size();
        }
        return s;
    }

private slots:

    void separator_0() {
        qDebug() << "\nIn each block the QStringBuilder based result appear first, "
            "QStringBased second.\n";
    }

    void separator_1() { SEP("literal + literal  (builder first)"); }

    void b_2_l1literal() {
        QBENCHMARK { r = l1literal % l1literal; }
        COMPARE(r, l1string + l1string);
    }
    void s_2_l1string() {
        QBENCHMARK { r = l1string + l1string; }
        COMPARE(r, QString(l1literal % l1literal));
    }


    void separator_2() { SEP("2 strings"); }

    void b_2_string() {
        QBENCHMARK { r = string % string; }
        COMPARE(r, string + string);
    }
    void s_2_string() {
        QBENCHMARK { r = string + string; }
        COMPARE(r, QString(string % string));
    }


    void separator_2c() { SEP("2 string refs"); }

    void b_2_stringref() {
        QBENCHMARK { r = stringref % stringref; }
        COMPARE(r, stringref.toString() + stringref.toString());
    }
    void s_2_stringref() {
        QBENCHMARK { r = stringref.toString() + stringref.toString(); }
        COMPARE(r, QString(stringref % stringref));
    }


    void separator_2b() { SEP("3 strings"); }

    void b_3_string() {
        QBENCHMARK { r = string % string % string; }
        COMPARE(r, string + string + string);
    }
    void s_3_string() {
        QBENCHMARK { r = string + string + string; }
        COMPARE(r, QString(string % string % string));
    }


    void separator_2a() { SEP("string + literal  (builder first)"); }

    void b_string_l1literal() {
        QBENCHMARK { r = string % l1literal; }
        COMPARE(r, string + l1string);
    }
    void b_string_l1string() {
        QBENCHMARK { r = string % l1string; }
        COMPARE(r, string + l1literal);
    }
    void s_string_l1literal() {
        QBENCHMARK { r = string + l1literal; }
        COMPARE(r, QString(string % l1string));
    }
    void s_string_l1string() {
        QBENCHMARK { r = string + l1string; }
        COMPARE(r, QString(string % l1literal));
    }


    void separator_3() { SEP("3 literals"); }

    void b_3_l1literal() {
        QBENCHMARK { r = l1literal % l1literal % l1literal; }
        COMPARE(r, l1string + l1string + l1string);
    }
    void s_3_l1string() {
        QBENCHMARK { r = l1string + l1string + l1string; }
        COMPARE(r, QString(l1literal % l1literal % l1literal));
    }


    void separator_4() { SEP("4 literals"); }

    void b_4_l1literal() {
        QBENCHMARK { r = l1literal % l1literal % l1literal % l1literal; }
        COMPARE(r, l1string + l1string + l1string + l1string);
    }
    void s_4_l1string() {
        QBENCHMARK { r = l1string + l1string + l1string + l1string; }
        COMPARE(r, QString(l1literal % l1literal % l1literal % l1literal));
    }


    void separator_5() { SEP("5 literals"); }

    void b_5_l1literal() {
        QBENCHMARK { r = l1literal % l1literal % l1literal % l1literal %l1literal; }
        COMPARE(r, l1string + l1string + l1string + l1string + l1string);
    }

    void s_5_l1string() {
        QBENCHMARK { r = l1string + l1string + l1string + l1string + l1string; }
        COMPARE(r, QString(l1literal % l1literal % l1literal % l1literal % l1literal));
    }


    void separator_6() { SEP("4 chars"); }

    void b_string_4_char() {
        QBENCHMARK { r = string + achar + achar + achar + achar; }
        COMPARE(r, QString(string % achar % achar % achar % achar));
    }

    void s_string_4_char() {
        QBENCHMARK { r = string + achar + achar + achar + achar; }
        COMPARE(r, QString(string % achar % achar % achar % achar));
    }


    void separator_7() { SEP("char + string + char"); }

    void b_char_string_char() {
        QBENCHMARK { r = achar + string + achar; }
        COMPARE(r, QString(achar % string % achar));
    }

    void s_char_string_char() {
        QBENCHMARK { r = achar + string + achar; }
        COMPARE(r, QString(achar % string % achar));
    }

    void separator_8() { SEP("string.arg"); }

    void b_string_arg() {
        const QString pattern = l1string + "%1" + l1string;
        QBENCHMARK { r = l1literal % string % l1literal; }
        COMPARE(r, l1string + string + l1string);
    }

    void s_string_arg() {
        const QString pattern = l1string + "%1" + l1string;
        QBENCHMARK { r = pattern.arg(string); }
        COMPARE(r, l1string + string + l1string);
    }

    void s_bytearray_arg() {
        QByteArray result;
        QBENCHMARK { result = ba + ba + ba; }
    }

private:
    const QLatin1Literal l1literal;
    const QLatin1String l1string;
    const QByteArray ba;
    const QString string;
    const QStringRef stringref;
    const QLatin1Char achar;

    QString r;
};


int main(int argc, char *argv[])
{
    if (argc == 2 && (argv[1] == L("--run-builder") || argv[1] == L("-b"))) {
        tst_qstringbuilder test;
        return test.run_builder();
    }

    if (argc == 2 && (argv[1] == L("--run-traditional") || argv[1] == L("-t"))) {
        tst_qstringbuilder test;
        return test.run_traditional();
    }

    if (argc == 1) {
        QCoreApplication app(argc, argv);
        QStringList args = app.arguments();
        tst_qstringbuilder test;
        return QTest::qExec(&test, argc, argv);
    }

    qDebug() << "Usage: " << argv[0] << " [--run-builder|-r|--run-traditional|-t]";
}


#include "main.moc"

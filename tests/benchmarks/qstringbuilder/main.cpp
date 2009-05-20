
#include "qstringbuilder.h"

#include <QDebug>
#include <QString>

#include <qtest.h>


#define COMPARE(a, b) QCOMPARE(a, b)
//#define COMPARE(a, b)

#define SEP(s) qDebug() << "\n------- " s "  ----------";

class tst_qstringbuilder : public QObject
{
    Q_OBJECT

public:
    tst_qstringbuilder();

private slots:
    void separator_2() { SEP("string + string  (builder first)"); }
    void b_2_l1literal();
    void s_2_l1string();

    void separator_3() { SEP("3 strings"); }
    void b_3_l1literal();
    void s_3_l1string();

    void separator_4() { SEP("4 strings"); }
    void b_4_l1literal();
    void s_4_l1string();

    void separator_5() { SEP("5 strings"); }
    void b_5_l1literal();
    void s_5_l1string();

    void separator_6() { SEP("4 chars"); }
    void b_string_4_char();
    void s_string_4_char();

    void separator_7() { SEP("char + string + char"); }
    void b_char_string_char();
    void s_char_string_char();

private:
    const QLatin1Literal l1literal;
    const QLatin1String l1string;
    const QString string;
    const char achar;
};


tst_qstringbuilder::tst_qstringbuilder()
  : l1literal("some literal"),
    l1string("some literal"),
    string(l1string),
    achar('c')
{}

void tst_qstringbuilder::b_2_l1literal()
{
    QString result;
    QBENCHMARK { result = l1literal % l1literal; }
    COMPARE(result, l1string + l1string);
}

void tst_qstringbuilder::b_3_l1literal()
{
    QString result;
    QBENCHMARK { result = l1literal % l1literal % l1literal; }
    COMPARE(result, l1string + l1string + l1string);
}

void tst_qstringbuilder::b_4_l1literal()
{
    QString result;
    QBENCHMARK { result = l1literal % l1literal % l1literal % l1literal; }
    COMPARE(result, l1string + l1string + l1string + l1string);
}

void tst_qstringbuilder::b_5_l1literal()
{
    QString result;
    QBENCHMARK { result = l1literal % l1literal % l1literal % l1literal %l1literal; }
    COMPARE(result, l1string + l1string + l1string + l1string + l1string);
}

void tst_qstringbuilder::b_string_4_char()
{
    QString result;
    QBENCHMARK { result = string + achar + achar + achar + achar; }
    COMPARE(result, QString(string % achar % achar % achar % achar));
}

void tst_qstringbuilder::b_char_string_char()
{
    QString result;
    QBENCHMARK { result = achar + string + achar; }
    COMPARE(result, QString(achar % string % achar));
}


void tst_qstringbuilder::s_2_l1string()
{
    QString result;
    QBENCHMARK { result = l1string + l1string; }
    COMPARE(result, QString(l1literal % l1literal));
}

void tst_qstringbuilder::s_3_l1string()
{
    QString result;
    QBENCHMARK { result = l1string + l1string + l1string; }
    COMPARE(result, QString(l1literal % l1literal % l1literal));
}

void tst_qstringbuilder::s_4_l1string()
{
    QString result;
    QBENCHMARK { result = l1string + l1string + l1string + l1string; }
    COMPARE(result, QString(l1literal % l1literal % l1literal % l1literal));
}

void tst_qstringbuilder::s_5_l1string()
{
    QString result;
    QBENCHMARK { result = l1string + l1string + l1string + l1string + l1string; }
    COMPARE(result, QString(l1literal % l1literal % l1literal % l1literal % l1literal));
}

void tst_qstringbuilder::s_string_4_char()
{
    QString result;
    QBENCHMARK { result = string + achar + achar + achar + achar; }
    COMPARE(result, QString(string % achar % achar % achar % achar));
}

void tst_qstringbuilder::s_char_string_char()
{
    QString result;
    QBENCHMARK { result = achar + string + achar; }
    COMPARE(result, QString(achar % string % achar));
}

QTEST_MAIN(tst_qstringbuilder)

#include "main.moc"

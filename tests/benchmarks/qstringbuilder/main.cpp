
#include "qstringbuilder.h"

#include <QDebug>
#include <QString>

#include <qtest.h>


#define COMPARE(a, b) QCOMPARE(a, b)
//#define COMPARE(a, b)

class tst_qstringbuilder : public QObject
{
    Q_OBJECT

public:
    tst_qstringbuilder();

private slots:
    // QStringBuilder based 
    void b_separator() { qDebug() << "\n------- QStringBuilder based ----------"; }
    void b_2_l1literal();
    void b_3_l1literal();
    void b_4_l1literal();
    void b_5_l1literal();
    void b_string_4_char();
    void b_char_string_char();

    // QString based for comparison
    void s_separator() { qDebug() << "\n-------- QString based ---------"; }
    void s_2_l1string();
    void s_3_l1string();
    void s_4_l1string();
    void s_5_l1string();
    void s_string_4_char();
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

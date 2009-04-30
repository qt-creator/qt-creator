
#include <QDebug>
#include <QString>

#include "../../../src/libs/utils/qstringbuilder.h"

#include <qtest.h>


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
    void b_string_4_char();

    // QString based for comparison
    void s_separator() { qDebug() << "\n-------- QString based ---------"; }
    void s_2_l1string();
    void s_3_l1string();
    void s_4_l1string();
    void s_string_4_char();

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
    QCOMPARE(result, l1string + l1string);
}

void tst_qstringbuilder::b_3_l1literal()
{
    QString result;
    QBENCHMARK { result = l1literal % l1literal % l1literal; }
    QCOMPARE(result, l1string + l1string + l1string);
}

void tst_qstringbuilder::b_4_l1literal()
{
    QString result;
    QBENCHMARK { result = l1literal % l1literal % l1literal % l1literal; }
    QCOMPARE(result, l1string + l1string + l1string + l1string);
}

void tst_qstringbuilder::b_string_4_char()
{
    QString result;
    QBENCHMARK { result = string + achar + achar + achar; }
    QCOMPARE(result, QString(string % achar % achar % achar));
}


void tst_qstringbuilder::s_2_l1string()
{
    QString result;
    QBENCHMARK { result = l1string + l1string; }
    QCOMPARE(result, QString(l1literal % l1literal));
}

void tst_qstringbuilder::s_3_l1string()
{
    QString result;
    QBENCHMARK { result = l1string + l1string + l1string; }
    QCOMPARE(result, QString(l1literal % l1literal % l1literal));
}

void tst_qstringbuilder::s_4_l1string()
{
    QString result;
    QBENCHMARK { result = l1string + l1string + l1string + l1string; }
    QCOMPARE(result, QString(l1literal % l1literal % l1literal % l1literal));
}

void tst_qstringbuilder::s_string_4_char()
{
    QString result;
    QBENCHMARK { result = string + achar + achar + achar; }
    QCOMPARE(result, QString(string % achar % achar % achar));
}

QTEST_MAIN(tst_qstringbuilder)

#include "main.moc"


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
    void builderbased_l1literal_l1literal();
    void builderbased_l1literal_l1literal_l1literal();
    void builderbased_l1literal_l1literal_l1literal_l1literal();

    // QString based for comparison
    void stringbased_l1string_l1string();
    void stringbased_l1string_l1string_l1string();
    void stringbased_l1string_l1string_l1string_l1string();

private:
    const QLatin1Literal l1literal;
    const QLatin1String l1string;
};


tst_qstringbuilder::tst_qstringbuilder()
    : l1literal("some literal"), l1string("some literal")
{}

void tst_qstringbuilder::builderbased_l1literal_l1literal()
{
    QString result;
    QBENCHMARK { result = l1literal % l1literal; }
    QCOMPARE(result, l1string + l1string);
}

void tst_qstringbuilder::builderbased_l1literal_l1literal_l1literal()
{
    QString result;
    QBENCHMARK { result = l1literal % l1literal % l1literal; }
    QCOMPARE(result, l1string + l1string + l1string);
}

void tst_qstringbuilder::builderbased_l1literal_l1literal_l1literal_l1literal()
{
    QString result;
    QBENCHMARK { result = l1literal % l1literal % l1literal % l1literal; }
    QCOMPARE(result, l1string + l1string + l1string + l1string);
}



void tst_qstringbuilder::stringbased_l1string_l1string()
{
    QString result;
    QBENCHMARK { result = l1string + l1string; }
    QCOMPARE(result, QString(l1literal % l1literal));
}

void tst_qstringbuilder::stringbased_l1string_l1string_l1string()
{
    QString result;
    QBENCHMARK { result = l1string + l1string + l1string; }
    QCOMPARE(result, QString(l1literal % l1literal % l1literal));
}

void tst_qstringbuilder::stringbased_l1string_l1string_l1string_l1string()
{
    QString result;
    QBENCHMARK { result = l1string + l1string + l1string + l1string; }
    QCOMPARE(result, QString(l1literal % l1literal % l1literal % l1literal));
}

QTEST_MAIN(tst_qstringbuilder)

#include "main.moc"

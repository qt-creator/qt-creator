/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
****************************************************************************/

#include <QtTest/QtTest>
#include "../../../src/libs/utils/qstringbuilder.h"

//TESTED_CLASS=QStringBuilder
//TESTED_FILES=qstringbuilder.h

class tst_QStringBuilder : public QObject
{
    Q_OBJECT

public:
    tst_QStringBuilder();
    ~tst_QStringBuilder() {}

public slots:
    void init() {}
    void cleanup() {}

private slots:
    void operator_percent();
};


tst_QStringBuilder::tst_QStringBuilder()
{
    //QTextCodec::setCodecForLocale(QTextCodec::codecForName("ISO 8859-1"));
}

void tst_QStringBuilder::operator_percent()
{
    QLatin1Literal l1literal("a literal");
    QLatin1String l1string("a literal");
    QLatin1Char l1char('c');
    QChar qchar(l1char);

    QCOMPARE(QString(l1literal % l1literal), QString(l1string + l1string));
}

QTEST_APPLESS_MAIN(tst_QStringBuilder)

#include "tst_qstringbuilder.moc"

/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "../../../src/shared/proparser/ioutils.cpp"

#include <QtTest/QtTest>

class tst_IoUtils : public QObject
{
    Q_OBJECT

private slots:
    void quoteArg_data();
    void quoteArg();
};

void tst_IoUtils::quoteArg_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");

    static const struct {
        const char * const in;
        const char * const out;
    } vals[] = {
#ifdef Q_OS_WIN
        { "", "\"\"" },
        { "hallo", "hallo" },
        { "hallo du", "\"hallo du\"" },
        { "hallo\\", "hallo\\" },
        { "hallo du\\", "\"hallo du\"\\" },
        { "ha\"llo", "\"ha\"\\^\"\"llo\"" },
        { "ha\\\"llo", "\"ha\"\\\\\\^\"\"llo\"" },
#else
        { "", "\"\"" },
        { "hallo", "hallo" },
        { "hallo du", "'hallo du'" },
        { "ha'llo", "'ha'\\''llo'" },
#endif
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++)
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << QString::fromLatin1(vals[i].out);
}

void tst_IoUtils::quoteArg()
{
    QFETCH(QString, in);
    QFETCH(QString, out);

    QCOMPARE(IoUtils::shellQuote(in), out);
}

QTEST_MAIN(tst_IoUtils)

#include "tst_ioutils.moc"

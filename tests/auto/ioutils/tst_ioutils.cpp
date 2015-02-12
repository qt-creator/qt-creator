/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <QtTest>
#include <proparser/ioutils.h>


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

    QCOMPARE(QMakeInternal::IoUtils::shellQuote(in), out);
}

QTEST_MAIN(tst_IoUtils)

#include "tst_ioutils.moc"

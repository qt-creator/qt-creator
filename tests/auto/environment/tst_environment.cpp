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

#include <utils/environment.h>

#include <QtTest>

using namespace Utils;

class tst_Environment : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void environment_data();
    void environment();

private:
    Environment env;
};

void tst_Environment::initTestCase()
{
    env.set("word", "hi");
}

void tst_Environment::environment_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");

    static const struct {
        const char * const in;
        const char * const out;
    } vals[] = {
#ifdef Q_OS_WIN
        { "", "" },
        { "hi", "hi" },
        { "hi%", "hi%" },
        { "hi% ho", "hi% ho" },
        { "hi%here ho", "hi%here ho" },
        { "hi%here%ho", "hi%here%ho" },
        { "hi%word%", "hihi" },
        { "hi%foo%word%", "hi%foohi" },
        { "%word%word%ho", "hiword%ho" },
        { "hi%word%x%word%ho", "hihixhiho" },
        { "hi%word%xx%word%ho", "hihixxhiho" },
        { "hi%word%%word%ho", "hihihiho" },
#else
        { "", "" },
        { "hi", "hi" },
        { "hi$", "hi$" },
        { "hi${", "hi${" },
        { "hi${word", "hi${word" },
        { "hi${word}", "hihi" },
        { "hi${word}ho", "hihiho" },
        { "hi$wo", "hi$wo" },
        { "hi$word", "hihi" },
        { "hi$word ho", "hihi ho" },
        { "$word", "hi" },
        { "hi${word}$word", "hihihi" },
#endif
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++)
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << QString::fromLatin1(vals[i].out);
}

void tst_Environment::environment()
{
    QFETCH(QString, in);
    QFETCH(QString, out);

    QCOMPARE(env.expandVariables(in), out);
}

QTEST_MAIN(tst_Environment)

#include "tst_environment.moc"

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

#include <utils/environment.h>

#include <QtTest/QtTest>

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

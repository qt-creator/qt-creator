/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <stringutils.h>

#include <QtTest>

//TESTED_COMPONENT=src/libs/utils

class TestMacroExpander : public Utils::AbstractQtcMacroExpander
{
public:
    virtual bool resolveMacro(const QString &name, QString *ret)
    {
        if (name == QLatin1String("a")) {
            *ret = QLatin1String("hi");
            return true;
        }
        return false;
    }
};

class tst_StringUtils : public QObject
{
    Q_OBJECT

private slots:
    void testWithTildeHomePath();
    void testMacroExpander_data();
    void testMacroExpander();

private:
    TestMacroExpander mx;
};

void tst_StringUtils::testWithTildeHomePath()
{
#ifndef Q_OS_WIN
    // home path itself
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath()), QString::fromLatin1("~"));
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QLatin1Char('/')),
             QString::fromLatin1("~"));
    QCOMPARE(Utils::withTildeHomePath(QString::fromLatin1("/unclean/..") + QDir::homePath()),
             QString::fromLatin1("~"));
    // sub of home path
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/foo")),
             QString::fromLatin1("~/foo"));
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/foo/")),
             QString::fromLatin1("~/foo"));
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/some/path/file.txt")),
             QString::fromLatin1("~/some/path/file.txt"));
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/some/unclean/../path/file.txt")),
             QString::fromLatin1("~/some/path/file.txt"));
    // not sub of home path
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/../foo")),
             QString(QDir::homePath() + QString::fromLatin1("/../foo")));
#else
    // windows: should return same as input
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath()), QDir::homePath());
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/foo")),
             QDir::homePath() + QString::fromLatin1("/foo"));
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/../foo")),
             Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/../foo")));
#endif
}

void tst_StringUtils::testMacroExpander_data()

{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");

    static const struct {
        const char * const in;
        const char * const out;
    } vals[] = {
        { "text", "text" },
        { "%{a}", "hi" },
        { "pre%{a}", "prehi" },
        { "%{a}post", "hipost" },
        { "pre%{a}post", "prehipost" },
        { "%{a}%{a}", "hihi" },
        { "%{a}text%{a}", "hitexthi" },
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++)
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << QString::fromLatin1(vals[i].out);
}

void tst_StringUtils::testMacroExpander()
{
    QFETCH(QString, in);
    QFETCH(QString, out);

    Utils::expandMacros(&in, &mx);
    QCOMPARE(in, out);
}

QTEST_MAIN(tst_StringUtils)

#include "tst_stringutils.moc"

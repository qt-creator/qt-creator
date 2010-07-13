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

#include <stringutils.h>

#include <QtTest/QtTest>

class tst_StringUtils : public QObject
{
    Q_OBJECT

private slots:
    void testWithTildeHomePath();

};

void tst_StringUtils::testWithTildeHomePath()
{
#ifndef Q_OS_WIN
    // home path itself
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath()), QLatin1String("~"));
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QLatin1Char('/')),
             QLatin1String("~"));
    QCOMPARE(Utils::withTildeHomePath(QLatin1String("/unclean/..") + QDir::homePath()),
             QLatin1String("~"));
    // sub of home path
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QLatin1String("/foo")),
             QLatin1String("~/foo"));
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QLatin1String("/foo/")),
             QLatin1String("~/foo"));
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QLatin1String("/some/path/file.txt")),
             QLatin1String("~/some/path/file.txt"));
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QLatin1String("/some/unclean/../path/file.txt")),
             QLatin1String("~/some/path/file.txt"));
    // not sub of home path
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QLatin1String("/../foo")),
             QDir::homePath() + QLatin1String("/../foo"));
#else
    // windows: should return same as input
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath()), QDir::homePath());
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QLatin1String("/foo")),
             QDir::homePath() + QLatin1String("/foo"));
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QLatin1String("/../foo")),
             Utils::withTildeHomePath(QDir::homePath() + QLatin1String("/../foo")));
#endif
}

QTEST_MAIN(tst_StringUtils)

#include "tst_stringutils.moc"

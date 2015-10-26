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

#include <utils/templateengine.h>

#include <QtTest>

//TESTED_COMPONENT=src/libs/utils

class tst_TemplateEngine : public QObject
{
    Q_OBJECT

private slots:
    void testTemplateEngine_data();
    void testTemplateEngine();
};

void tst_TemplateEngine::testTemplateEngine_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedOutput");
    QTest::addColumn<QString>("expectedErrorMessage");
    QTest::newRow("if")
        << QString::fromLatin1("@if 1\nline 1\n@elsif 0\nline 2\n@else\nline 3\n@endif\n")
        << QString::fromLatin1("line 1\n\n")
        << QString();
    QTest::newRow("elsif")
        << QString::fromLatin1("@if 0\nline 1\n@elsif 1\nline 2\n@else\nline 3\n@endif\n")
        << QString::fromLatin1("line 2\n\n")
        << QString();
    QTest::newRow("else")
        << QString::fromLatin1("@if 0\nline 1\n@elsif 0\nline 2\n@else\nline 3\n@endif\n")
        << QString::fromLatin1("line 3\n\n")
        << QString();
    QTest::newRow("nested-if")
        << QString::fromLatin1("@if 1\n"
                               "  @if 1\nline 1\n@elsif 0\nline 2\n@else\nline 3\n@endif\n"
                               "@else\n"
                               "  @if 1\nline 4\n@elsif 0\nline 5\n@else\nline 6\n@endif\n"
                               "@endif\n")
        << QString::fromLatin1("line 1\n\n")
        << QString();
    QTest::newRow("nested-else")
        << QString::fromLatin1("@if 0\n"
                               "  @if 1\nline 1\n@elsif 0\nline 2\n@else\nline 3\n@endif\n"
                               "@else\n"
                               "  @if 1\nline 4\n@elsif 0\nline 5\n@else\nline 6\n@endif\n"
                               "@endif\n")
        << QString::fromLatin1("line 4\n\n")
        << QString();
    QTest::newRow("twice-nested-if")
        << QString::fromLatin1("@if 0\n"
                               "  @if 1\n"
                               "    @if 1\nline 1\n@else\nline 2\n@endif\n"
                               "  @endif\n"
                               "@else\n"
                               "  @if 1\n"
                               "    @if 1\nline 3\n@else\nline 4\n@endif\n"
                               "  @endif\n"
                               "@endif\n")
        << QString::fromLatin1("line 3\n\n")
        << QString();
}

void tst_TemplateEngine::testTemplateEngine()
{
    QFETCH(QString, input);
    QFETCH(QString, expectedOutput);
    QFETCH(QString, expectedErrorMessage);

    QString errorMessage;
    QString output = Utils::TemplateEngine::processText(Utils::globalMacroExpander(), input, &errorMessage);

    QCOMPARE(output, expectedOutput);
    QCOMPARE(errorMessage, expectedErrorMessage);
}


QTEST_MAIN(tst_TemplateEngine)

#include "tst_templateengine.moc"

/****************************************************************************
**
** Copyright (C) 2015 Andre Hartmann <aha_1980@gmx.de>
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

#include <utils/ansiescapecodehandler.h>

#include <QString>
#include <QtTest>

using namespace Utils;

typedef QList<FormattedText> FormattedTextList;

Q_DECLARE_METATYPE(QTextCharFormat);
Q_DECLARE_METATYPE(FormattedText);
Q_DECLARE_METATYPE(FormattedTextList);

static QString ansiEscape(const QByteArray &sequence)
{
    return QString::fromLatin1("\x1b[" + sequence);
}

class tst_AnsiEscapeCodeHandler : public QObject
{
    Q_OBJECT

public:
    tst_AnsiEscapeCodeHandler();

private Q_SLOTS:
    void testSimpleFormat();
    void testSimpleFormat_data();
    void testLineOverlappingFormat();
    void testSplitControlSequence();
    void oneLineBenchmark();
    void multiLineBenchmark();

private:
    const QString red;
    const QString redBackground;
    const QString red256;
    const QString redBackground256;
    const QString mustard256;
    const QString mustardBackground256;
    const QString gray256;
    const QString grayBackground256;
    const QString mustardRgb;
    const QString mustardBackgroundRgb;
    const QString bold;
    const QString normal;
    const QString normal1;
    const QString eraseToEol;
};

tst_AnsiEscapeCodeHandler::tst_AnsiEscapeCodeHandler() :
    red(ansiEscape("31m")),
    redBackground(ansiEscape("41m")),
    red256(ansiEscape("38;5;1m")),
    redBackground256(ansiEscape("48;5;1m")),
    mustard256(ansiEscape("38;5;220m")),
    mustardBackground256(ansiEscape("48;5;220m")),
    gray256(ansiEscape("38;5;250m")),
    grayBackground256(ansiEscape("48;5;250m")),
    mustardRgb(ansiEscape("38;2;255;204;0m")),
    mustardBackgroundRgb(ansiEscape("48;2;255;204;0m")),
    bold(ansiEscape("1m")),
    normal(ansiEscape("0m")),
    normal1(ansiEscape("m")),
    eraseToEol(ansiEscape("K"))
{
}

void tst_AnsiEscapeCodeHandler::testSimpleFormat()
{
    QFETCH(QString, text);
    QFETCH(QTextCharFormat, format);
    QFETCH(FormattedTextList, expected);

    AnsiEscapeCodeHandler handler;
    FormattedTextList result = handler.parseText(FormattedText(text, format));
    handler.endFormatScope();

    QCOMPARE(result.size(), expected.size());
    for (int i = 0; i < result.size(); ++i) {
        QCOMPARE(result[i].text, expected[i].text);
        QCOMPARE(result[i].format, expected[i].format);
    }
}

void tst_AnsiEscapeCodeHandler::testSimpleFormat_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QTextCharFormat>("format");
    QTest::addColumn<FormattedTextList>("expected");

    // Test pass-through
    QTextCharFormat defaultFormat;
    QTest::newRow("Pass-through") << "Hello World" << defaultFormat
                       << (FormattedTextList() << FormattedText("Hello World", defaultFormat));

    QTextCharFormat redFormat;
    redFormat.setForeground(QColor(170, 0, 0));
    QTest::newRow("Text-color change (8 color)")
            << QString("This is " + red + "red" + normal + " text") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("This is ", defaultFormat)
                << FormattedText("red", redFormat)
                << FormattedText(" text", defaultFormat));

    QTextCharFormat redBackgroundFormat;
    redBackgroundFormat.setBackground(QColor(170, 0, 0));
    QTest::newRow("Background-color change (8 color)")
            << QString("This is " + redBackground + "red" + normal + " text") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("This is ", defaultFormat)
                << FormattedText("red", redBackgroundFormat)
                << FormattedText(" text", defaultFormat));

    QTest::newRow("Text-color change (256 color mode / 8 foreground colors)")
            << QString("This is " + red256 + "red" + normal + " text") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("This is ", defaultFormat)
                << FormattedText("red", redFormat)
                << FormattedText(" text", defaultFormat));

    QTest::newRow("Background-color change (256 color mode / 8 background colors)")
            << QString("This is " + redBackground256 + "red" + normal + " text") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("This is ", defaultFormat)
                << FormattedText("red", redBackgroundFormat)
                << FormattedText(" text", defaultFormat));

    QTextCharFormat mustardFormat;
    mustardFormat.setForeground(QColor(255, 204, 0));
    QTest::newRow("Text-color change (256 color mode / 216 colors)")
            << QString("This is " + mustard256 + "mustard" + normal + " text") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("This is ", defaultFormat)
                << FormattedText("mustard", mustardFormat)
                << FormattedText(" text", defaultFormat));

    QTextCharFormat mustardBackgroundFormat;
    mustardBackgroundFormat.setBackground(QColor(255, 204, 0));
    QTest::newRow("Background-color change (256 color mode / 216 colors)")
            << QString("This is " + mustardBackground256 + "mustard" + normal + " text") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("This is ", defaultFormat)
                << FormattedText("mustard", mustardBackgroundFormat)
                << FormattedText(" text", defaultFormat));

    uint gray = (250 - 232) * 11;
    QTextCharFormat grayFormat;
    grayFormat.setForeground(QColor(gray, gray, gray));
    QTest::newRow("Text-color change (256 color mode / 24 grayscale)")
            << QString("This is " + gray256 + "gray" + normal + " text") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("This is ", defaultFormat)
                << FormattedText("gray", grayFormat)
                << FormattedText(" text", defaultFormat));

    QTextCharFormat grayBackgroundFormat;
    grayBackgroundFormat.setBackground(QColor(gray, gray, gray));
    QTest::newRow("Background-color change (256 color mode / 24 grayscale)")
            << QString("This is " + grayBackground256 + "gray" + normal + " text") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("This is ", defaultFormat)
                << FormattedText("gray", grayBackgroundFormat)
                << FormattedText(" text", defaultFormat));

    QTest::newRow("Text-color change (RGB color mode)")
            << QString("This is " + mustardRgb + "mustard" + normal + " text") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("This is ", defaultFormat)
                << FormattedText("mustard", mustardFormat)
                << FormattedText(" text", defaultFormat));

    QTest::newRow("Background-color change (RGB color mode)")
            << QString("This is " + mustardBackgroundRgb + "mustard" + normal + " text") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("This is ", defaultFormat)
                << FormattedText("mustard", mustardBackgroundFormat)
                << FormattedText(" text", defaultFormat));

    QTextCharFormat boldFormat;
    boldFormat.setFontWeight(QFont::Bold);
    QTest::newRow("Text-format change")
            << QString("A line of " + bold + "bold" + normal + " text") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("A line of ", defaultFormat)
                << FormattedText("bold", boldFormat)
                << FormattedText(" text", defaultFormat));

    QTest::newRow("Alternative reset pattern (QTCREATORBUG-10132)")
            << QString("A line of " + bold + "bold" + normal1 + " text") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("A line of ", defaultFormat)
                << FormattedText("bold", boldFormat)
                << FormattedText(" text", defaultFormat));

    QTest::newRow("Erase to EOL is unsupported and stripped")
            << QString("All text after this is " + eraseToEol + "not deleted") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("All text after this is ", defaultFormat)
                << FormattedText("not deleted", defaultFormat));

    QTest::newRow("Unfinished control sequence \\x1b")
            << QString("A text before \x1b") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("A text before ", defaultFormat));

    QTest::newRow("Unfinished control sequence \\x1b[")
            << QString("A text before \x1b[") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("A text before ", defaultFormat));

    QTest::newRow("Unfinished control sequence \\x1b[3")
            << QString("A text before \x1b[3") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("A text before ", defaultFormat));

    QTest::newRow("Unfinished control sequence \\x1b[31")
            << QString("A text before \x1b[31") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("A text before ", defaultFormat));

    QTest::newRow("Unfinished control sequence \\x1b[31,")
            << QString("A text before \x1b[31,") << QTextCharFormat()
            << (FormattedTextList()
                << FormattedText("A text before ", defaultFormat));
}

void tst_AnsiEscapeCodeHandler::testLineOverlappingFormat()
{
    // Test line-overlapping formats
    const QString line1 = "A line of " + bold + "bold text";
    const QString line2 = "A line of " + normal + "normal text";

    QTextCharFormat defaultFormat;

    AnsiEscapeCodeHandler handler;
    FormattedTextList result;
    result.append(handler.parseText(FormattedText(line1, defaultFormat)));
    result.append(handler.parseText(FormattedText(line2, defaultFormat)));

    QTextCharFormat boldFormat;
    boldFormat.setFontWeight(QFont::Bold);

    QCOMPARE(result.size(), 4);
    QCOMPARE(result[0].text, QLatin1String("A line of "));
    QCOMPARE(result[0].format, defaultFormat);
    QCOMPARE(result[1].text, QLatin1String("bold text"));
    QCOMPARE(result[1].format, boldFormat);
    QCOMPARE(result[2].text, QLatin1String("A line of "));
    QCOMPARE(result[2].format, boldFormat);
    QCOMPARE(result[3].text, QLatin1String("normal text"));
    QCOMPARE(result[3].format, defaultFormat);
}

void tst_AnsiEscapeCodeHandler::testSplitControlSequence()
{
    // Test line-overlapping formats
    const QString line1 = "Normal line \x1b";
    const QString line2 = "[1m bold line";

    QTextCharFormat defaultFormat;

    AnsiEscapeCodeHandler handler;
    FormattedTextList result;
    result.append(handler.parseText(FormattedText(line1, defaultFormat)));
    result.append(handler.parseText(FormattedText(line2, defaultFormat)));

    QTextCharFormat boldFormat;
    boldFormat.setFontWeight(QFont::Bold);

    QCOMPARE(result.size(), 2);
    QCOMPARE(result[0].text, QLatin1String("Normal line "));
    QCOMPARE(result[0].format, defaultFormat);
    QCOMPARE(result[1].text, QLatin1String(" bold line"));
    QCOMPARE(result[1].format, boldFormat);
}

void tst_AnsiEscapeCodeHandler::oneLineBenchmark()
{
    const QString line = QLatin1String("Normal text")
            + mustard256 + QLatin1String(" mustard text ")
            + bold + QLatin1String(" bold text ")
            + grayBackground256 + QLatin1String(" gray background text");
    AnsiEscapeCodeHandler handler;
    QTextCharFormat defaultFormat;

    QBENCHMARK {
        handler.parseText(FormattedText(line, defaultFormat));
    }
}

void tst_AnsiEscapeCodeHandler::multiLineBenchmark()
{
    const QString line1 = QLatin1String("Normal text")
            + mustard256 + QLatin1String(" mustard text ")
            + bold;
    const QString line2 = QLatin1String(" bold text ")
            + grayBackground256 + QLatin1String(" gray background text");
    AnsiEscapeCodeHandler handler;
    QTextCharFormat defaultFormat;

    QBENCHMARK {
        handler.parseText(FormattedText(line1, defaultFormat));
        handler.parseText(FormattedText(line2, defaultFormat));
    }
}

QTEST_APPLESS_MAIN(tst_AnsiEscapeCodeHandler)

#include "tst_ansiescapecodehandler.moc"

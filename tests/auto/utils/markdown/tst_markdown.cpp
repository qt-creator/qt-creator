// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/markdownbrowser.h>

#include <QTest>
#include <QTextDocument>

//TESTED_COMPONENT=src/libs/utils

using namespace Utils;

class tst_Markdown : public QObject
{
    Q_OBJECT

private slots:
    void testEscapeMarkdownHtml_data();
    void testEscapeMarkdownHtml();
    void testRenderedTextKeepsAngleBrackets();
};

void tst_Markdown::testEscapeMarkdownHtml_data()
{
    QTest::addColumn<QString>("markdown");
    QTest::addColumn<QString>("expected");

    QTest::newRow("plain") << "nothing to escape" << "nothing to escape";
    QTest::newRow("template") << "Mixin<A> is a type" << "Mixin&lt;A> is a type";
    QTest::newRow("code span") << "`Mixin<A>` is a type" << "`Mixin<A>` is a type";
    QTest::newRow("mixed") << "`Mixin<A>` and Mixin<A>"
                           << "`Mixin<A>` and Mixin&lt;A>";
    QTest::newRow("double backtick span") << "``a<b`c`` and a<b"
                                          << "``a<b`c`` and a&lt;b";
    QTest::newRow("unclosed backtick") << "`a<b" << "`a&lt;b";
    QTest::newRow("multi line span") << "`a<b\nc<d` and e<f"
                                     << "`a<b\nc<d` and e&lt;f";
    QTest::newRow("span not across blank line") << "`a<b\n\nc<d`"
                                                << "`a&lt;b\n\nc&lt;d`";
    QTest::newRow("span not across setext heading") << "`x\n<b>hi</b>\n===\n`y"
                                                    << "`x\n&lt;b>hi&lt;/b>\n===\n`y";
    QTest::newRow("span not across underscore break") << "`x\n<b>hi</b>\n___\n`y"
                                                      << "`x\n&lt;b>hi&lt;/b>\n___\n`y";
    QTest::newRow("entity reference") << "&lt;b&gt; and R&D"
                                      << "&amp;lt;b&amp;gt; and R&amp;D";
    QTest::newRow("inline triple backtick span") << "```a<b``` and c<d\n\nnext <i>d</i>"
                                                 << "```a<b``` and c&lt;d\n\nnext &lt;i>d&lt;/i>";
    QTest::newRow("fenced block") << "```\na<b\n```\nc<d"
                                  << "```\na<b\n```\nc&lt;d";
    QTest::newRow("tilde fence") << "~~~cpp\na<b\n~~~\nc<d"
                                 << "~~~cpp\na<b\n~~~\nc&lt;d";
    QTest::newRow("unclosed fence") << "```\na<b\n" << "```\na<b\n";
    QTest::newRow("nested fence marker") << "````\n```\na<b\n````\nc<d"
                                         << "````\n```\na<b\n````\nc&lt;d";
    QTest::newRow("autolink") << "see <https://example.com/a?b=c> now"
                              << "see <https://example.com/a?b=c> now";
    // '<' cannot occur inside an autolink, so this is not one.
    QTest::newRow("autolink with bracket") << "see <https://example.com/a<b>"
                                          << "see &lt;https://example.com/a&lt;b>";
    QTest::newRow("mail autolink") << "mail <a@example.com> now"
                                   << "mail <a@example.com> now";
    QTest::newRow("no autolink") << "<not an url> here" << "&lt;not an url> here";
    QTest::newRow("html tag") << "<b>bold</b>" << "&lt;b>bold&lt;/b>";
    QTest::newRow("indented span") << "  * item `a<b` and c<d"
                                   << "  * item `a<b` and c&lt;d";
    QTest::newRow("span not across heading") << "`x\n# <b>hi</b>\n`y"
                                             << "`x\n# &lt;b>hi&lt;/b>\n`y";
    QTest::newRow("span not across list item") << "`x\n- <b>hi</b>\n`y"
                                               << "`x\n- &lt;b>hi&lt;/b>\n`y";
    QTest::newRow("span not across quote") << "`x\n> <b>hi</b>\n`y"
                                           << "`x\n> &lt;b>hi&lt;/b>\n`y";
    QTest::newRow("span not across fence") << "`x\n```\n<b>\n```\n`y"
                                           << "`x\n```\n<b>\n```\n`y";
    QTest::newRow("span across wrapped text") << "`a<b\nstill the same span` c<d"
                                              << "`a<b\nstill the same span` c&lt;d";
    QTest::newRow("crlf span") << "`x\r\n\r\ny<z`\r\n"
                               << "`x\r\n\r\ny&lt;z`\r\n";
    QTest::newRow("crlf fence") << "```\r\na<b\r\n```\r\nc<d"
                                << "```\r\na<b\r\n```\r\nc&lt;d";
    // Four spaces make an indented code block, not a fence.
    QTest::newRow("overindented fence") << "    ```\na<b" << "    ```\na&lt;b";
    QTest::newRow("quoted fence") << "> ```\n> a<b\n> ```\n> c<d"
                                  << "> ```\n> a<b\n> ```\n> c&lt;d";
    QTest::newRow("backslash escaped") << "a\\<b and c<d" << "a\\<b and c&lt;d";
}

void tst_Markdown::testEscapeMarkdownHtml()
{
    QFETCH(QString, markdown);
    QFETCH(QString, expected);

    QCOMPARE(escapeMarkdownHtml(markdown), expected);
}

void tst_Markdown::testRenderedTextKeepsAngleBrackets()
{
    const QString markdown = "**Type hierarchy of Mixin<A>**\n\n"
                             "The `collectSubtypes(A)` call never lists Mixin<A>.\n";

    QTextDocument unescaped;
    unescaped.setMarkdown(markdown);
    QVERIFY(!unescaped.toPlainText().contains("never lists"));

    QTextDocument escaped;
    escaped.setMarkdown(escapeMarkdownHtml(markdown));
    const QString text = escaped.toPlainText();
    QVERIFY(text.contains("Type hierarchy of Mixin<A>"));
    QVERIFY(text.contains("The collectSubtypes(A) call never lists Mixin<A>."));
}

QTEST_MAIN(tst_Markdown)

#include "tst_markdown.moc"

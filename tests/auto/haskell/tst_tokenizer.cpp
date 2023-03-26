/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <haskelltokenizer.h>

#include <QObject>
#include <QtTest>

using namespace Haskell::Internal;

const QSet<char> escapes{'a', 'b', 'f', 'n', 'r', 't', 'v', '\\', '"', '\'', '&'};

struct TokenInfo
{
    TokenType type;
    int column;
    QString text;
};

Q_DECLARE_METATYPE(TokenInfo)

bool operator==(const TokenInfo &info, const Token &token)
{
    return info.type == token.type
            && info.column == token.startCol
            && info.text.length() == token.length
            && info.text == token.text.toString();
}

bool operator==(const Token &token, const TokenInfo &info)
{
    return info == token;
}

class tst_Tokenizer : public QObject
{
    Q_OBJECT

private slots:
    void singleLineComment_data();
    void singleLineComment();

    void multiLineComment_data();
    void multiLineComment();

    void string_data();
    void string();

    void character_data();
    void character();

    void number_data();
    void number();

    void keyword_data();
    void keyword();

    void variable_data();
    void variable();

    void constructor_data();
    void constructor();

    void op_data();
    void op();

private:
    void setupData();
    void addRow(const char *name,
                const QString &input,
                const QList<TokenInfo> &tokens,
                Tokens::State startState = Tokens::State::None,
                Tokens::State endState = Tokens::State::None);
    void checkData();
};

void tst_Tokenizer::setupData()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QList<TokenInfo>>("output");
    QTest::addColumn<int>("startState");
    QTest::addColumn<int>("endState");
}

void tst_Tokenizer::addRow(const char *name,
                           const QString &input,
                           const QList<TokenInfo> &tokens,
                           Tokens::State startState,
                           Tokens::State endState)
{
    QTest::newRow(name) << input << tokens << int(startState) << int(endState);
}

void tst_Tokenizer::checkData()
{
    QFETCH(QString, input);
    QFETCH(QList<TokenInfo>, output);
    QFETCH(int, startState);
    QFETCH(int, endState);
    const Tokens tokens = HaskellTokenizer::tokenize(input, startState);
    QCOMPARE(tokens.length(), output.length());
    QCOMPARE(tokens.state, endState);
    for (int i = 0; i < tokens.length(); ++i) {
        const Token t = tokens.at(i);
        const TokenInfo ti = output.at(i);
        QVERIFY2(t == ti, QString("Token at index %1 does not match, {%2, %3, \"%4\"} != {%5, %6, \"%7\"}")
                 .arg(i)
                 .arg(int(t.type)).arg(t.startCol).arg(t.text.toString())
                 .arg(int(ti.type)).arg(ti.column).arg(ti.text)
                 .toUtf8().constData());
    }
}

void tst_Tokenizer::singleLineComment_data()
{
    setupData();

    addRow("simple", " -- foo", {
               {TokenType::Whitespace, 0, " "},
               {TokenType::SingleLineComment, 1, "-- foo"}
           });
    addRow("dash, id", "--foo", {
               {TokenType::SingleLineComment, 0, "--foo"}
           });
    addRow("dash, space, op", "-- |foo", {
               {TokenType::SingleLineComment, 0, "-- |foo"}
           });
    addRow("multi-dash, space", "---- foo", {
               {TokenType::SingleLineComment, 0, "---- foo"}
           });
    addRow("dash, op", "--| foo", {
               {TokenType::Operator, 0, "--|"},
               {TokenType::Whitespace, 3, " "},
               {TokenType::Variable, 4, "foo"}
           });
    addRow("dash, special", "--(foo", {
               {TokenType::SingleLineComment, 0, "--(foo"}
           });
    addRow("not a qualified varsym", "F.-- foo", {
               {TokenType::Constructor, 0, "F"},
               {TokenType::Operator, 1, "."},
               {TokenType::SingleLineComment, 2, "-- foo"}
           });
}

void tst_Tokenizer::singleLineComment()
{
    checkData();
}

void tst_Tokenizer::multiLineComment_data()
{
    setupData();

    addRow("trailing dashes", "{---foo -}", {
               {TokenType::MultiLineComment, 0, "{---foo -}"}
           });
    addRow("multiline", "{- foo", {
               {TokenType::MultiLineComment, 0, "{- foo"}
           },
           Tokens::State::None,
           Tokens::State(int(Tokens::State::MultiLineCommentGuard) + 1));
    addRow("multiline2", "bar -}", {
               {TokenType::MultiLineComment, 0, "bar -}"}
           },
           Tokens::State(int(Tokens::State::MultiLineCommentGuard) + 1),
           Tokens::State::None);
    addRow("nested", "{- fo{-o", {
               {TokenType::MultiLineComment, 0, "{- fo{-o"}
           },
           Tokens::State::None,
           Tokens::State(int(Tokens::State::MultiLineCommentGuard) + 2));
    addRow("nested2", "bar -}", {
               {TokenType::MultiLineComment, 0, "bar -}"}
           },
           Tokens::State(int(Tokens::State::MultiLineCommentGuard) + 2),
           Tokens::State(int(Tokens::State::MultiLineCommentGuard) + 1));
    addRow("nested3", "bar -}", {
               {TokenType::MultiLineComment, 0, "bar -}"}
           },
           Tokens::State(int(Tokens::State::MultiLineCommentGuard) + 1),
           Tokens::State::None);
}

void tst_Tokenizer::multiLineComment()
{
    checkData();
}

void tst_Tokenizer::string_data()
{
    setupData();

    addRow("simple", "\"foo\"", {
               {TokenType::String, 0, "\"foo\""}
           });

    addRow("unterminated", "\"", {
               {TokenType::StringError, 0, "\""}
           });
    addRow("unterminated2", "\"foo", {
               {TokenType::String, 0, "\"fo"},
               {TokenType::StringError, 3, "o"}
           });
    addRow("unterminated with escape", "\"\\\\", {
               {TokenType::String, 0, "\""},
               {TokenType::EscapeSequence, 1, "\\"},
               {TokenType::StringError, 2, "\\"}
           });

    // gaps
    addRow("gap", "\" \\   \\\"", {
               {TokenType::String, 0, "\" \\   \\\""}
           });
    addRow("gap over endline", "\"foo\\", {
               {TokenType::String, 0, "\"foo\\"}
           },
           Tokens::State::None, Tokens::State::StringGap);
    addRow("gap over endline2", "\\foo\"", {
               {TokenType::String, 0, "\\foo\""}
           },
           Tokens::State::StringGap, Tokens::State::None);
    addRow("gap error", "\"\\ ab \\\"", {
               {TokenType::String, 0, "\"\\ "},
               {TokenType::StringError, 3, "ab"},
               {TokenType::String, 5, " \\\""}
           });
    addRow("gap error with quote", "\"\\ \"", {
               {TokenType::String, 0, "\"\\ "},
               {TokenType::StringError, 3, "\""}
           },
           Tokens::State::None, Tokens::State::StringGap);

    // char escapes (including wrong ones)
    for (char c = '!'; c <= '~'; ++c) {
        // skip uppercase and '^', since these can be part of ascii escapes
        // and 'o' and 'x' since they start octal and hex escapes
        // and digits as part of decimal escape
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '^' || c == 'o' || c == 'x')
            continue;
        const QChar qc(c);
        const QByteArray name = QString("charesc '%1'").arg(qc).toUtf8();
        const QString input = QString("\"\\%1\"").arg(qc);
        if (escapes.contains(c)) {
            addRow(name.constData(), input, {
                       {TokenType::String, 0, "\""},
                       {TokenType::EscapeSequence, 1, QLatin1String("\\") + qc},
                       {TokenType::String, 3, "\""}
                   });
        } else {
            addRow(name.constData(), input, {
                       {TokenType::String, 0, "\"\\"},
                       {TokenType::StringError, 2, qc},
                       {TokenType::String, 3, "\""}
                   });
        }
    }

    addRow("decimal escape", "\"\\1234a\"", {
               {TokenType::String, 0, "\""},
               {TokenType::EscapeSequence, 1, "\\1234"},
               {TokenType::String, 6, "a\""}
           });

    addRow("octal escape", "\"\\o0678a\"", {
               {TokenType::String, 0, "\""},
               {TokenType::EscapeSequence, 1, "\\o067"},
               {TokenType::String, 6, "8a\""}
           });
    addRow("octal escape error", "\"\\o8a\"", {
               {TokenType::String, 0, "\"\\"},
               {TokenType::StringError, 2, "o"},
               {TokenType::String, 3, "8a\""}
           });

    addRow("hexadecimal escape", "\"\\x0678Abg\"", {
               {TokenType::String, 0, "\""},
               {TokenType::EscapeSequence, 1, "\\x0678Ab"},
               {TokenType::String, 9, "g\""}
           });
    addRow("hexadecimal escape error", "\"\\xg\"", {
               {TokenType::String, 0, "\"\\"},
               {TokenType::StringError, 2, "x"},
               {TokenType::String, 3, "g\""}
           });

    // ascii cntrl escapes (including wrong ones)
    for (char c = '!'; c <= '~'; ++c) {
        if (c == '"') // is special because it also ends the string
            continue;
        const QChar qc(c);
        const QByteArray name = QString("ascii cntrl '^%1'").arg(qc).toUtf8();
        const QString input = QString("\"\\^%1\"").arg(qc);
        if ((qc >= 'A' && qc <= 'Z') || qc == '@' || qc == '[' || qc == '\\' || qc == ']'
                || qc == '^' || qc == '_') {
            addRow(name.constData(), input, {
                       {TokenType::String, 0, "\""},
                       {TokenType::EscapeSequence, 1, QLatin1String("\\^") + qc},
                       {TokenType::String, 4, "\""}
                   });
        } else {
            addRow(name.constData(), input, {
                       {TokenType::String, 0, "\"\\"},
                       {TokenType::StringError, 2, "^"},
                       {TokenType::String, 3, QString(qc) + "\""}
                   });
        }
    }

    addRow("ascii escape SOH", "\"\\SOHN\"", {
               {TokenType::String, 0, "\""},
               {TokenType::EscapeSequence, 1, "\\SOH"},
               {TokenType::String, 5, "N\""}
           });
    addRow("ascii escape SO", "\"\\SON\"", {
               {TokenType::String, 0, "\""},
               {TokenType::EscapeSequence, 1, "\\SO"},
               {TokenType::String, 4, "N\""}
           });
    addRow("ascii escape error", "\"\\TON\"", {
               {TokenType::String, 0, "\"\\"},
               {TokenType::StringError, 2, "T"},
               {TokenType::String, 3, "ON\""}
           });
    addRow("ascii escape error 2", "\"\\STO\"", {
               {TokenType::String, 0, "\"\\"},
               {TokenType::StringError, 2, "S"},
               {TokenType::String, 3, "TO\""}
           });
}

void tst_Tokenizer::string()
{
    checkData();
}

void tst_Tokenizer::character_data()
{
    setupData();

    addRow("simple", "'a'", {
               {TokenType::Char, 0, "'a'"}
           });
    addRow("too many", "'abc'", {
               {TokenType::Char, 0, "'a"},
               {TokenType::CharError, 2, "bc"},
               {TokenType::Char, 4, "'"}
           });
    addRow("too few", "''", {
               {TokenType::Char, 0, "'"},
               {TokenType::CharError, 1, "'"}
           });
    addRow("only quote", "'", {
               {TokenType::CharError, 0, "'"}
           });
    addRow("unterminated", "'a", {
               {TokenType::Char, 0, "'"},
               {TokenType::CharError, 1, "a"}
           });
    addRow("unterminated too many", "'abc", {
               {TokenType::Char, 0, "'a"},
               {TokenType::CharError, 2, "bc"}
           });
    addRow("unterminated backslash", "'\\", {
               {TokenType::Char, 0, "'"},
               {TokenType::CharError, 1, "\\"}
           });

    // char escapes (including wrong ones)
    for (char c = '!'; c <= '~'; ++c) {
        // skip uppercase and '^', since these can be part of ascii escapes
        // and 'o' and 'x' since they start octal and hex escapes
        // and digits as part of decimal escape
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '^' || c == 'o' || c == 'x')
            continue;
        const QChar qc(c);
        const QByteArray name = QString("charesc '%1'").arg(qc).toUtf8();
        const QString input = QString("'\\%1'").arg(qc);
        if (c != '&' && escapes.contains(c)) {
            addRow(name.constData(), input, {
                       {TokenType::Char, 0, "'"},
                       {TokenType::EscapeSequence, 1, QLatin1String("\\") + qc},
                       {TokenType::Char, 3, "'"}
                   });
        } else {
            addRow(name.constData(), input, {
                       {TokenType::Char, 0, "'\\"},
                       {TokenType::CharError, 2, qc},
                       {TokenType::Char, 3, "'"}
                   });
        }
    }

    addRow("decimal escape", "'\\1234'", {
               {TokenType::Char, 0, "'"},
               {TokenType::EscapeSequence, 1, "\\1234"},
               {TokenType::Char, 6, "'"}
           });
    addRow("decimal escape too long", "'\\1234a'", {
               {TokenType::Char, 0, "'"},
               {TokenType::EscapeSequence, 1, "\\1234"},
               {TokenType::CharError, 6, "a"},
               {TokenType::Char, 7, "'"}
           });

    addRow("octal escape", "'\\o067'", {
               {TokenType::Char, 0, "'"},
               {TokenType::EscapeSequence, 1, "\\o067"},
               {TokenType::Char, 6, "'"}
           });
    addRow("octal escape error", "'\\o8'", {
               {TokenType::Char, 0, "'\\"},
               {TokenType::CharError, 2, "o"},
               {TokenType::CharError, 3, "8"},
               {TokenType::Char, 4, "'"}
           });

    addRow("hexadecimal escape", "'\\x0678Ab'", {
               {TokenType::Char, 0, "'"},
               {TokenType::EscapeSequence, 1, "\\x0678Ab"},
               {TokenType::Char, 9, "'"}
           });
    addRow("hexadecimal escape error", "'\\xg'", {
               {TokenType::Char, 0, "'\\"},
               {TokenType::CharError, 2, "x"},
               {TokenType::CharError, 3, "g"},
               {TokenType::Char, 4, "'"}
           });

    // ascii cntrl escapes (including wrong ones)
    for (char c = '!'; c <= '~'; ++c) {
        if (c == '\'') // is special because it also ends the string
            continue;
        const QChar qc(c);
        const QByteArray name = QString("ascii cntrl '^%1'").arg(qc).toUtf8();
        const QString input = QString("'\\^%1'").arg(qc);
        if ((qc >= 'A' && qc <= 'Z') || qc == '@' || qc == '[' || qc == '\\' || qc == ']'
                || qc == '^' || qc == '_') {
            addRow(name.constData(), input, {
                       {TokenType::Char, 0, "'"},
                       {TokenType::EscapeSequence, 1, QLatin1String("\\^") + qc},
                       {TokenType::Char, 4, "'"}
                   });
        } else {
            addRow(name.constData(), input, {
                       {TokenType::Char, 0, "'\\"},
                       {TokenType::CharError, 2, "^"},
                       {TokenType::CharError, 3, qc},
                       {TokenType::Char, 4, "'"}
                   });
        }
    }

    addRow("ascii escape SOH", "'\\SOH'", {
               {TokenType::Char, 0, "'"},
               {TokenType::EscapeSequence, 1, "\\SOH"},
               {TokenType::Char, 5, "'"}
           });
    addRow("ascii escape SO, too long", "'\\SON'", {
               {TokenType::Char, 0, "'"},
               {TokenType::EscapeSequence, 1, "\\SO"},
               {TokenType::CharError, 4, "N"},
               {TokenType::Char, 5, "'"}
           });
    addRow("ascii escape error", "'\\TON'", {
               {TokenType::Char, 0, "'\\"},
               {TokenType::CharError, 2, "T"},
               {TokenType::CharError, 3, "ON"},
               {TokenType::Char, 5, "'"}
           });
}

void tst_Tokenizer::character()
{
    checkData();
}

void tst_Tokenizer::number_data()
{
    setupData();

    addRow("decimal", "012345", {
               {TokenType::Integer, 0, "012345"}
           });
    addRow("single digit decimal", "0", {
               {TokenType::Integer, 0, "0"}
           });
    addRow("octal", "0o1234", {
               {TokenType::Integer, 0, "0o1234"}
           });
    // this is a bit weird, but correct: octal 1 followed by decimal 8
    addRow("number after octal", "0O18", {
               {TokenType::Integer, 0, "0O1"},
               {TokenType::Integer, 3, "8"}
           });
    addRow("not octal", "0o9", {
               {TokenType::Integer, 0, "0"},
               {TokenType::Variable, 1, "o9"},
           });
    addRow("hexadecimal", "0x9fA", {
               {TokenType::Integer, 0, "0x9fA"}
           });
    // hex number followed by identifier 'g'
    addRow("hexadecimal", "0X9fg", {
               {TokenType::Integer, 0, "0X9f"},
               {TokenType::Variable, 4, "g"}
           });

    // 0 followed by identifier
    addRow("decimal followed by identifier", "0z6", {
               {TokenType::Integer, 0, "0"},
               {TokenType::Variable, 1, "z6"}
           });

    addRow("float", "0123.45", {
               {TokenType::Float, 0, "0123.45"}
           });
    addRow("decimal + operator '.'", "0123.", {
               {TokenType::Integer, 0, "0123"},
               {TokenType::Operator, 4, "."}
           });
    addRow("operator '.' + decimal", ".0123", {
               {TokenType::Operator, 0, "."},
               {TokenType::Integer, 1, "0123"}
           });
    addRow("without '.', with exp 'e'", "0123e45", {
               {TokenType::Float, 0, "0123e45"}
           });
    addRow("without '.', with exp 'E'", "0123E45", {
               {TokenType::Float, 0, "0123E45"}
           });
    addRow("without '.', with '+'", "0123e+45", {
               {TokenType::Float, 0, "0123e+45"}
           });
    addRow("without '.', with '-'", "0123e-45", {
               {TokenType::Float, 0, "0123e-45"}
           });
    addRow("without '.', with '+', missing decimal", "0123e+", {
               {TokenType::Integer, 0, "0123"},
               {TokenType::Variable, 4, "e"},
               {TokenType::Operator, 5, "+"}
           });
    addRow("without '.', missing decimal", "0123e", {
               {TokenType::Integer, 0, "0123"},
               {TokenType::Variable, 4, "e"}
           });
    addRow("exp 'e'", "01.23e45", {
               {TokenType::Float, 0, "01.23e45"}
           });
    addRow("exp 'E'", "01.23E45", {
               {TokenType::Float, 0, "01.23E45"}
           });
    addRow("with '+'", "01.23e+45", {
               {TokenType::Float, 0, "01.23e+45"}
           });
    addRow("with '-'", "01.23e-45", {
               {TokenType::Float, 0, "01.23e-45"}
           });
    addRow("with '+', missing decimal", "01.23e+", {
               {TokenType::Float, 0, "01.23"},
               {TokenType::Variable, 5, "e"},
               {TokenType::Operator, 6, "+"}
           });
    addRow("missing decimal", "01.23e", {
               {TokenType::Float, 0, "01.23"},
               {TokenType::Variable, 5, "e"}
           });
}

void tst_Tokenizer::number()
{
    checkData();
}

void tst_Tokenizer::keyword_data()
{
    setupData();

    addRow("data", "data", {
               {TokenType::Keyword, 0, "data"}
           });
    addRow("not a qualified varid", "Foo.case", {
               {TokenType::Constructor, 0, "Foo"},
               {TokenType::Operator, 3, "."},
               {TokenType::Keyword, 4, "case"}
           });
    addRow(":", ":", {
               {TokenType::Keyword, 0, ":"}
           });
    addRow("->", "->", {
               {TokenType::Keyword, 0, "->"}
           });
    addRow("not a qualified varsym", "Foo...", {
               {TokenType::Constructor, 0, "Foo"},
               {TokenType::Operator, 3, "..."}
           });
}

void tst_Tokenizer::keyword()
{
    checkData();
}

void tst_Tokenizer::variable_data()
{
    setupData();

    addRow("simple", "fOo_1'", {
               {TokenType::Variable, 0, "fOo_1'"}
           });
    addRow("start with '_'", "_1", {
               {TokenType::Variable, 0, "_1"}
           });
    addRow("not a keyword", "cases", {
               {TokenType::Variable, 0, "cases"}
           });
    addRow("not a keyword 2", "qualified", {
               {TokenType::Variable, 0, "qualified"}
           });
    addRow("not a keyword 3", "as", {
               {TokenType::Variable, 0, "as"}
           });
    addRow("not a keyword 4", "hiding", {
               {TokenType::Variable, 0, "hiding"}
           });
    addRow(".variable", ".foo", {
               {TokenType::Operator, 0, "."},
               {TokenType::Variable, 1, "foo"}
           });
    addRow("variable.", "foo.", {
               {TokenType::Variable, 0, "foo"},
               {TokenType::Operator, 3, "."}
           });
    addRow("variable.variable", "blah.foo", {
               {TokenType::Variable, 0, "blah"},
               {TokenType::Operator, 4, "."},
               {TokenType::Variable, 5, "foo"}
           });
    addRow("qualified", "Blah.foo", {
               {TokenType::Variable, 0, "Blah.foo"}
           });
    addRow("qualified2", "Goo.Blah.foo", {
               {TokenType::Variable, 0, "Goo.Blah.foo"}
           });
    addRow("variable + op '..'", "foo..", {
               {TokenType::Variable, 0, "foo"},
               {TokenType::Keyword, 3, ".."}
           });
    addRow("variable + op '...'", "foo...", {
               {TokenType::Variable, 0, "foo"},
               {TokenType::Operator, 3, "..."}
           });
}

void tst_Tokenizer::variable()
{
    checkData();
}

void tst_Tokenizer::constructor_data()
{
    setupData();

    addRow("simple", "Foo", {
               {TokenType::Constructor, 0, "Foo"}
           });
    addRow("qualified", "Foo.Bar", {
               {TokenType::Constructor, 0, "Foo.Bar"}
           });
    addRow("followed by op '.'", "Foo.Bar.", {
               {TokenType::Constructor, 0, "Foo.Bar"},
               {TokenType::Operator, 7, "."}
           });

}

void tst_Tokenizer::constructor()
{
    checkData();
}

void tst_Tokenizer::op_data()
{
    setupData();

    addRow("simple", "+-=", {
               {TokenType::Operator, 0, "+-="}
           });
    addRow("qualified", "Foo.+-=", {
               {TokenType::Operator, 0, "Foo.+-="}
           });
    addRow("qualified '.'", "Foo..", {
               {TokenType::Operator, 0, "Foo.."}
           });
    addRow("constructor plus op", "Foo+", {
               {TokenType::Constructor, 0, "Foo"},
               {TokenType::Operator, 3, "+"}
           });
}

void tst_Tokenizer::op()
{
    checkData();
}

QTEST_MAIN(tst_Tokenizer)

#include "tst_tokenizer.moc"

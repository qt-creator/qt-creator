// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpphighlighter.h"

#include "cppdoxygen.h"
#include "cpptoolsreuse.h"

#include <texteditor/textdocumentlayout.h>
#include <utils/textutils.h>

#include <cplusplus/SimpleLexer.h>
#include <cplusplus/Lexer.h>

#include <QFile>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextLayout>

#ifdef WITH_TESTS
#include <QtTest>
#endif

using namespace TextEditor;
using namespace CPlusPlus;

namespace CppEditor {

CppHighlighter::CppHighlighter(QTextDocument *document) :
    SyntaxHighlighter(document)
{
    setDefaultTextFormatCategories();
}

void CppHighlighter::highlightBlock(const QString &text)
{
    const int previousBlockState_ = previousBlockState();
    int lexerState = 0, initialBraceDepth = 0;
    if (previousBlockState_ != -1) {
        lexerState = previousBlockState_ & 0xff;
        initialBraceDepth = previousBlockState_ >> 8;
    }

    int braceDepth = initialBraceDepth;

    SimpleLexer tokenize;
    tokenize.setLanguageFeatures(m_languageFeatures);
    const QTextBlock prevBlock = currentBlock().previous();
    QByteArray inheritedRawStringSuffix;
    if (prevBlock.isValid()) {
        inheritedRawStringSuffix = TextDocumentLayout::expectedRawStringSuffix(prevBlock);
        tokenize.setExpectedRawStringSuffix(inheritedRawStringSuffix);
    }

    int initialLexerState = lexerState;
    const Tokens tokens = tokenize(text, initialLexerState);
    lexerState = tokenize.state(); // refresh lexer state

    static const auto lexerStateWithoutNewLineExpectedBit = [](int state) { return state & ~0x80; };
    initialLexerState = lexerStateWithoutNewLineExpectedBit(initialLexerState);
    int foldingIndent = initialBraceDepth;
    if (TextBlockUserData *userData = TextDocumentLayout::textUserData(currentBlock())) {
        userData->setFoldingIndent(0);
        userData->setFoldingStartIncluded(false);
        userData->setFoldingEndIncluded(false);
    }

    if (tokens.isEmpty()) {
        setCurrentBlockState((braceDepth << 8) | lexerState);
        TextDocumentLayout::clearParentheses(currentBlock());
        if (!text.isEmpty())  {// the empty line can still contain whitespace
            if (initialLexerState == T_COMMENT)
                setFormatWithSpaces(text, 0, text.length(), formatForCategory(C_COMMENT));
            else if (initialLexerState == T_DOXY_COMMENT)
                setFormatWithSpaces(text, 0, text.length(), formatForCategory(C_DOXYGEN_COMMENT));
            else
                setFormat(0, text.length(), formatForCategory(C_VISUAL_WHITESPACE));
        }
        TextDocumentLayout::setFoldingIndent(currentBlock(), foldingIndent);
        TextDocumentLayout::setExpectedRawStringSuffix(currentBlock(), inheritedRawStringSuffix);
        return;
    }

    const int firstNonSpace = tokens.first().utf16charsBegin();

    // Keep "semantic parentheses".
    Parentheses parentheses = Utils::filtered(TextDocumentLayout::parentheses(currentBlock()),
            [](const Parenthesis &p) { return p.source.isValid(); });
    const auto insertParen = [&parentheses](const Parenthesis &p) { insertSorted(parentheses, p); };
    parentheses.reserve(5);

    bool expectPreprocessorKeyword = false;
    bool onlyHighlightComments = false;

    for (int i = 0; i < tokens.size(); ++i) {
        const Token &tk = tokens.at(i);

        int previousTokenEnd = 0;
        if (i != 0) {
            inheritedRawStringSuffix.clear();

            // mark the whitespaces
            previousTokenEnd = tokens.at(i - 1).utf16charsBegin() +
                               tokens.at(i - 1).utf16chars();
        }

        if (previousTokenEnd != tk.utf16charsBegin()) {
            setFormat(previousTokenEnd,
                      tk.utf16charsBegin() - previousTokenEnd,
                      formatForCategory(C_VISUAL_WHITESPACE));
        }

        if (tk.is(T_LPAREN) || tk.is(T_LBRACE) || tk.is(T_LBRACKET)) {
            const QChar c = text.at(tk.utf16charsBegin());
            insertParen({Parenthesis::Opened, c, tk.utf16charsBegin()});
            if (tk.is(T_LBRACE)) {
                ++braceDepth;

                // if a folding block opens at the beginning of a line, treat the entire line
                // as if it were inside the folding block
                if (tk.utf16charsBegin() == firstNonSpace) {
                    ++foldingIndent;
                    TextDocumentLayout::userData(currentBlock())->setFoldingStartIncluded(true);
                }
            }
        } else if (tk.is(T_RPAREN) || tk.is(T_RBRACE) || tk.is(T_RBRACKET)) {
            const QChar c = text.at(tk.utf16charsBegin());
            insertParen({Parenthesis::Closed, c, tk.utf16charsBegin()});
            if (tk.is(T_RBRACE)) {
                --braceDepth;
                if (braceDepth < foldingIndent) {
                    // unless we are at the end of the block, we reduce the folding indent
                    if (i == tokens.size()-1 || tokens.at(i+1).is(T_SEMICOLON))
                        TextDocumentLayout::userData(currentBlock())->setFoldingEndIncluded(true);
                    else
                        foldingIndent = qMin(braceDepth, foldingIndent);
                }
            }
        }

        bool highlightCurrentWordAsPreprocessor = expectPreprocessorKeyword;

        if (expectPreprocessorKeyword)
            expectPreprocessorKeyword = false;

        if (onlyHighlightComments && !tk.isComment())
            continue;

        if (i == 0 && tk.is(T_POUND)) {
            setFormatWithSpaces(text, tk.utf16charsBegin(), tk.utf16chars(),
                          formatForCategory(C_PREPROCESSOR));
            expectPreprocessorKeyword = true;
        } else if (highlightCurrentWordAsPreprocessor && (tk.isKeyword() || tk.is(T_IDENTIFIER))
                   && isPPKeyword(QStringView(text).mid(tk.utf16charsBegin(), tk.utf16chars()))) {
            setFormat(tk.utf16charsBegin(), tk.utf16chars(), formatForCategory(C_PREPROCESSOR));
            const QStringView ppKeyword = QStringView(text).mid(tk.utf16charsBegin(), tk.utf16chars());
            if (ppKeyword == QLatin1String("error")
                    || ppKeyword == QLatin1String("warning")
                    || ppKeyword == QLatin1String("pragma")) {
                onlyHighlightComments = true;
            }

        } else if (tk.is(T_NUMERIC_LITERAL)) {
            setFormat(tk.utf16charsBegin(), tk.utf16chars(), formatForCategory(C_NUMBER));
        } else if (tk.isStringLiteral() || tk.isCharLiteral()) {
            if (!highlightRawStringLiteral(text, tk, QString::fromUtf8(inheritedRawStringSuffix)))
                highlightStringLiteral(text, tk);
        } else if (tk.isComment()) {
            const int startPosition = initialLexerState ? previousTokenEnd : tk.utf16charsBegin();
            if (tk.is(T_COMMENT) || tk.is(T_CPP_COMMENT)) {
                setFormatWithSpaces(text, startPosition, tk.utf16charsEnd() - startPosition,
                              formatForCategory(C_COMMENT));
            }

            else // a doxygen comment
                highlightDoxygenComment(text, startPosition, tk.utf16charsEnd() - startPosition);

            // we need to insert a close comment parenthesis, if
            //  - the line starts in a C Comment (initalState != 0)
            //  - the first token of the line is a T_COMMENT (i == 0 && tk.is(T_COMMENT))
            //  - is not a continuation line (tokens.size() > 1 || !state)
            if (initialLexerState && i == 0 && (tk.is(T_COMMENT) || tk.is(T_DOXY_COMMENT))
                && (tokens.size() > 1 || !lexerState)) {
                --braceDepth;
                // unless we are at the end of the block, we reduce the folding indent
                if (i == tokens.size()-1)
                    TextDocumentLayout::userData(currentBlock())->setFoldingEndIncluded(true);
                else
                    foldingIndent = qMin(braceDepth, foldingIndent);
                const int tokenEnd = tk.utf16charsBegin() + tk.utf16chars() - 1;
                insertParen({Parenthesis::Closed, QLatin1Char('-'), tokenEnd});

                // clear the initial state.
                initialLexerState = 0;
            }

        } else if (tk.isKeyword()
                   || (m_languageFeatures.qtKeywordsEnabled
                       && isQtKeyword(
                           QStringView{text}.mid(tk.utf16charsBegin(), tk.utf16chars())))
                   || (m_languageFeatures.objCEnabled && tk.isObjCAtKeyword())) {
            setFormat(tk.utf16charsBegin(), tk.utf16chars(), formatForCategory(C_KEYWORD));
        } else if (tk.isPrimitiveType()) {
            setFormat(tk.utf16charsBegin(), tk.utf16chars(),
                      formatForCategory(C_PRIMITIVE_TYPE));
        } else if (tk.isOperator()) {
            setFormat(tk.utf16charsBegin(), tk.utf16chars(), formatForCategory(C_OPERATOR));
        } else if (tk.isPunctuation()) {
            setFormat(tk.utf16charsBegin(), tk.utf16chars(), formatForCategory(C_PUNCTUATION));
        } else if (i == 0 && tokens.size() > 1 && tk.is(T_IDENTIFIER) && tokens.at(1).is(T_COLON)) {
            setFormat(tk.utf16charsBegin(), tk.utf16chars(), formatForCategory(C_LABEL));
        } else if (tk.is(T_IDENTIFIER)) {
            highlightWord(QStringView(text).mid(tk.utf16charsBegin(), tk.utf16chars()),
                          tk.utf16charsBegin(),
                          tk.utf16chars());
        }
    }

    // mark the trailing white spaces
    const int lastTokenEnd = tokens.last().utf16charsEnd();
    if (text.length() > lastTokenEnd)
        formatSpaces(text, lastTokenEnd, text.length() - lastTokenEnd);

    if (!initialLexerState && lexerStateWithoutNewLineExpectedBit(lexerState)
        && !tokens.isEmpty()) {
        const Token &lastToken = tokens.last();
        if (lastToken.is(T_COMMENT) || lastToken.is(T_DOXY_COMMENT)) {
            insertParen({Parenthesis::Opened, QLatin1Char('+'), lastToken.utf16charsBegin()});
            ++braceDepth;
        }
    }

    TextDocumentLayout::setParentheses(currentBlock(), parentheses);

    // if the block is ifdefed out, we only store the parentheses, but

    // do not adjust the brace depth.
    if (TextDocumentLayout::ifdefedOut(currentBlock())) {
        braceDepth = initialBraceDepth;
        foldingIndent = initialBraceDepth;
    }

    TextDocumentLayout::setFoldingIndent(currentBlock(), foldingIndent);

    // optimization: if only the brace depth changes, we adjust subsequent blocks
    // to have QSyntaxHighlighter stop the rehighlighting
    int currentState = currentBlockState();
    if (currentState != -1) {
        int oldState = currentState & 0xff;
        int oldBraceDepth = currentState >> 8;
        if (oldState == tokenize.state() && oldBraceDepth != braceDepth) {
            TextDocumentLayout::FoldValidator foldValidor;
            foldValidor.setup(qobject_cast<TextDocumentLayout *>(document()->documentLayout()));
            int delta = braceDepth - oldBraceDepth;
            QTextBlock block = currentBlock().next();
            while (block.isValid() && block.userState() != -1) {
                TextDocumentLayout::changeBraceDepth(block, delta);
                TextDocumentLayout::changeFoldingIndent(block, delta);
                foldValidor.process(block);
                block = block.next();
            }
            foldValidor.finalize();
        }
    }

    setCurrentBlockState((braceDepth << 8) | tokenize.state());
    TextDocumentLayout::setExpectedRawStringSuffix(currentBlock(),
                                                   tokenize.expectedRawStringSuffix());
}

void CppHighlighter::setLanguageFeatures(const LanguageFeatures &languageFeatures)
{
    if (languageFeatures != m_languageFeatures) {
        m_languageFeatures = languageFeatures;
        rehighlight();
    }
}

bool CppHighlighter::isPPKeyword(QStringView text) const
{
    switch (text.length())
    {
    case 2:
        if (text.at(0) == QLatin1Char('i') && text.at(1) == QLatin1Char('f'))
            return true;
        break;

    case 4:
        if (text.at(0) == QLatin1Char('e')
            && (text == QLatin1String("elif") || text == QLatin1String("else")))
            return true;
        break;

    case 5:
        switch (text.at(0).toLatin1()) {
        case 'i':
            if (text == QLatin1String("ifdef"))
                return true;
            break;
          case 'u':
            if (text == QLatin1String("undef"))
                return true;
            break;
        case 'e':
            if (text == QLatin1String("endif") || text == QLatin1String("error"))
                return true;
            break;
        }
        break;

    case 6:
        switch (text.at(0).toLatin1()) {
        case 'i':
            if (text == QLatin1String("ifndef") || text == QLatin1String("import"))
                return true;
            break;
        case 'd':
            if (text == QLatin1String("define"))
                return true;
            break;
        case 'p':
            if (text == QLatin1String("pragma"))
                return true;
            break;
        }
        break;

    case 7:
        switch (text.at(0).toLatin1()) {
        case 'i':
            if (text == QLatin1String("include"))
                return true;
            break;
        case 'w':
            if (text == QLatin1String("warning"))
                return true;
            break;
        }
        break;

    case 12:
        if (text.at(0) == QLatin1Char('i') && text == QLatin1String("include_next"))
            return true;
        break;

    default:
        break;
    }

    return false;
}

void CppHighlighter::highlightWord(QStringView word, int position, int length)
{
    // try to highlight Qt 'identifiers' like QObject and Q_PROPERTY

    if (word.length() > 2 && word.at(0) == QLatin1Char('Q')) {
        if (word.at(1) == QLatin1Char('_') // Q_
            || (word.at(1) == QLatin1Char('T') && word.at(2) == QLatin1Char('_'))) { // QT_
            for (int i = 1; i < word.length(); ++i) {
                const QChar &ch = word.at(i);
                if (!(ch.isUpper() || ch == QLatin1Char('_')))
                    return;
            }

            setFormat(position, length, formatForCategory(C_TYPE));
        }
    }
}

bool CppHighlighter::highlightRawStringLiteral(QStringView text, const Token &tk,
                                               const QString &inheritedSuffix)
{
    // Step one: Does the lexer think this is a raw string literal?
    switch (tk.kind()) {
    case T_RAW_STRING_LITERAL:
    case T_RAW_WIDE_STRING_LITERAL:
    case T_RAW_UTF8_STRING_LITERAL:
    case T_RAW_UTF16_STRING_LITERAL:
    case T_RAW_UTF32_STRING_LITERAL:
        break;
    default:
        return false;
    }

    // Step two: Try to find all the components (prefix/string/suffix). We might be in the middle
    //           of a multi-line literal, though, so prefix and/or suffix might be missing.
    int delimiterOffset = -1;
    int stringOffset = 0;
    int stringLength = tk.utf16chars();
    int endDelimiterOffset = -1;
    QString expectedSuffix = inheritedSuffix;
    [&] {
        // If the "inherited" suffix is not empty, then this token is a string continuation and
        // can therefore not start a new raw string literal.
        // FIXME: The lexer starts the token at the first non-whitespace character, so
        //        we have to correct for that here.
        if (!inheritedSuffix.isEmpty()) {
            stringLength += tk.utf16charOffset;
            return;
        }

        // Conversely, since we are in a raw string literal that is not a continuation,
        // the start sequence must be in here.
        const int rOffset = text.indexOf(QLatin1String("R\""), tk.utf16charsBegin());
        QTC_ASSERT(rOffset != -1, return);
        const int tentativeDelimiterOffset = rOffset + 2;
        const int openParenOffset = text.indexOf('(', tentativeDelimiterOffset);
        QTC_ASSERT(openParenOffset != -1, return);
        const QStringView delimiter = text.mid(tentativeDelimiterOffset,
                                               openParenOffset - tentativeDelimiterOffset);
        expectedSuffix = ')' + delimiter + '"';
        delimiterOffset = tentativeDelimiterOffset;
        stringOffset = delimiterOffset + delimiter.length() + 1;
        stringLength -= delimiter.length() + 1;
    }();
    int operatorOffset = tk.utf16charsBegin() + tk.utf16chars();
    int operatorLength = 0;
    if (tk.f.userDefinedLiteral) {
        const int closingQuoteOffset = text.lastIndexOf('"', operatorOffset);
        QTC_ASSERT(closingQuoteOffset >= tk.utf16charsBegin(), return false);
        operatorOffset = closingQuoteOffset + 1;
        operatorLength = tk.utf16charsBegin() + tk.utf16chars() - operatorOffset;
        stringLength -= operatorLength;
    }
    if (text.mid(tk.utf16charsBegin(), operatorOffset - tk.utf16charsBegin())
            .endsWith(expectedSuffix)) {
        endDelimiterOffset = operatorOffset - expectedSuffix.size();
        stringLength -= expectedSuffix.size();
    }

    // Step three: Do the actual formatting. For clarity, we display only the actual content as
    //             a string, and the rest (including the delimiter) as a keyword.
    const QTextCharFormat delimiterFormat = formatForCategory(C_KEYWORD);
    if (delimiterOffset != -1)
        setFormat(tk.utf16charsBegin(), stringOffset - tk.utf16charsBegin(), delimiterFormat);
    setFormatWithSpaces(text.toString(), stringOffset, stringLength, formatForCategory(C_STRING));
    if (endDelimiterOffset != -1)
        setFormat(endDelimiterOffset, expectedSuffix.size(), delimiterFormat);
    if (operatorLength > 0)
        setFormat(operatorOffset, operatorLength, formatForCategory(C_OPERATOR));
    return true;
}

void CppHighlighter::highlightStringLiteral(QStringView text, const CPlusPlus::Token &tk)
{
    switch (tk.kind()) {
    case T_WIDE_STRING_LITERAL:
    case T_UTF8_STRING_LITERAL:
    case T_UTF16_STRING_LITERAL:
    case T_UTF32_STRING_LITERAL:
        break;
    default:
        if (!tk.userDefinedLiteral()) { // Simple case: No prefix, no suffix.
            setFormatWithSpaces(text.toString(), tk.utf16charsBegin(), tk.utf16chars(),
                                formatForCategory(C_STRING));
            return;
        }
    }

    int stringOffset = 0;
    if (!tk.f.joined) {
        stringOffset = text.indexOf('"', tk.utf16charsBegin());
        QTC_ASSERT(stringOffset > 0, return);
        setFormat(tk.utf16charsBegin(), stringOffset - tk.utf16charsBegin(),
                  formatForCategory(C_KEYWORD));
    }
    int operatorOffset = tk.utf16charsBegin() + tk.utf16chars();
    if (tk.userDefinedLiteral()) {
        const int closingQuoteOffset = text.lastIndexOf('"', operatorOffset);
        QTC_ASSERT(closingQuoteOffset >= tk.utf16charsBegin(), return);
        operatorOffset = closingQuoteOffset + 1;
    }
    setFormatWithSpaces(text.toString(), stringOffset, operatorOffset - tk.utf16charsBegin(),
                        formatForCategory(C_STRING));
    if (const int operatorLength = tk.utf16charsBegin() + tk.utf16chars() - operatorOffset;
        operatorLength > 0) {
        setFormat(operatorOffset, operatorLength, formatForCategory(C_OPERATOR));
    }
}

void CppHighlighter::highlightDoxygenComment(const QString &text, int position, int)
{
    int initial = position;

    const QChar *uc = text.unicode();
    const QChar *it = uc + position;

    const QTextCharFormat &format = formatForCategory(C_DOXYGEN_COMMENT);
    const QTextCharFormat &kwFormat = formatForCategory(C_DOXYGEN_TAG);

    while (!it->isNull()) {
        if (it->unicode() == QLatin1Char('\\') ||
            it->unicode() == QLatin1Char('@')) {
            ++it;

            const QChar *start = it;
            while (isValidAsciiIdentifierChar(*it))
                ++it;

            int k = classifyDoxygenTag(start, it - start);
            if (k != T_DOXY_IDENTIFIER) {
                setFormatWithSpaces(text, initial, start - uc - initial, format);
                setFormat(start - uc - 1, it - start + 1, kwFormat);
                initial = it - uc;
            }
        } else
            ++it;
    }

    setFormatWithSpaces(text, initial, it - uc - initial, format);
}

#ifdef WITH_TESTS
namespace Internal {
CppHighlighterTest::CppHighlighterTest()
{
    QFile source(":/cppeditor/testcases/highlightingtestcase.cpp");
    QVERIFY(source.open(QIODevice::ReadOnly));

    m_doc.setPlainText(QString::fromUtf8(source.readAll()));
    setDocument(&m_doc);
    rehighlight();
}

void CppHighlighterTest::test_data()
{
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("column");
    QTest::addColumn<int>("lastLine");
    QTest::addColumn<int>("lastColumn");
    QTest::addColumn<TextStyle>("style");

    QTest::newRow("auto return type") << 1 << 1 << 1 << 4 << C_KEYWORD;
    QTest::newRow("opening brace") << 2 << 1 << 2 << 1 << C_PUNCTUATION;
    QTest::newRow("return") << 3 << 5 << 3 << 10 << C_KEYWORD;
    QTest::newRow("raw string prefix") << 3 << 12 << 3 << 14 << C_KEYWORD;
    QTest::newRow("raw string content (multi-line)") << 3 << 15 << 6 << 13 << C_STRING;
    QTest::newRow("raw string suffix") << 6 << 14 << 6 << 15 << C_KEYWORD;
    QTest::newRow("raw string prefix 2") << 6 << 17 << 6 << 19 << C_KEYWORD;
    QTest::newRow("raw string content 2") << 6 << 20 << 6 << 25 << C_STRING;
    QTest::newRow("raw string suffix 2") << 6 << 26 << 6 << 27 << C_KEYWORD;
    QTest::newRow("comment") << 6 << 29 << 6 << 41 << C_COMMENT;
    QTest::newRow("raw string prefix 3") << 6 << 53 << 6 << 45 << C_KEYWORD;
    QTest::newRow("raw string content 3") << 6 << 46 << 6 << 50 << C_STRING;
    QTest::newRow("raw string suffix 3") << 6 << 51 << 6 << 52 << C_KEYWORD;
    QTest::newRow("semicolon") << 6 << 53 << 6 << 53 << C_PUNCTUATION;
    QTest::newRow("closing brace") << 7 << 1 << 7 << 1 << C_PUNCTUATION;
    QTest::newRow("void") << 9 << 1 << 9 << 4 << C_PRIMITIVE_TYPE;
    QTest::newRow("bool") << 11 << 5 << 11 << 8 << C_PRIMITIVE_TYPE;
    QTest::newRow("true") << 11 << 15 << 11 << 18 << C_KEYWORD;
    QTest::newRow("false") << 12 << 15 << 12 << 19 << C_KEYWORD;
    QTest::newRow("nullptr") << 13 << 15 << 13 << 21 << C_KEYWORD;
    QTest::newRow("auto var type") << 18 << 15 << 18 << 8 << C_KEYWORD;
    QTest::newRow("integer literal") << 18 << 28 << 18 << 28 << C_NUMBER;
    QTest::newRow("floating-point literal 1") << 19 << 28 << 19 << 31 << C_NUMBER;
    QTest::newRow("floating-point literal 2") << 20 << 28 << 20 << 30 << C_NUMBER;
    QTest::newRow("template keyword") << 23 << 1 << 23 << 8 << C_KEYWORD;
    QTest::newRow("type in template type parameter") << 23 << 10 << 23 << 12 << C_PRIMITIVE_TYPE;
    QTest::newRow("integer literal as non-type template parameter default value")
        << 23 << 18 << 23 << 18 << C_NUMBER;
    QTest::newRow("class keyword") << 23 << 21 << 23 << 25 << C_KEYWORD;
    QTest::newRow("struct keyword") << 25 << 1 << 25 << 6 << C_KEYWORD;
    QTest::newRow("operator keyword") << 26 << 5 << 26 << 12 << C_KEYWORD;
    QTest::newRow("type in conversion operator") << 26 << 14 << 26 << 16 << C_PRIMITIVE_TYPE;
    QTest::newRow("concept keyword") << 29 << 22 << 29 << 28 << C_KEYWORD;
    QTest::newRow("user-defined UTF-16 string literal (prefix)")
        << 32 << 16 << 32 << 16 << C_KEYWORD;
    QTest::newRow("user-defined UTF-16 string literal (content)")
        << 32 << 17 << 32 << 21 << C_STRING;
    QTest::newRow("user-defined UTF-16 string literal (suffix)")
        << 32 << 22 << 32 << 23 << C_OPERATOR;
    QTest::newRow("wide string literal (prefix)") << 33 << 17 << 33 << 17 << C_KEYWORD;
    QTest::newRow("wide string literal (content)") << 33 << 18 << 33 << 24 << C_STRING;
    QTest::newRow("UTF-8 string literal (prefix)") << 34 << 17 << 34 << 18 << C_KEYWORD;
    QTest::newRow("UTF-8 string literal (content)") << 34 << 19 << 34 << 24 << C_STRING;
    QTest::newRow("UTF-32 string literal (prefix)") << 35 << 17 << 35 << 17 << C_KEYWORD;
    QTest::newRow("UTF-8 string literal (content)") << 35 << 18 << 35 << 23 << C_STRING;
    QTest::newRow("user-defined UTF-16 raw string literal (prefix)")
        << 36 << 17 << 36 << 20 << C_KEYWORD;
    QTest::newRow("user-defined UTF-16 raw string literal (content)")
        << 36 << 38 << 37 << 8 << C_STRING;
    QTest::newRow("user-defined UTF-16 raw string literal (suffix 1)")
        << 37 << 9 << 37 << 10 << C_KEYWORD;
    QTest::newRow("user-defined UTF-16 raw string literal (suffix 2)")
        << 37 << 11 << 37 << 12 << C_OPERATOR;
    QTest::newRow("multi-line user-defined UTF-16 string literal (prefix)")
        << 38 << 17 << 38 << 17 << C_KEYWORD;
    QTest::newRow("multi-line user-defined UTF-16 string literal (content)")
        << 38 << 18 << 39 << 3 << C_STRING;
    QTest::newRow("multi-line user-defined UTF-16 string literal (suffix)")
        << 39 << 4 << 39 << 5 << C_OPERATOR;
    QTest::newRow("multi-line raw string literal with consecutive closing parens (prefix)")
        << 48 << 18 << 48 << 20 << C_KEYWORD;
    QTest::newRow("multi-line raw string literal with consecutive closing parens (content)")
        << 49 << 1 << 49 << 1 << C_STRING;
    QTest::newRow("multi-line raw string literal with consecutive closing parens (suffix)")
        << 49 << 2 << 49 << 3 << C_KEYWORD;
}

void CppHighlighterTest::test()
{
    QFETCH(int, line);
    QFETCH(int, column);
    QFETCH(int, lastLine);
    QFETCH(int, lastColumn);
    QFETCH(TextStyle, style);

    const int startPos = Utils::Text::positionInText(&m_doc, line, column);
    const int lastPos = Utils::Text::positionInText(&m_doc, lastLine, lastColumn);
    const auto getActualFormat = [&](int pos) -> QTextCharFormat {
        const QTextBlock block = m_doc.findBlock(pos);
        if (!block.isValid())
            return {};
        const QList<QTextLayout::FormatRange> &ranges = block.layout()->formats();
        for (const QTextLayout::FormatRange &range : ranges) {
            const int offset = block.position() + range.start;
            if (offset > pos)
                return {};
            if (offset + range.length <= pos)
                continue;
            return range.format;
        }
        return {};
    };

    const QTextCharFormat formatForStyle = formatForCategory(style);
    for (int pos = startPos; pos <= lastPos; ++pos) {
        const QChar c = m_doc.characterAt(pos);
        if (c == QChar::ParagraphSeparator)
            continue;
        const QTextCharFormat expectedFormat = asSyntaxHighlight(
            c.isSpace() ? whitespacified(formatForStyle) : formatForStyle);

        const QTextCharFormat actualFormat = getActualFormat(pos);
        if (actualFormat != expectedFormat) {
            int posLine;
            int posCol;
            Utils::Text::convertPosition(&m_doc, pos, &posLine, &posCol);
            qDebug() << posLine << posCol << c
                     << actualFormat.foreground() << expectedFormat.foreground()
                     << actualFormat.background() << expectedFormat.background();
        }
        QCOMPARE(actualFormat, expectedFormat);
    }
}

void CppHighlighterTest::testParentheses_data()
{
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("expectedParenCount");

    QTest::newRow("function head") << 41 << 2;
    QTest::newRow("function opening brace") << 42 << 1;
    QTest::newRow("loop head") << 43 << 1;
    QTest::newRow("comment") << 44 << 0;
    QTest::newRow("loop end") << 45 << 3;
    QTest::newRow("function closing brace") << 46 << 1;
}

void CppHighlighterTest::testParentheses()
{
    QFETCH(int, line);
    QFETCH(int, expectedParenCount);

    QTextBlock block = m_doc.findBlockByNumber(line - 1);
    QVERIFY(block.isValid());
    QCOMPARE(TextDocumentLayout::parentheses(block).count(), expectedParenCount);
}

} // namespace Internal
#endif // WITH_TESTS

} // namespace CppEditor

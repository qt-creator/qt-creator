// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdqpropertyhighlighter.h"

#include "moc/parser.h"
#include "moc/preprocessor.h"
#include "moc/utils.h"

#include <utils/textutils.h>

#include <QVersionNumber>

using namespace TextEditor;

namespace ClangCodeModel {
namespace Internal {

class QPropertyHighlighter::Private
{
public:
    QByteArray lexemUntil(Token target);

    void highlightProperty();

    const QTextDocument *document;
    QString input;
    int position;
    Preprocessor pp;
    Parser parser;
    TextEditor::HighlightingResults results;

private:
    void highlightType();
    void highlightAttributes();
    void highlightRevision();
    bool until(Token target);
    void skipCxxAttributes();
    void addResult(TextStyle textStyle, int symbolOffset = 0);
};

QPropertyHighlighter::QPropertyHighlighter(const QTextDocument *doc, const QString &input,
                                           int position)
    : d(new Private)
{
    d->document = doc;
    d->input = input;
    d->position = position;

    d->pp.macros["Q_MOC_RUN"];
    d->pp.macros["__cplusplus"];

    // Don't stumble over GCC extensions
    Macro dummyVariadicFunctionMacro;
    dummyVariadicFunctionMacro.isFunction = true;
    dummyVariadicFunctionMacro.isVariadic = true;
    dummyVariadicFunctionMacro.arguments += Symbol(0, PP_IDENTIFIER, "__VA_ARGS__");
    d->pp.macros["__attribute__"] = dummyVariadicFunctionMacro;
    d->pp.macros["__declspec"] = dummyVariadicFunctionMacro;
}

QPropertyHighlighter::~QPropertyHighlighter() { delete d; }

const HighlightingResults QPropertyHighlighter::highlight()
{
    try {
        d->highlightProperty();
        return d->results;
    } catch (const MocParseException &) {
        return {};
    }
}

void QPropertyHighlighter::Private::highlightProperty()
{
    parser.symbols = pp.preprocessed({}, input.toUtf8());
    parser.next(Q_PROPERTY_TOKEN);
    parser.next(LPAREN);
    highlightType();
    parser.next();
    addResult(C_FIELD);
    highlightAttributes();
}

void QPropertyHighlighter::Private::highlightType()
{
    for (;;) {
        skipCxxAttributes();
        switch (parser.next()) {
        case SIGNED:
        case UNSIGNED:
        case CONST_TOKEN:
        case VOLATILE:
            addResult(C_KEYWORD);
            continue;
        case Q_MOC_COMPAT_TOKEN:
        case Q_INVOKABLE_TOKEN:
        case Q_SCRIPTABLE_TOKEN:
        case Q_SIGNALS_TOKEN:
        case Q_SLOTS_TOKEN:
        case Q_SIGNAL_TOKEN:
        case Q_SLOT_TOKEN:
            return;
        case NOTOKEN:
            return;
        default:
            parser.prev();
            break;
        }
        break;
    }

    skipCxxAttributes();
    parser.test(ENUM) || parser.test(CLASS) || parser.test(STRUCT);
    for(;;) {
        skipCxxAttributes();
        switch (parser.next()) {
        case IDENTIFIER:
            addResult(C_TYPE);
            break;
        case CHAR_TOKEN:
        case SHORT_TOKEN:
        case INT_TOKEN:
        case LONG_TOKEN:
            addResult(C_KEYWORD);
            // preserve '[unsigned] long long', 'short int', 'long int', 'long double'
            if (parser.test(LONG_TOKEN) || parser.test(INT_TOKEN) || parser.test(DOUBLE)) {
                parser.prev();
                continue;
            }
            break;
        case FLOAT_TOKEN:
        case DOUBLE:
        case VOID_TOKEN:
        case BOOL_TOKEN:
        case AUTO:
            addResult(C_KEYWORD);
            break;
        case NOTOKEN:
            return;
        default:
            parser.prev();
            break;
        }
        if (parser.test(LANGLE)) {
            if (results.size() < 2) {
                // '<' cannot start a type
                return;
            }
            until(RANGLE); // TODO: Highlight template parameter?
        }
        if (parser.test(SCOPE)) {
            addResult(C_PUNCTUATION);
        } else {
            break;
        }
    }
    while (parser.test(CONST_TOKEN) || parser.test(VOLATILE) || parser.test(SIGNED)
           || parser.test(UNSIGNED) || parser.test(STAR) || parser.test(AND)
           || parser.test(ANDAND)) {
        TextStyle textStyle = parser.test(CONST_TOKEN) || parser.test(VOLATILE)
                ? C_KEYWORD
                : parser.test(SIGNED) || parser.test(UNSIGNED)
                  ? C_TYPE
                  : C_PUNCTUATION;
        addResult(textStyle);
    }
}

void QPropertyHighlighter::Private::highlightAttributes()
{
    auto checkIsFunction = [&](const QByteArray &def, const char *name) {
        if (def.endsWith(')')) {
            QByteArray msg = "Providing a function for ";
            msg += name;
            msg += " in a property declaration is not be supported in Qt 6.";
            parser.error(msg.constData());
        }
    };

    while (parser.test(IDENTIFIER)) {
        const QByteArray l = parser.lexem();
        if (l[0] == 'C' && l == "CONSTANT") {
            addResult(C_KEYWORD);
            continue;
        } else if (l[0] == 'F' && l == "FINAL") {
            addResult(C_KEYWORD);
            continue;
        } else if (l[0] == 'N' && l == "NAME") {
            addResult(C_KEYWORD);
            parser.next(IDENTIFIER);
            addResult(C_FIELD);
            continue;
        } else if (l[0] == 'R' && l == "REQUIRED") {
            addResult(C_KEYWORD);
            continue;
        } else if (l[0] == 'R' && l == "REVISION" && parser.test(LPAREN)) {
            parser.prev();
            highlightRevision();
            continue;
        }

        QByteArray v, v2;
        if (parser.test(LPAREN)) {
            v = lexemUntil(RPAREN);
            v = v.mid(1, v.length() - 2); // removes the '(' and ')'
        } else if (parser.test(INTEGER_LITERAL)) {
            v = parser.lexem();
            if (l != "REVISION")
                parser.error(1);
        } else {
            addResult(C_KEYWORD);
            parser.next(IDENTIFIER);
            addResult(C_FUNCTION);
            v = parser.lexem();
            if (parser.test(LPAREN))
                v2 = lexemUntil(RPAREN);
            else if (v != "true" && v != "false")
                v2 = "()";
        }
        switch (l[0]) {
        case 'M':
            if (l != "MEMBER")
                parser.error(2);
            break;
        case 'R':
            if (l == "READ")
                break;
            if (l == "RESET")
                break;
            if (l == "REVISION") {
                parser.prev();
                highlightRevision();
                break;
            }
            parser.error(2);
            break;
        case 'S':
            if (l == "SCRIPTABLE") {
                checkIsFunction(v + v2, "SCRIPTABLE");
            } else if (l == "STORED") {
                checkIsFunction(v + v2, "STORED");
            } else
                parser.error(2);
            break;
        case 'W': if (l != "WRITE") parser.error(2);
            break;
        case 'B': if (l != "BINDABLE") parser.error(2);
            break;
        case 'D': if (l != "DESIGNABLE") parser.error(2);
            checkIsFunction(v + v2, "DESIGNABLE");
            break;
        case 'N': if (l != "NOTIFY") parser.error(2);
            break;
        case 'U': if (l != "USER") parser.error(2);
            checkIsFunction(v + v2, "USER");
            break;
        default:
            parser.error(2);
        }
    }
}

void QPropertyHighlighter::Private::highlightRevision()
{
    addResult(C_KEYWORD);
    QByteArray revisionString;
    const bool hasParen = parser.test(LPAREN);
    if (hasParen) {
        revisionString = lexemUntil(RPAREN);
        revisionString.remove(0, 1);
        revisionString.chop(1);
        revisionString.replace(',', '.');
    } else {
        parser.next(INTEGER_LITERAL);
        revisionString = parser.lexem();
    }
    QVersionNumber version = QVersionNumber::fromString(QString::fromUtf8(revisionString));
    if (version.isNull() || version.segmentCount() > 2)
        parser.error("Invalid revision");
}

QByteArray QPropertyHighlighter::Private::lexemUntil(Token target)
{
    int from = parser.index;
    until(target);
    QByteArray s;
    while (from <= parser.index) {
        QByteArray n = parser.symbols.at(from++-1).lexem();
        if (s.size() && n.size()) {
            char prev = s.at(s.size()-1);
            char next = n.at(0);
            if ((is_ident_char(prev) && is_ident_char(next))
                    || (prev == '<' && next == ':')
                    || (prev == '>' && next == '>'))
                s += ' ';
        }
        s += n;
    }
    return s;
}

bool QPropertyHighlighter::Private::until(Token target)
{
    int braceCount = 0;
    int brackCount = 0;
    int parenCount = 0;
    int angleCount = 0;
    if (parser.index) {
        switch(parser.symbols.at(parser.index-1).token) {
        case LBRACE: ++braceCount; break;
        case LBRACK: ++brackCount; break;
        case LPAREN: ++parenCount; break;
        case LANGLE: ++angleCount; break;
        default: break;
        }
    }

    //when searching commas within the default argument, we should take care of template depth (anglecount)
    // unfortunatelly, we do not have enough semantic information to know if '<' is the operator< or
    // the beginning of a template type. so we just use heuristics.
    int possible = -1;

    while (parser.index < parser.symbols.size()) {
        Token t = parser.symbols.at(parser.index++).token;
        switch (t) {
        case LBRACE: ++braceCount; break;
        case RBRACE: --braceCount; break;
        case LBRACK: ++brackCount; break;
        case RBRACK: --brackCount; break;
        case LPAREN: ++parenCount; break;
        case RPAREN: --parenCount; break;
        case LANGLE:
            if (parenCount == 0 && braceCount == 0)
                ++angleCount;
            break;
        case RANGLE:
            if (parenCount == 0 && braceCount == 0)
                --angleCount;
            break;
        case GTGT:
            if (parenCount == 0 && braceCount == 0) {
                angleCount -= 2;
                t = RANGLE;
            }
            break;
        default: break;
        }
        if (t == target
                && braceCount <= 0
                && brackCount <= 0
                && parenCount <= 0
                && (target != RANGLE || angleCount <= 0)) {
            if (target != COMMA || angleCount <= 0)
                return true;
            possible = parser.index;
        }

        if (target == COMMA && t == EQ && possible != -1) {
            parser.index = possible;
            return true;
        }

        if (braceCount < 0 || brackCount < 0 || parenCount < 0
                || (target == RANGLE && angleCount < 0)) {
            --parser.index;
            break;
        }

        if (braceCount <= 0 && t == SEMIC) {
            // Abort on semicolon. Allow recovering bad template parsing (QTBUG-31218)
            break;
        }
    }

    if (target == COMMA && angleCount != 0 && possible != -1) {
        parser.index = possible;
        return true;
    }

    return false;
}

void QPropertyHighlighter::Private::skipCxxAttributes()
{
    int rewind = parser.index;
    if (parser.test(LBRACK) && parser.test(LBRACK) && until(RBRACK) && parser.test(RBRACK))
        return;
    parser.index = rewind;
}

void QPropertyHighlighter::Private::addResult(TextStyle textStyle, int symbolOffset)
{
    const Symbol &s = parser.symbol_lookup(symbolOffset);
    int line, column;
    Utils::Text::convertPosition(document, position + s.from, &line, &column);
    if (line > 0 && column >= 0) {
        TextStyles styles;
        styles.mainStyle = textStyle;
        results << HighlightingResult(line, column + 1, s.len, styles);
    }
}

} // namespace Internal
} // namespace ClangCodeModel

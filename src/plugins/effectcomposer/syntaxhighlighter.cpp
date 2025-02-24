// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "syntaxhighlighter.h"

#include "syntaxhighlighterdata.h"

#include <texteditor/fontsettings.h>

#include <glsl/glslparser.h>
#include <utils/algorithm.h>

#include <QTextCharFormat>

namespace {

enum class Category {
    Argument,
    Attribute,
    Comment,
    GlFunction,
    GlType,
    Identifier,
    Keyword,
    Normal,
    Number,
    Operator,
    Primitive,
    Symbol,
    Count,
};

union StateUnion {
    struct
    {
        quint32 lexerState : 8;
        quint32 braceDepth : 8;
        quint32 kind : 8;
    };
    int data;

    explicit StateUnion(int val)
    {
        if (val == -1) {
            lexerState = 0;
            braceDepth = 0;
            kind = 0;
        } else {
            data = val;
        }
    }

    explicit StateUnion(quint8 lexerState, quint8 braceDepth, quint8 kind)
    {
        this->lexerState = lexerState;
        this->braceDepth = braceDepth;
        this->kind = kind;
    }
};

bool isArgument(const QByteArray &data)
{
    static const QSet<QByteArrayView> argumentKeywords = Utils::toSet(
        EffectComposer::SyntaxHighlighterData::reservedArgumentNames());
    return argumentKeywords.contains(data);
}

bool isReservedFunc(const QByteArray &data)
{
    static const QSet<QByteArrayView> reservedFunctions = [] {
        auto items = Utils::transform(
            EffectComposer::SyntaxHighlighterData::reservedFunctionNames(),
            [](const QByteArrayView &funcName) { return funcName.chopped(2); });
        return Utils::toSet(items);
    }();
    return reservedFunctions.contains(data);
}

Category classifyToken(int tokenKind)
{
    using Kind = GLSL::Parser::VariousConstants;

    switch (Kind(tokenKind)) {
        // Operator
    case Kind::T_AMPERSAND:
    case Kind::T_AND_ASSIGN:
    case Kind::T_DIV_ASSIGN:
    case Kind::T_ADD_ASSIGN:
    case Kind::T_OR_ASSIGN:
    case Kind::T_MOD_ASSIGN:
    case Kind::T_MUL_ASSIGN:
    case Kind::T_QUESTION:
    case Kind::T_EQUAL:
    case Kind::T_STAR:
    case Kind::T_PLUS:
    case Kind::T_DASH:
    case Kind::T_COLON:
    case Kind::T_TILDE:
    case Kind::T_CARET:
    case Kind::T_AND_OP:
    case Kind::T_DEC_OP:
    case Kind::T_EQ_OP:
    case Kind::T_GE_OP:
    case Kind::T_INC_OP:
    case Kind::T_LEFT_OP:
    case Kind::T_LE_OP:
    case Kind::T_NE_OP:
    case Kind::T_OR_OP:
    case Kind::T_RIGHT_OP:
    case Kind::T_XOR_OP:
    case Kind::T_XOR_ASSIGN:
        return Category::Operator;

    case Kind::T_VOID:
    case Kind::T_BOOL:
    case Kind::T_INT:
    case Kind::T_UINT:
    case Kind::T_DOUBLE:
    case Kind::T_FLOAT:
        return Category::Primitive;

    case Kind::T_BVEC2:
    case Kind::T_BVEC3:
    case Kind::T_BVEC4:
    case Kind::T_DMAT2:
    case Kind::T_DMAT2X2:
    case Kind::T_DMAT2X3:
    case Kind::T_DMAT2X4:
    case Kind::T_DMAT3:
    case Kind::T_DMAT3X2:
    case Kind::T_DMAT3X3:
    case Kind::T_DMAT3X4:
    case Kind::T_DMAT4:
    case Kind::T_DMAT4X2:
    case Kind::T_DMAT4X3:
    case Kind::T_DMAT4X4:
    case Kind::T_DVEC2:
    case Kind::T_DVEC3:
    case Kind::T_DVEC4:
    case Kind::T_IVEC2:
    case Kind::T_IVEC3:
    case Kind::T_IVEC4:
    case Kind::T_MAT2:
    case Kind::T_MAT2X2:
    case Kind::T_MAT2X3:
    case Kind::T_MAT2X4:
    case Kind::T_MAT3:
    case Kind::T_MAT3X2:
    case Kind::T_MAT3X3:
    case Kind::T_MAT3X4:
    case Kind::T_MAT4:
    case Kind::T_MAT4X2:
    case Kind::T_MAT4X3:
    case Kind::T_MAT4X4:
    case Kind::T_UVEC2:
    case Kind::T_UVEC3:
    case Kind::T_UVEC4:
    case Kind::T_VEC2:
    case Kind::T_VEC3:
    case Kind::T_VEC4:
        return Category::GlType;

        // Keyword
    case Kind::T_SWITCH:
    case Kind::T_CASE:
    case Kind::T_DEFAULT:
    case Kind::T_CONST:
    case Kind::T_DO:
    case Kind::T_IF:
    case Kind::T_ELSE:
    case Kind::T_WHILE:
    case Kind::T_FOR:
    case Kind::T_BREAK:
    case Kind::T_CONTINUE:
    case Kind::T_IN:
    case Kind::T_TRUE:
    case Kind::T_FALSE:
    case Kind::T_UNIFORM:
    case Kind::T_SUBROUTINE:
    case Kind::T_RETURN:
        return Category::Keyword;

    case Kind::T_NUMBER:
        return Category::Number;

    case Kind::T_COMMENT:
        return Category::Comment;

    case Kind::T_ERROR:
        return Category::Symbol;

    default:
        return Category::Normal;
    }
}

TextEditor::TextStyle styleForCategory(int format)
{
    using namespace TextEditor;

    static const QMap<Category, TextEditor::TextStyle> styleMap{
        {Category::Argument, TextEditor::C_OUTPUT_ARGUMENT},
        {Category::Attribute, TextEditor::C_ATTRIBUTE},
        {Category::Comment, TextEditor::C_COMMENT},
        {Category::GlFunction, TextEditor::C_GLOBAL},
        {Category::GlType, TextEditor::C_JS_GLOBAL_VAR},
        {Category::Identifier, TextEditor::C_QML_LOCAL_ID},
        {Category::Keyword, TextEditor::C_KEYWORD},
        {Category::Normal, TextEditor::C_TEXT},
        {Category::Number, TextEditor::C_NUMBER},
        {Category::Operator, TextEditor::C_OPERATOR},
        {Category::Primitive, TextEditor::C_PRIMITIVE_TYPE},
        {Category::Symbol, TextEditor::C_LABEL},
    };

    const Category cat = Category(format);

    QTC_ASSERT(styleMap.contains(cat), return C_TEXT);

    return styleMap.value(cat);
}

Category getIdentifierCategory(int previousTokenKind, const QByteArray &data)
{
    using Kind = GLSL::Parser::VariousConstants;

    if (previousTokenKind == Kind::T_DOT)
        return Category::Attribute;

    if (previousTokenKind == Kind::T_ERROR) // T_ERROR means @
        return Category::Symbol;

    if (::isArgument(data))
        return Category::Argument;

    if (::isReservedFunc(data))
        return Category::GlFunction;

    return Category::Normal;
}

} // namespace

namespace EffectComposer {

SyntaxHighlighter::SyntaxHighlighter()
    : TextEditor::SyntaxHighlighter()
{
    setTextFormatCategories(int(Category::Count), ::styleForCategory);
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    using TextEditor::TextStyle;
    using Kind = GLSL::Parser::VariousConstants;

    const int prevBlockState = previousBlockState();
    const StateUnion state(prevBlockState);

    const QByteArray blockBytes = text.toLatin1();
    GLSL::Engine engine;
    GLSL::Lexer lex(&engine, blockBytes.constData(), blockBytes.size());

    lex.setState(state.lexerState);
    lex.setScanKeywords(true);
    lex.setScanComments(true);

    lex.setVariant(GLSL::Lexer::Variant_GLSL_400);

    QList<GLSL::Token> tokens;
    GLSL::Token tk;
    int prevTokenKind = state.kind;

    // Hack: If the state is one, it means that the lexer has stopped on a multiline comment
    bool multilinecommentState = state.lexerState == 1;

    do {
        lex.yylex(&tk);
        tokens.append(tk);
        if (tk.is(Kind::EOF_SYMBOL))
            break;

        const QByteArray tokenData(blockBytes.mid(tk.position, tk.length));

        if (tk.is(Kind::T_IDENTIFIER)) {
            Category idCategory = ::getIdentifierCategory(prevTokenKind, tokenData);
            setFormat(tk.begin(), tk.length, formatForCategory(int(idCategory)));
        } else if (tk.is(Kind::T_COMMENT)) {
            if (tokenData.startsWith(R"(/*)"))
                multilinecommentState = true;

            if (multilinecommentState && tokenData.endsWith(R"(*/)"))
                multilinecommentState = false;

            highlightToken(tk);
        } else {
            highlightToken(tk);
        }

        prevTokenKind = tk.kind;
    } while (true);

    if (multilinecommentState)
        prevTokenKind = 1; // Hack: To return it back to the comment state!

    const StateUnion blockState(lex.state(), 0, prevTokenKind);
    setCurrentBlockState(blockState.data);

    formatSpaces(text);
}

void SyntaxHighlighter::highlightToken(const GLSL::Token &token)
{
    using Kind = GLSL::Parser::VariousConstants;
    const Kind kind = Kind(token.kind);

    const int category = int(::classifyToken(kind));
    setFormat(token.begin(), token.length, formatForCategory(category));
}

} // namespace EffectComposer

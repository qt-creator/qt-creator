#ifndef CPPCODEFORMATTER_H
#define CPPCODEFORMATTER_H

#include "cpptools_global.h"

#include <cplusplus/SimpleLexer.h>
#include <Token.h>

#include <QtCore/QChar>
#include <QtCore/QStack>
#include <QtCore/QList>
#include <QtCore/QVector>
#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE
class QTextDocument;
class QTextBlock;
QT_END_NAMESPACE

namespace CppTools {    
namespace Internal {
class CppCodeFormatterData;
}

class CPPTOOLS_EXPORT CodeFormatter
{
public:
    CodeFormatter();
    virtual ~CodeFormatter();

    void updateStateUntil(const QTextBlock &block);
    int indentFor(const QTextBlock &block);

    void setTabSize(int tabSize);

    static void invalidateCache(QTextDocument *document);

protected:
    virtual void onEnter(int newState, int *indentDepth, int *savedIndentDepth) const = 0;
    virtual void adjustIndent(const QList<CPlusPlus::Token> &tokens, int lexerState, int *indentDepth) const = 0;

protected:
    enum StateType {
        invalid = 0,

        topmost_intro, // The first line in a "topmost" definition.

        multiline_comment_start, // Inside the first line of a multi-line C style block comment.
        multiline_comment_cont, // Inside the following lines of a multi-line C style block comment.
        cpp_macro_start, // After the '#' token
        cpp_macro, // The start of a C preprocessor macro definition.
        cpp_macro_cont, // Subsequent lines of a multi-line C preprocessor macro definition.
        cpp_macro_conditional, // Special marker used for separating saved from current state when dealing with #ifdef
        qt_like_macro, // after an identifier starting with Q_ or QT_ at the beginning of the line

        defun_open, // Brace that opens a top-level function definition.
        using_start, // right after the "using" token

        class_start, // after the 'class' token
        class_open, // Brace that opens a class definition.
        member_init_open, // After ':' that starts a member initialization list.

        enum_start, // After 'enum'
        enum_open, // Brace that opens a enum declaration.
        brace_list_open, // Open brace nested inside an enum or for a static array list.

        namespace_start, // after the namespace token, before the opening brace.
        namespace_open, // Brace that opens a C++ namespace block.

        declaration_start, // shifted a token which could start a declaration.
        operator_declaration, // after 'operator' in declaration_start

        template_start, // after the 'template' token
        template_param, // after the '<' in a template_start

        if_statement, // After 'if'
        maybe_else, // after the first substatement in an if
        else_clause, // The else line of an if-else construct.

        for_statement, // After the 'for' token
        for_statement_paren_open, // While inside the (...)
        for_statement_init, // The initializer part of the for statement
        for_statement_condition, // The condition part of the for statement
        for_statement_expression, // The expression part of the for statement

        switch_statement, // After 'switch' token
        case_start, // after a 'case' or 'default' token
        case_cont, // after the colon in a case/default

        statement_with_condition, // A statement that takes a condition after the start token.
        do_statement, // After 'do' token
        return_statement, // After 'return'
        block_open, // Statement block open brace.

        substatement, // The first line after a conditional or loop construct.
        substatement_open, // The brace that opens a substatement block.

        arglist_open, // after the lparen. TODO: check if this is enough.
        stream_op, // Lines continuing a stream operator (C++ only).
        ternary_op, // The ? : operator

        condition_open, // Start of a condition in 'if', 'while', entered after opening paren
        condition_paren_open, // After an lparen in a condition

        assign_open, // after an assignment token

        expression, // after a '=' in a declaration_start once we're sure it's not '= {'
        initializer, // after a '=' in a declaration start
    };

    struct State {
        State()
            : savedIndentDepth(0)
            , type(0)
        {}

        State(quint8 ty, quint16 savedDepth)
            : savedIndentDepth(savedDepth)
            , type(ty)
        {}

        quint16 savedIndentDepth;
        quint8 type;

        bool operator==(const State &other) const {
            return type == other.type
                && savedIndentDepth == other.savedIndentDepth;
        }
    };

    State state(int belowTop = 0) const;
    const QVector<State> &newStatesThisLine() const;
    int tokenIndex() const;
    int tokenCount() const;
    const CPlusPlus::Token &currentToken() const;
    const CPlusPlus::Token &tokenAt(int idx) const;
    int column(int position) const;

    bool isBracelessState(int type) const;

private:
    void recalculateStateAfter(const QTextBlock &block);
    void storeBlockState(const QTextBlock &block);
    void restoreBlockState(const QTextBlock &block);

    QStringRef currentTokenText() const;

    int tokenizeBlock(const QTextBlock &block, bool *endedJoined = 0);

    void turnInto(int newState);

    bool tryExpression(bool alsoExpression = false);
    bool tryDeclaration();
    bool tryStatement();

    void enter(int newState);
    void leave(bool statementDone = false);
    void correctIndentation(const QTextBlock &block);

    void dump();

private:
    static QStack<State> initialState();

    QStack<State> m_beginState;
    QStack<State> m_currentState;
    QStack<State> m_newStates;

    QList<CPlusPlus::Token> m_tokens;
    QString m_currentLine;
    CPlusPlus::Token m_currentToken;
    int m_tokenIndex;

    // should store indent level and padding instead
    int m_indentDepth;

    int m_tabSize;

    friend class Internal::CppCodeFormatterData;
};

class CPPTOOLS_EXPORT QtStyleCodeFormatter : public CodeFormatter
{
public:
    QtStyleCodeFormatter();

    void setIndentSize(int size);

    void setIndentSubstatementBraces(bool onOff);
    void setIndentSubstatementStatements(bool onOff);
    void setIndentDeclarationBraces(bool onOff);
    void setIndentDeclarationMembers(bool onOff);

protected:
    virtual void onEnter(int newState, int *indentDepth, int *savedIndentDepth) const;
    virtual void adjustIndent(const QList<CPlusPlus::Token> &tokens, int lexerState, int *indentDepth) const;

private:
    int m_indentSize;
    bool m_indentSubstatementBraces;
    bool m_indentSubstatementStatements;
    bool m_indentDeclarationBraces;
    bool m_indentDeclarationMembers;
};

} // namespace CppTools

#endif // CPPCODEFORMATTER_H

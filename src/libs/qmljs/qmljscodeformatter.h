/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLJSCODEFORMATTER_H
#define QMLJSCODEFORMATTER_H

#include "qmljs_global.h"

#include "qmljsscanner.h"

#include <QtCore/QStack>
#include <QtCore/QList>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE
class QTextDocument;
class QTextBlock;
QT_END_NAMESPACE

namespace QmlJS {

class QMLJS_EXPORT CodeFormatter
{
    Q_GADGET
public:
    CodeFormatter();
    virtual ~CodeFormatter();

    // updates all states up until block if necessary
    // it is safe to call indentFor on block afterwards
    void updateStateUntil(const QTextBlock &block);

    // calculates the state change introduced by changing a single line
    void updateLineStateChange(const QTextBlock &block);

    int indentFor(const QTextBlock &block);
    int indentForNewLineAfter(const QTextBlock &block);

    void setTabSize(int tabSize);

    void invalidateCache(QTextDocument *document);

protected:
    virtual void onEnter(int newState, int *indentDepth, int *savedIndentDepth) const = 0;
    virtual void adjustIndent(const QList<Token> &tokens, int lexerState, int *indentDepth) const = 0;

    struct State;
    class QMLJS_EXPORT BlockData
    {
    public:
        BlockData();

        QStack<State> m_beginState;
        QStack<State> m_endState;
        int m_indentDepth;
        int m_blockRevision;
    };

    virtual void saveBlockData(QTextBlock *block, const BlockData &data) const = 0;
    virtual bool loadBlockData(const QTextBlock &block, BlockData *data) const = 0;

    virtual void saveLexerState(QTextBlock *block, int state) const = 0;
    virtual int loadLexerState(const QTextBlock &block) const = 0;

public: // must be public to make Q_GADGET introspection work
    enum StateType {
        invalid = 0,

        topmost_intro, // The first line in a "topmost" definition.

        top_qml, // root state for qml
        top_js, // root for js
        objectdefinition_or_js, // file starts with identifier

        multiline_comment_start,
        multiline_comment_cont,

        import_start, // after 'import'
        import_maybe_dot_or_version_or_as, // after string or identifier
        import_dot, // after .
        import_maybe_as, // after version
        import_as,

        property_start, // after 'property'
        default_property_start, // after 'default'
        property_type, // after first identifier
        property_list_open, // after 'list' as a type
        property_maybe_initializer, // after

        signal_start, // after 'signal'
        signal_maybe_arglist, // after identifier
        signal_arglist_open, // after '('

        function_start, // after 'function'
        function_arglist_open, // after '(' starting function argument list
        function_arglist_closed, // after ')' in argument list, expecting '{'

        binding_or_objectdefinition, // after an identifier

        binding_assignment, // after :
        objectdefinition_open, // after {

        expression,
        expression_continuation, // at the end of the line, when the next line definitely is a continuation
        expression_maybe_continuation, // at the end of the line, when the next line may be an expression
        expression_or_objectdefinition, // after a binding starting with an identifier ("x: foo")

        paren_open, // opening ( in expression
        bracket_open, // opening [ in expression
        objectliteral_open, // opening { in expression

        bracket_element_start, // after starting bracket_open or after ',' in bracket_open
        bracket_element_maybe_objectdefinition, // after an identifier in bracket_element_start

        ternary_op, // The ? : operator

        jsblock_open,

        empty_statement, // for a ';', will be popped directly
        breakcontinue_statement, // for continue/break, will be popped directly

        if_statement, // After 'if'
        maybe_else, // after the first substatement in an if
        else_clause, // The else line of an if-else construct.

        condition_open, // Start of a condition in 'if', 'while', entered after opening paren
        condition_paren_open, // After an lparen in a condition

        substatement, // The first line after a conditional or loop construct.
        substatement_open, // The brace that opens a substatement block.

        return_statement, // After 'return'
        throw_statement, // After 'throw'

        statement_with_condition, // After the 'for', 'while', 'catch', ... token
        statement_with_condition_paren_open, // While inside the (...)

        statement_with_block, // try, finally

        do_statement, // after 'do'
        do_statement_while_paren_open, // after '(' in while clause

        switch_statement, // After 'switch' token
        case_start, // after a 'case' or 'default' token
        case_cont // after the colon in a case/default
    };
    Q_ENUMS(StateType)

protected:
    // extends Token::Kind from qmljsscanner.h
    // the entries until EndOfExistingTokenKinds must match
    enum TokenKind {
        EndOfFile,
        Keyword,
        Identifier,
        String,
        Comment,
        Number,
        LeftParenthesis,
        RightParenthesis,
        LeftBrace,
        RightBrace,
        LeftBracket,
        RightBracket,
        Semicolon,
        Colon,
        Comma,
        Dot,
        Delimiter,

        EndOfExistingTokenKinds,

        Break,
        Case,
        Catch,
        Continue,
        Debugger,
        Default,
        Delete,
        Do,
        Else,
        Finally,
        For,
        Function,
        If,
        In,
        Instanceof,
        New,
        Return,
        Switch,
        This,
        Throw,
        Try,
        Typeof,
        Var,
        Void,
        While,
        With,

        Import,
        Signal,
        On,
        As,
        List,
        Property,

        Question,
        PlusPlus,
        MinusMinus
    };

    TokenKind extendedTokenKind(const QmlJS::Token &token) const;

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
    const Token &currentToken() const;
    const Token &tokenAt(int idx) const;
    int column(int position) const;

    bool isBracelessState(int type) const;
    bool isExpressionEndState(int type) const;

    void dump() const;

private:
    void recalculateStateAfter(const QTextBlock &block);
    void saveCurrentState(const QTextBlock &block);
    void restoreCurrentState(const QTextBlock &block);

    QStringRef currentTokenText() const;

    int tokenizeBlock(const QTextBlock &block);

    void turnInto(int newState);

    bool tryInsideExpression(bool alsoExpression = false);
    bool tryStatement();

    void enter(int newState);
    void leave(bool statementDone = false);
    void correctIndentation(const QTextBlock &block);

private:
    static QStack<State> initialState();

    QStack<State> m_beginState;
    QStack<State> m_currentState;
    QStack<State> m_newStates;

    QList<Token> m_tokens;
    QString m_currentLine;
    Token m_currentToken;
    int m_tokenIndex;

    // should store indent level and padding instead
    int m_indentDepth;

    int m_tabSize;
};

} // namespace QmlJS

#endif // QMLJSCODEFORMATTER_H

/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPCODEFORMATTER_H
#define CPPCODEFORMATTER_H

#include "cpptools_global.h"
#include "cppcodestylesettings.h"

#include <texteditor/tabsettings.h>

#include <cplusplus/Token.h>
#include <cplusplus/SimpleLexer.h>

#include <QStack>
#include <QList>
#include <QVector>

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
    Q_GADGET
public:
    CodeFormatter();
    virtual ~CodeFormatter();

    // updates all states up until block if necessary
    // it is safe to call indentFor on block afterwards
    void updateStateUntil(const QTextBlock &block);

    // calculates the state change introduced by changing a single line
    void updateLineStateChange(const QTextBlock &block);

    void indentFor(const QTextBlock &block, int *indent, int *padding);
    void indentForNewLineAfter(const QTextBlock &block, int *indent, int *padding);

    void setTabSize(int tabSize);

    void invalidateCache(QTextDocument *document);

protected:
    virtual void onEnter(int newState, int *indentDepth, int *savedIndentDepth, int *paddingDepth, int *savedPaddingDepth) const = 0;
    virtual void adjustIndent(const QList<CPlusPlus::Token> &tokens, int lexerState, int *indentDepth, int *paddingDepth) const = 0;

    class State;
    class BlockData
    {
    public:
        BlockData();

        QStack<State> m_beginState;
        QStack<State> m_endState;
        int m_indentDepth;
        int m_paddingDepth;
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

        multiline_comment_start, // Inside the first line of a multi-line C style block comment.
        multiline_comment_cont, // Inside the following lines of a multi-line C style block comment.
        cpp_macro_start, // After the '#' token
        cpp_macro, // The start of a C preprocessor macro definition.
        cpp_macro_cont, // Subsequent lines of a multi-line C preprocessor macro definition.
        cpp_macro_conditional, // Special marker used for separating saved from current state when dealing with #ifdef
        qt_like_macro, // after an identifier starting with Q_ or QT_ at the beginning of the line
        label, // after an identifier followed by a colon

        defun_open, // Brace that opens a top-level function definition.
        using_start, // right after the "using" token

        class_start, // after the 'class' token
        class_open, // Brace that opens a class definition.

        access_specifier_start, // after 'private', 'protected' etc.

        member_init_open, // After ':' that starts a member initialization list.
        member_init_expected, // At the start and after every ',' in member_init_open
        member_init, // After an identifier in member_init_expected
        member_init_nest_open, // After '(' or '{' in member_init.

        enum_start, // After 'enum'
        enum_open, // Brace that opens a enum declaration.
        brace_list_open, // Open brace nested inside an enum or for a static array list.

        namespace_start, // after the namespace token, before the opening brace.
        namespace_open, // Brace that opens a C++ namespace block.

        extern_start, // after the extern token, before the opening brace.
        extern_open, // Brace that opens a C++ extern block.

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
        stream_op, // After a '<<' or '>>' in a context where it's likely a stream operator.
        stream_op_cont, // When finding another stream operator in stream_op
        ternary_op, // The ? : operator
        braceinit_open, // after '{' in an expression context

        condition_open, // Start of a condition in 'if', 'while', entered after opening paren
        condition_paren_open, // After an lparen in a condition

        assign_open, // after an assignment token

        expression, // after a '=' in a declaration_start once we're sure it's not '= {'
        assign_open_or_initializer, // after a '=' in a declaration start

        lambda_instroducer_or_subscribtion, // just after '[' or in cases '[]' and '[id]' when we're not sure in the exact kind of expression
        lambda_declarator_expected, // just after ']' in lambda_introducer_or_subscribtion
        lambda_declarator_or_expression, // just after '](' when previous state is 'lambda_instroducer_or_subscribtion'
        lambda_statement_expected,
        lambda_instroducer,              // when '=', '&' or ',' occurred within '[]'
        lambda_declarator,               // just after ']' when previous state is lambda_introducer
        lambda_statement                 // just after '{' when previous state is lambda_declarator or lambda_declarator_or_expression

    };
    Q_ENUMS(StateType)

protected:
    class State {
    public:
        State()
            : savedIndentDepth(0)
            , savedPaddingDepth(0)
            , type(0)
        {}

        State(quint8 ty, quint16 savedIndentDepth, qint16 savedPaddingDepth)
            : savedIndentDepth(savedIndentDepth)
            , savedPaddingDepth(savedPaddingDepth)
            , type(ty)
        {}

        quint16 savedIndentDepth;
        quint16 savedPaddingDepth;
        quint8 type;

        bool operator==(const State &other) const {
            return type == other.type
                && savedIndentDepth == other.savedIndentDepth
                && savedPaddingDepth == other.savedPaddingDepth;
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

    void dump() const;

private:
    void recalculateStateAfter(const QTextBlock &block);
    void saveCurrentState(const QTextBlock &block);
    void restoreCurrentState(const QTextBlock &block);

    QStringRef currentTokenText() const;

    int tokenizeBlock(const QTextBlock &block, bool *endedJoined = 0);

    void turnInto(int newState);

    bool tryExpression(bool alsoExpression = false);
    bool tryDeclaration();
    bool tryStatement();

    void enter(int newState);
    void leave(bool statementDone = false);
    void correctIndentation(const QTextBlock &block);

private:
    static QStack<State> initialState();

    QStack<State> m_beginState;
    QStack<State> m_currentState;
    QStack<State> m_newStates;

    QList<CPlusPlus::Token> m_tokens;
    QString m_currentLine;
    CPlusPlus::Token m_currentToken;
    int m_tokenIndex;

    int m_indentDepth;
    int m_paddingDepth;

    int m_tabSize;

    friend class Internal::CppCodeFormatterData;
};

class CPPTOOLS_EXPORT QtStyleCodeFormatter : public CodeFormatter
{
public:
    QtStyleCodeFormatter();
    QtStyleCodeFormatter(const TextEditor::TabSettings &tabSettings,
                         const CppCodeStyleSettings &settings);

    void setTabSettings(const TextEditor::TabSettings &tabSettings);
    void setCodeStyleSettings(const CppCodeStyleSettings &settings);

protected:
    virtual void onEnter(int newState, int *indentDepth, int *savedIndentDepth, int *paddingDepth, int *savedPaddingDepth) const;
    virtual void adjustIndent(const QList<CPlusPlus::Token> &tokens, int lexerState, int *indentDepth, int *paddingDepth) const;

    virtual void saveBlockData(QTextBlock *block, const BlockData &data) const;
    virtual bool loadBlockData(const QTextBlock &block, BlockData *data) const;

    virtual void saveLexerState(QTextBlock *block, int state) const;
    virtual int loadLexerState(const QTextBlock &block) const;

    static bool shouldClearPaddingOnEnter(int state);

private:
    void addContinuationIndent(int *paddingDepth) const;

    TextEditor::TabSettings m_tabSettings;
    CppCodeStyleSettings m_styleSettings;
};

} // namespace CppTools

#endif // CPPCODEFORMATTER_H

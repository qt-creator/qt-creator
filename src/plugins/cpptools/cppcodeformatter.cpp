/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cppcodeformatter.h"

#include <Token.h>
#include <Lexer.h>

#include <texteditor/basetextdocumentlayout.h>

#include <QtCore/QDebug>
#include <QtGui/QTextDocument>
#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>

using namespace CPlusPlus;
using namespace CppTools;
using namespace TextEditor;
using namespace CppTools::Internal;

CodeFormatter::BlockData::BlockData()
    : m_blockRevision(-1)
{
}

CodeFormatter::CodeFormatter()
    : m_indentDepth(0)
    , m_tabSize(4)
{
}

CodeFormatter::~CodeFormatter()
{
}

void CodeFormatter::setTabSize(int tabSize)
{
    m_tabSize = tabSize;
}

void CodeFormatter::recalculateStateAfter(const QTextBlock &block)
{
    restoreCurrentState(block.previous());

    bool endedJoined = false;
    const int lexerState = tokenizeBlock(block, &endedJoined);
    m_tokenIndex = 0;
    m_newStates.clear();

    if (tokenAt(0).kind() == T_POUND) {
        enter(cpp_macro_start);
        m_tokenIndex = 1;
    }

    for (; m_tokenIndex < m_tokens.size(); ) {
        m_currentToken = tokenAt(m_tokenIndex);
        const int kind = m_currentToken.kind();

        switch (m_currentState.top().type) {
        case topmost_intro:
            tryDeclaration();
            break;

        case namespace_start:
            switch (kind) {
            case T_LBRACE:      enter(namespace_open); break;
            case T_RBRACE:      leave(); break;
            } break;

        case namespace_open:
            if (tryDeclaration())
                break;
            switch (kind) {
            case T_RBRACE:      leave(); continue; // always nested in namespace_start
            } break;

        case class_start:
            switch (kind) {
            case T_SEMICOLON:   leave(); break;
            case T_LBRACE:      enter(class_open); break;
            } break;

        case class_open:
            if (tryDeclaration())
                break;
            switch (kind) {
            case T_RBRACE:      leave(); continue; // always nested in class_start
            } break;

        case enum_start:
            switch (kind) {
            case T_SEMICOLON:   leave(); break;
            case T_LBRACE:      enter(enum_open); break;
            } break;

        case enum_open:
            switch (kind) {
            case T_RBRACE:      leave(); continue; // always nested in enum_start
            case T_LBRACE:      enter(brace_list_open); break;
            } break;

        case brace_list_open:
            switch (kind) {
            case T_RBRACE:      leave(); break;
            case T_LBRACE:      enter(brace_list_open); break;
            } break;

        case using_start:
            switch (kind) {
            case T_SEMICOLON:   leave(); break;
            } break;

        case template_start:
            switch (kind) {
            case T_LESS:        turnInto(template_param); break;
            } break;

        case template_param:
            switch (kind) {
            case T_LESS:        enter(template_param); break;
            case T_GREATER:     leave(); break;
            } break;

        case operator_declaration:
            switch (kind) {
            case T_LPAREN:      break;
            default:            leave(); break;
            } break;

        case declaration_start:
            if (tryExpression(true))
                break;
            switch (kind) {
            case T_RBRACE:
            case T_SEMICOLON:   leave(true); break;
            case T_EQUAL:       enter(initializer); break;
            case T_LBRACE:      enter(defun_open); break;
            case T_COLON:       enter(member_init_open); break;
            case T_OPERATOR:    enter(operator_declaration); break;
            } break;

        case initializer:
            switch (kind) {
            case T_LBRACE:      enter(brace_list_open); break;
            default:            turnInto(expression); continue;
            } break;

        case expression:
            if (tryExpression())
                break;
            switch (kind) {
            case T_SEMICOLON:   leave(); continue;
            case T_LBRACE:
            case T_COLON:
                if (m_currentState.at(m_currentState.size() - 2).type == declaration_start) {
                    // oops, the expression was a function declaration argument list, hand lbrace/colon to declaration_start
                    leave();
                    continue;
                }
                break;
            } break;

        case arglist_open:
            if (tryExpression())
                break;
            switch (kind) {
            case T_RPAREN:      leave(); break;
            } break;

        case ternary_op:
            if (tryExpression())
                break;
            switch (kind) {
            case T_RPAREN:
            case T_COMMA:
            case T_SEMICOLON:   leave(); continue; // always nested, propagate
            } break;

        case stream_op:
        case stream_op_cont:
            if (kind != T_LESS_LESS && kind != T_GREATER_GREATER && tryExpression())
                break;
            switch (kind) {
            case T_LESS_LESS:
            case T_GREATER_GREATER:
                if (m_currentState.top().type == stream_op)
                    enter(stream_op_cont);
                else // stream_op_cont already
                    turnInto(stream_op_cont);
                break;
            case T_COMMA:
            case T_SEMICOLON:   leave(); continue; // always nested, propagate semicolon
            } break;

        case member_init_open:
            switch (kind) {
            case T_LBRACE:      turnInto(defun_open); break;
            case T_SEMICOLON:   leave(); continue; // so we don't break completely if it's a bitfield or ternary
            } break;

        case defun_open:
            if (tryStatement())
                break;
            switch (kind) {
            case T_RBRACE:      leave(); continue; // always nested in declaration_start
            } break;

        case switch_statement:
        case statement_with_condition:
        case if_statement:
            switch (kind) {
            case T_LPAREN:      enter(condition_open); break;
            } break;

        case maybe_else:
            if (m_currentToken.isComment()) {
                break;
            } else if (kind == T_ELSE) {
                turnInto(else_clause);
                enter(substatement);
                break;
            } else {
                leave(true);
                continue;
            }

        case else_clause:
            // ### shouldn't happen
            dump();
            Q_ASSERT(false);
            leave(true);
            break;

        case do_statement:
            // ### shouldn't happen
            dump();
            Q_ASSERT(false);
            leave(true);
            break;

        case return_statement:
            switch (kind) {
            case T_SEMICOLON:   leave(true); break;
            } break;

        case substatement:
            // prefer substatement_open over block_open
            if (kind != T_LBRACE && tryStatement())
                break;
            switch (kind) {
            case T_LBRACE:      turnInto(substatement_open); break;
            case T_SEMICOLON:   leave(true); break;
            } break;

        case for_statement:
            switch (kind) {
            case T_LPAREN:      enter(for_statement_paren_open); break;
            } break;

        case for_statement_paren_open:
            enter(for_statement_init); continue;

        case for_statement_init:
            switch (kind) {
            case T_SEMICOLON:   turnInto(for_statement_condition); break;
            case T_LPAREN:      enter(condition_paren_open); break;
            } break;

        case for_statement_condition:
            switch (kind) {
            case T_SEMICOLON:   turnInto(for_statement_expression); break;
            case T_LPAREN:      enter(condition_paren_open); break;
            } break;

        case for_statement_expression:
            switch (kind) {
            case T_RPAREN:      leave(); turnInto(substatement); break;
            case T_LPAREN:      enter(condition_paren_open); break;
            } break;

        case case_start:
            switch (kind) {
            case T_COLON:       turnInto(case_cont); break;
            } break;

        case case_cont:
            if (kind != T_CASE && kind != T_DEFAULT && tryStatement())
                break;
            switch (kind) {
            case T_RBRACE:      leave(); continue;
            case T_DEFAULT:
            case T_CASE:        leave(); continue;
            } break;

        case substatement_open:
            if (tryStatement())
                break;
            switch (kind) {
            case T_RBRACE:      leave(true); break;
            } break;

        case condition_open:
            switch (kind) {
            case T_RPAREN:      turnInto(substatement); break;
            case T_LPAREN:      enter(condition_paren_open); break;
            } break;

        case block_open:
            if (tryStatement())
                break;
            switch(kind) {
            case T_RBRACE:      leave(true); break;
            } break;

        // paren nesting
        case condition_paren_open:
            switch (kind) {
            case T_RPAREN:      leave(); break;
            case T_LPAREN:      enter(condition_paren_open); break;
            } break;

        case qt_like_macro:
            switch (kind) {
            case T_LPAREN:      enter(arglist_open); break;
            case T_SEMICOLON:   leave(true); break;
            default:            leave(); continue;
            } break;

        case multiline_comment_start:
        case multiline_comment_cont:
            if (kind != T_COMMENT && kind != T_DOXY_COMMENT) {
                leave();
                continue;
            } else if (m_tokenIndex == m_tokens.size() - 1
                    && lexerState == Lexer::State_Default) {
                leave();
            } else if (m_tokenIndex == 0 && m_currentToken.isComment()) {
                // to allow enter/leave to update the indentDepth
                turnInto(multiline_comment_cont);
            }
            break;

        case cpp_macro_start: {
            const int size = m_currentState.size();

            int previousMarker = -1;
            int previousPreviousMarker = -1;
            for (int i = size - 1; i >= 0; --i) {
                if (m_currentState.at(i).type == cpp_macro_conditional) {
                    if (previousMarker == -1)
                        previousMarker = i;
                    else {
                        previousPreviousMarker = i;
                        break;
                    }
                }
            }

            QStringRef tokenText = currentTokenText();
            if (tokenText == QLatin1String("ifdef")
                    || tokenText == QLatin1String("if")
                    || tokenText == QLatin1String("ifndef")) {
                enter(cpp_macro_conditional);
                // copy everything right of previousMarker, excluding cpp_macro_conditional
                for (int i = previousMarker + 1; i < size; ++i)
                    m_currentState += m_currentState.at(i);
            }
            if (previousMarker != -1) {
                if (tokenText == QLatin1String("endif")) {
                    QStack<State>::iterator begin = m_currentState.begin() + previousPreviousMarker + 1;
                    QStack<State>::iterator end = m_currentState.begin() + previousMarker + 1;
                    m_currentState.erase(begin, end);
                } else if (tokenText == QLatin1String("else")
                        || tokenText == QLatin1String("elif")) {
                    m_currentState.resize(previousMarker + 1);
                    for (int i = previousPreviousMarker + 1; i < previousMarker; ++i)
                        m_currentState += m_currentState.at(i);
                }
            }

            turnInto(cpp_macro);
            break;
        }

        case cpp_macro:
        case cpp_macro_cont:
            break;

        default:
            qWarning() << "Unhandled state" << m_currentState.top().type;
            break;

        } // end of state switch

        ++m_tokenIndex;
    }

    int topState = m_currentState.top().type;

    if (topState != multiline_comment_start
            && topState != multiline_comment_cont
            && (lexerState == Lexer::State_MultiLineComment
                || lexerState == Lexer::State_MultiLineDoxyComment)) {
        enter(multiline_comment_start);
    }

    if (topState == qt_like_macro)
        leave(true);

    if ((topState == cpp_macro_cont
            || topState == cpp_macro) && !endedJoined)
        leave();

    if (topState == cpp_macro && endedJoined)
        turnInto(cpp_macro_cont);

    saveCurrentState(block);
}

int CodeFormatter::indentFor(const QTextBlock &block)
{
//    qDebug() << "indenting for" << block.blockNumber() + 1;

    restoreCurrentState(block.previous());
    correctIndentation(block);
    return m_indentDepth;
}

void CodeFormatter::updateStateUntil(const QTextBlock &endBlock)
{
    QStack<State> previousState = initialState();
    QTextBlock it = endBlock.document()->firstBlock();

    // find the first block that needs recalculation
    for (; it.isValid() && it != endBlock; it = it.next()) {
        BlockData blockData;
        if (!loadBlockData(it, &blockData))
            break;
        if (blockData.m_blockRevision != it.revision())
            break;
        if (previousState.isEmpty() || blockData.m_beginState.isEmpty()
                || previousState != blockData.m_beginState)
            break;
        if (loadLexerState(it) == -1)
            break;

        previousState = blockData.m_endState;
    }

    if (it == endBlock)
        return;

    // update everthing until endBlock
    for (; it.isValid() && it != endBlock; it = it.next()) {
        recalculateStateAfter(it);
    }

    // invalidate everything below by marking the state in endBlock as invalid
    if (it.isValid()) {
        BlockData invalidBlockData;
        saveBlockData(&it, invalidBlockData);
    }
}

void CodeFormatter::updateLineStateChange(const QTextBlock &block)
{
    if (!block.isValid())
        return;

    BlockData blockData;
    if (loadBlockData(block, &blockData) && blockData.m_blockRevision == block.revision())
        return;

    recalculateStateAfter(block);

    // invalidate everything below by marking the next block's state as invalid
    QTextBlock next = block.next();
    if (!next.isValid())
        return;

    saveBlockData(&next, BlockData());
}

CodeFormatter::State CodeFormatter::state(int belowTop) const
{
    if (belowTop < m_currentState.size())
        return m_currentState.at(m_currentState.size() - 1 - belowTop);
    else
        return State();
}

const QVector<CodeFormatter::State> &CodeFormatter::newStatesThisLine() const
{
    return m_newStates;
}

int CodeFormatter::tokenIndex() const
{
    return m_tokenIndex;
}

int CodeFormatter::tokenCount() const
{
    return m_tokens.size();
}

const CPlusPlus::Token &CodeFormatter::currentToken() const
{
    return m_currentToken;
}

void CodeFormatter::invalidateCache(QTextDocument *document)
{
    if (!document)
        return;

    BlockData invalidBlockData;
    QTextBlock it = document->firstBlock();
    for (; it.isValid(); it = it.next()) {
        saveBlockData(&it, invalidBlockData);
    }
}

void CodeFormatter::enter(int newState)
{
    int savedIndentDepth = m_indentDepth;
    onEnter(newState, &m_indentDepth, &savedIndentDepth);
    State s(newState, savedIndentDepth);
    m_currentState.push(s);
    m_newStates.push(s);
}

void CodeFormatter::leave(bool statementDone)
{
    Q_ASSERT(m_currentState.size() > 1);
    if (m_currentState.top().type == topmost_intro)
        return;

    if (m_newStates.size() > 0)
        m_newStates.pop();

    // restore indent depth
    State poppedState = m_currentState.pop();
    m_indentDepth = poppedState.savedIndentDepth;

    int topState = m_currentState.top().type;

    // does it suffice to check if token is T_SEMICOLON or T_RBRACE?
    // maybe distinction between leave and turnInto?
    if (statementDone) {
        if (topState == substatement
                || topState == statement_with_condition
                || topState == for_statement
                || topState == switch_statement
                || topState == do_statement) {
            leave(true);
        } else if (topState == if_statement) {
            if (poppedState.type != maybe_else)
                enter(maybe_else);
            else
                leave(true);
        } else if (topState == else_clause) {
            // leave the else *and* the surrounding if, to prevent another else
            leave();
            leave(true);
        }
    }
}

void CodeFormatter::correctIndentation(const QTextBlock &block)
{
    const int lexerState = tokenizeBlock(block);
    Q_ASSERT(m_currentState.size() >= 1);

    adjustIndent(m_tokens, lexerState, &m_indentDepth);
}

bool CodeFormatter::tryExpression(bool alsoExpression)
{    
    int newState = -1;

    const int kind = m_currentToken.kind();
    switch (kind) {
    case T_LPAREN:          newState = arglist_open; break;
    case T_QUESTION:        newState = ternary_op; break;

    case T_LESS_LESS:
    case T_GREATER_GREATER:
        newState = stream_op;
        for (int i = m_currentState.size() - 1; i >= 0; --i) {
            const int type = m_currentState.at(i).type;
            if (type == arglist_open) { // likely a left-shift instead
                newState = -1;
                break;
            }
            if (type == topmost_intro
                    || type == substatement_open
                    || type == defun_open
                    || type == namespace_open
                    || type == class_open
                    || type == brace_list_open) {
                break;
            }
        }
        break;
    }

    if (newState != -1) {
        if (alsoExpression)
            enter(expression);
        enter(newState);
        return true;
    }

    return false;
}

bool CodeFormatter::tryDeclaration()
{
    const int kind = m_currentToken.kind();
    switch (kind) {
    case T_Q_ENUMS:
    case T_Q_PROPERTY:
    case T_Q_FLAGS:
    case T_Q_GADGET:
    case T_Q_OBJECT:
    case T_Q_INTERFACES:
    case T_Q_DECLARE_INTERFACE:
    case T_Q_PRIVATE_SLOT:
        enter(qt_like_macro);
        return true;
    case T_IDENTIFIER:
        if (m_tokenIndex == 0) {
            QString tokenText = currentTokenText().toString();
            if (tokenText.startsWith(QLatin1String("Q_"))
                    || tokenText.startsWith(QLatin1String("QT_"))
                    || tokenText.startsWith(QLatin1String("QDOC_"))) {
                enter(qt_like_macro);
                return true;
            }
        }
        // fallthrough
    case T_CHAR:
    case T_WCHAR_T:
    case T_BOOL:
    case T_SHORT:
    case T_INT:
    case T_LONG:
    case T_SIGNED:
    case T_UNSIGNED:
    case T_FLOAT:
    case T_DOUBLE:
    case T_VOID:
    case T_AUTO:
    case T___TYPEOF__:
    case T___ATTRIBUTE__:
    case T_STATIC:
    case T_FRIEND:
    case T_CONST:
    case T_VOLATILE:
    case T_INLINE:
        enter(declaration_start);
        return true;

    case T_TEMPLATE:
        enter(template_start);
        return true;

    case T_NAMESPACE:
        enter(namespace_start);
        return true;

    case T_STRUCT:
    case T_UNION:
    case T_CLASS:
        enter(class_start);
        return true;

    case T_ENUM:
        enter(enum_start);
        return true;

    case T_USING:
        enter(using_start);
        return true;

    default:
        return false;
    }
}

bool CodeFormatter::tryStatement()
{
    const int kind = m_currentToken.kind();
    if (tryDeclaration())
        return true;
    switch (kind) {
    case T_RETURN:
        enter(return_statement);
        enter(expression);
        return true;
    case T_FOR:
        enter(for_statement);
        return true;
    case T_SWITCH:
        enter(switch_statement);
        return true;
    case T_IF:
        enter(if_statement);
        return true;
    case T_WHILE:
    case T_Q_FOREACH:
        enter(statement_with_condition);
        return true;
    case T_DO:
        enter(do_statement);
        enter(substatement);
        return true;
    case T_CASE:
    case T_DEFAULT:
        enter(case_start);
        return true;
    case T_LBRACE:
        enter(block_open);
        return true;
    default:
        return false;
    }
}

bool CodeFormatter::isBracelessState(int type) const
{
    return type == substatement
        || type == if_statement
        || type == else_clause
        || type == statement_with_condition
        || type == for_statement
        || type == do_statement;
}

const Token &CodeFormatter::tokenAt(int idx) const
{
    static const Token empty;
    if (idx < 0 || idx >= m_tokens.size())
        return empty;
    else
        return m_tokens.at(idx);
}

int CodeFormatter::column(int index) const
{
    int col = 0;
    if (index > m_currentLine.length())
        index = m_currentLine.length();

    const QChar tab = QLatin1Char('\t');

    for (int i = 0; i < index; i++) {
        if (m_currentLine[i] == tab) {
            col = ((col / m_tabSize) + 1) * m_tabSize;
        } else {
            col++;
        }
    }
    return col;
}

QStringRef CodeFormatter::currentTokenText() const
{
    return m_currentLine.midRef(m_currentToken.begin(), m_currentToken.length());
}

void CodeFormatter::turnInto(int newState)
{
    leave(false);
    enter(newState);
}

void CodeFormatter::saveCurrentState(const QTextBlock &block)
{
    if (!block.isValid())
        return;

    BlockData blockData;
    blockData.m_blockRevision = block.revision();
    blockData.m_beginState = m_beginState;
    blockData.m_endState = m_currentState;
    blockData.m_indentDepth = m_indentDepth;

    QTextBlock saveableBlock(block);
    saveBlockData(&saveableBlock, blockData);
}

void CodeFormatter::restoreCurrentState(const QTextBlock &block)
{
    if (block.isValid()) {             
        BlockData blockData;
        if (loadBlockData(block, &blockData)) {
            m_indentDepth = blockData.m_indentDepth;
            m_currentState = blockData.m_endState;
            m_beginState = m_currentState;
            return;
        }
    }

    m_currentState = initialState();
    m_beginState = m_currentState;
    m_indentDepth = 0;
}

QStack<CodeFormatter::State> CodeFormatter::initialState()
{
    static QStack<CodeFormatter::State> initialState;
    if (initialState.isEmpty())
        initialState.push(State(topmost_intro, 0));
    return initialState;
}

int CodeFormatter::tokenizeBlock(const QTextBlock &block, bool *endedJoined)
{
    int startState = loadLexerState(block.previous());
    if (block.blockNumber() == 0)
        startState = 0;
    Q_ASSERT(startState != -1);

    SimpleLexer tokenize;
    tokenize.setQtMocRunEnabled(true);
    tokenize.setObjCEnabled(true);

    m_currentLine = block.text();
    // to determine whether a line was joined, Tokenizer needs a
    // newline character at the end
    m_currentLine.append(QLatin1Char('\n'));
    m_tokens = tokenize(m_currentLine, startState);

    if (endedJoined)
        *endedJoined = tokenize.endedJoined();

    const int lexerState = tokenize.state();
    BaseTextDocumentLayout::setLexerState(block, lexerState);
    return lexerState;
}

void CodeFormatter::dump()
{
    qDebug() << "Current token index" << m_tokenIndex;
    qDebug() << "Current state:";
    foreach (State s, m_currentState) {
        qDebug() << s.type << s.savedIndentDepth;
    }
    qDebug() << "Current indent depth:" << m_indentDepth;
}


namespace CppTools {
namespace Internal {
    class CppCodeFormatterData: public TextEditor::CodeFormatterData
    {
    public:
        CodeFormatter::BlockData m_data;
    };
}
}

QtStyleCodeFormatter::QtStyleCodeFormatter()
    : m_indentSize(4)
    , m_indentSubstatementBraces(false)
    , m_indentSubstatementStatements(true)
    , m_indentDeclarationBraces(false)
    , m_indentDeclarationMembers(true)
{
}

void QtStyleCodeFormatter::setIndentSize(int size)
{
    m_indentSize = size;
}

void QtStyleCodeFormatter::setIndentSubstatementBraces(bool onOff)
{
    m_indentSubstatementBraces = onOff;
}

void QtStyleCodeFormatter::setIndentSubstatementStatements(bool onOff)
{
    m_indentSubstatementStatements = onOff;
}

void QtStyleCodeFormatter::setIndentDeclarationBraces(bool onOff)
{
    m_indentDeclarationBraces = onOff;
}

void QtStyleCodeFormatter::setIndentDeclarationMembers(bool onOff)
{
    m_indentDeclarationMembers = onOff;
}

void QtStyleCodeFormatter::saveBlockData(QTextBlock *block, const BlockData &data) const
{
    TextBlockUserData *userData = BaseTextDocumentLayout::userData(*block);
    CppCodeFormatterData *cppData = static_cast<CppCodeFormatterData *>(userData->codeFormatterData());
    if (!cppData) {
        cppData = new CppCodeFormatterData;
        userData->setCodeFormatterData(cppData);
    }
    cppData->m_data = data;
}

bool QtStyleCodeFormatter::loadBlockData(const QTextBlock &block, BlockData *data) const
{
    TextBlockUserData *userData = BaseTextDocumentLayout::testUserData(block);
    if (!userData)
        return false;
    CppCodeFormatterData *cppData = static_cast<CppCodeFormatterData *>(userData->codeFormatterData());
    if (!cppData)
        return false;

    *data = cppData->m_data;
    return true;
}

void QtStyleCodeFormatter::saveLexerState(QTextBlock *block, int state) const
{
    BaseTextDocumentLayout::setLexerState(*block, state);
}

int QtStyleCodeFormatter::loadLexerState(const QTextBlock &block) const
{
    return BaseTextDocumentLayout::lexerState(block);
}

void QtStyleCodeFormatter::onEnter(int newState, int *indentDepth, int *savedIndentDepth) const
{
    const State &parentState = state();
    const Token &tk = currentToken();
    const int tokenPosition = column(tk.begin());
    const bool firstToken = (tokenIndex() == 0);
    const bool lastToken = (tokenIndex() == tokenCount() - 1);

    switch (newState) {
    case namespace_start:
        if (firstToken)
            *savedIndentDepth = tokenPosition;
        *indentDepth = tokenPosition;
        break;

    case enum_start:
    case class_start:
        if (firstToken)
            *savedIndentDepth = tokenPosition;
        *indentDepth = tokenPosition + 2*m_indentSize;
        break;

    case template_param:
        if (!lastToken)
            *indentDepth = tokenPosition + tk.length();
        else
            *indentDepth += 2*m_indentSize;
        break;

    case statement_with_condition:
    case for_statement:
    case switch_statement:
    case if_statement:
    case return_statement:
        if (firstToken)
            *savedIndentDepth = tokenPosition;
        *indentDepth = *savedIndentDepth + 2*m_indentSize;
        break;

    case declaration_start:
        if (firstToken)
            *savedIndentDepth = tokenPosition;
        // continuation indent in function bodies only, to not indent
        // after the return type in "void\nfoo() {}"
        for (int i = 0; state(i).type != topmost_intro; ++i) {
            if (state(i).type == defun_open) {
                *indentDepth = *savedIndentDepth + 2*m_indentSize;
                break;
            }
        }
        break;

    case arglist_open:
    case condition_paren_open:
        if (!lastToken)
            *indentDepth = tokenPosition + 1;
        else
            *indentDepth += m_indentSize;
        break;

    case ternary_op:
        if (!lastToken)
            *indentDepth = tokenPosition + tk.length() + 1;
        else
            *indentDepth += m_indentSize;
        break;

    case stream_op:
        *indentDepth = tokenPosition + tk.length() + 1;
        break;
    case stream_op_cont:
        if (firstToken)
            *savedIndentDepth = *indentDepth = tokenPosition + tk.length() + 1;
        break;

    case member_init_open:
        // undo the continuation indent of the parent
        *savedIndentDepth = parentState.savedIndentDepth;

        if (firstToken)
            *indentDepth = tokenPosition + tk.length() + 1;
        else
            *indentDepth = *savedIndentDepth + m_indentSize;
        break;

    case case_cont:
        *indentDepth += m_indentSize;
        break;

    case class_open:
    case enum_open:
    case defun_open: {
        // undo the continuation indent of the parent
        *savedIndentDepth = parentState.savedIndentDepth;

        bool followedByData = (!lastToken && !tokenAt(tokenIndex() + 1).isComment());
        if (firstToken || followedByData)
            *savedIndentDepth = tokenPosition;

        *indentDepth = *savedIndentDepth;

        if (followedByData) {
            *indentDepth = column(tokenAt(tokenIndex() + 1).begin());
        } else if (m_indentDeclarationMembers) {
            *indentDepth += m_indentSize;
        }
        break;
    }

    case substatement_open:
        if (firstToken) {
            *savedIndentDepth = tokenPosition;
            *indentDepth = *savedIndentDepth;
        } else if (m_indentSubstatementBraces && !m_indentSubstatementStatements) {
            // ### The preceding check is quite arbitrary.
            // It actually needs another flag to determine whether the closing curly
            // should be indented or not
            *indentDepth = *savedIndentDepth += m_indentSize;
        }

        if (m_indentSubstatementStatements) {
            if (parentState.type != switch_statement)
                *indentDepth += m_indentSize;
        }
        break;

    case brace_list_open:
        if (parentState.type != initializer)
            *indentDepth = parentState.savedIndentDepth + m_indentSize;
        else if (lastToken) {
            *savedIndentDepth = state(1).savedIndentDepth;
            *indentDepth = *savedIndentDepth + m_indentSize;
        }
        break;

    case block_open:
        if (parentState.type != case_cont)
            *indentDepth += m_indentSize;
        break;

    case condition_open:
        // undo the continuation indent of the parent
        *indentDepth = parentState.savedIndentDepth;
        *savedIndentDepth = *indentDepth;

        // fixed extra indent when continuing 'if (', but not for 'else if ('
        if (tokenPosition <= *indentDepth + m_indentSize)
            *indentDepth += 2*m_indentSize;
        else
            *indentDepth = tokenPosition + 1;
        break;

    case substatement:
        // undo the continuation indent of the parent
        *indentDepth = parentState.savedIndentDepth;
        *savedIndentDepth = *indentDepth;
        break;

    case maybe_else: {
        // set indent to outermost braceless savedIndent
        int outermostBraceless = 0;
        while (isBracelessState(state(outermostBraceless).type))
            ++outermostBraceless;
        *indentDepth = state(outermostBraceless - 1).savedIndentDepth;
        // this is where the else should go, if one appears - aligned to if_statement
        *savedIndentDepth = state().savedIndentDepth;
    }   break;

    case for_statement_paren_open:
        *indentDepth = tokenPosition + 1;
        break;

    case multiline_comment_start:
        *indentDepth = tokenPosition + 2;
        break;

    case multiline_comment_cont:
        *indentDepth = tokenPosition;
        break;

    case cpp_macro:
    case cpp_macro_cont:
        *indentDepth = m_indentSize;
        break;
    }
}

void QtStyleCodeFormatter::adjustIndent(const QList<CPlusPlus::Token> &tokens, int lexerState, int *indentDepth) const
{
    State topState = state();
    State previousState = state(1);

    const bool topWasMaybeElse = (topState.type == maybe_else);
    if (topWasMaybeElse) {
        int outermostBraceless = 1;
        while (state(outermostBraceless).type != invalid && isBracelessState(state(outermostBraceless).type))
            ++outermostBraceless;

        topState = state(outermostBraceless);
        previousState = state(outermostBraceless + 1);
    }


    // adjusting the indentDepth here instead of in enter() gives 'else if' the correct indentation
    // ### could be moved?
    if (topState.type == substatement)
        *indentDepth += m_indentSize;

    // keep user-adjusted indent in multiline comments
    if (topState.type == multiline_comment_start
            || topState.type == multiline_comment_cont) {
        if (!tokens.isEmpty()) {
            *indentDepth = tokens.at(0).begin();
            return;
        }
    }

    const int kind = tokenAt(0).kind();
    switch (kind) {
    case T_POUND: *indentDepth = 0; break;
    case T_COLON:
        // ### ok for constructor initializer lists - what about ? and bitfields?
        if (topState.type == expression && previousState.type == declaration_start) {
            *indentDepth = previousState.savedIndentDepth + m_indentSize;
        } else if (topState.type == ternary_op) {
            *indentDepth -= 2;
        }
        break;
    case T_COMMA:
        if (topState.type == member_init_open) {
            *indentDepth -= 2;
        }
        break;
    case T_LBRACE: {
        if (topState.type == case_cont) {
            *indentDepth = topState.savedIndentDepth;
        // function definition - argument list is expression state
        } else if (topState.type == expression && previousState.type == declaration_start) {
            *indentDepth = previousState.savedIndentDepth;
            if (m_indentDeclarationBraces)
                *indentDepth += m_indentSize;
        } else if (topState.type == class_start) {
            *indentDepth = topState.savedIndentDepth;
            if (m_indentDeclarationBraces)
                *indentDepth += m_indentSize;
        } else if (topState.type == substatement) {
            *indentDepth = topState.savedIndentDepth;
            if (m_indentSubstatementBraces)
                *indentDepth += m_indentSize;
        } else if (topState.type != defun_open
                && topState.type != block_open
                && !topWasMaybeElse) {
            *indentDepth = topState.savedIndentDepth;
        }

        break;
    }
    case T_RBRACE: {
        if (topState.type == block_open && previousState.type == case_cont) {
            *indentDepth = previousState.savedIndentDepth;
            break;
        }
        for (int i = 0; state(i).type != topmost_intro; ++i) {
            const int type = state(i).type;
            if (type == defun_open
                    || type == substatement_open
                    || type == class_open
                    || type == brace_list_open
                    || type == namespace_open
                    || type == block_open
                    || type == enum_open) {
                *indentDepth = state(i).savedIndentDepth;
                break;
            }
        }
        break;
    }
    // Disabled for now, see QTCREATORBUG-1825. It makes extending if conditions
    // awkward: inserting a newline just before the ) shouldn't align to 'if'.
    //case T_RPAREN:
    //    if (topState.type == condition_open) {
    //        *indentDepth = previousState.savedIndentDepth;
    //    }
    //    break;
    case T_DEFAULT:
    case T_CASE:
        for (int i = 0; state(i).type != topmost_intro; ++i) {
            const int type = state(i).type;
            if (type == switch_statement || type == case_cont) {
                *indentDepth = state(i).savedIndentDepth;
                break;
            } else if (type == topmost_intro) {
                break;
            }
        }
        break;
    case T_PUBLIC:
    case T_PRIVATE:
    case T_PROTECTED:
    case T_Q_SIGNALS:
        if (topState.type == class_open) {
            if (tokenAt(1).is(T_COLON) || tokenAt(2).is(T_COLON))
                *indentDepth = topState.savedIndentDepth;
        }
        break;
    case T_ELSE:
        if (topWasMaybeElse)
            *indentDepth = state().savedIndentDepth; // topSavedIndent is actually the previous
        break;
    case T_LESS_LESS:
    case T_GREATER_GREATER:
        if (topState.type == stream_op || topState.type == stream_op_cont)
            *indentDepth -= 3; // to align << with <<
        break;
    case T_COMMENT:
    case T_DOXY_COMMENT:
    case T_CPP_COMMENT:
    case T_CPP_DOXY_COMMENT:
        // unindent the last line of a comment
        if ((topState.type == multiline_comment_cont
             || topState.type == multiline_comment_start)
                && (kind == T_COMMENT || kind == T_DOXY_COMMENT)
                && (lexerState == Lexer::State_Default
                    || tokens.size() != 1)) {
            *indentDepth -= m_indentSize;
        }
        break;
    }
}

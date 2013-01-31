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

#include "qmljscodeformatter.h"

#include <QDebug>
#include <QMetaEnum>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>

using namespace QmlJS;

CodeFormatter::BlockData::BlockData()
    : m_indentDepth(0)
    , m_blockRevision(-1)
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

    const int lexerState = tokenizeBlock(block);
    m_tokenIndex = 0;
    m_newStates.clear();

    //qDebug() << "Starting to look at " << block.text() << block.blockNumber() + 1;

    for (; m_tokenIndex < m_tokens.size(); ) {
        m_currentToken = tokenAt(m_tokenIndex);
        const int kind = extendedTokenKind(m_currentToken);

        //qDebug() << "Token" << m_currentLine.mid(m_currentToken.begin(), m_currentToken.length) << m_tokenIndex << "in line" << block.blockNumber() + 1;
        //dump();

        if (kind == Comment
                && state().type != multiline_comment_cont
                && state().type != multiline_comment_start) {
            m_tokenIndex += 1;
            continue;
        }

        switch (m_currentState.top().type) {
        case topmost_intro:
            switch (kind) {
            case Identifier:    enter(objectdefinition_or_js); continue;
            case Import:        enter(top_qml); continue;
            case LeftBrace:     enter(top_js); enter(expression); continue; // if a file starts with {, it's likely json
            default:            enter(top_js); continue;
            } break;

        case top_qml:
            switch (kind) {
            case Import:        enter(import_start); break;
            case Identifier:    enter(binding_or_objectdefinition); break;
            } break;

        case top_js:
            tryStatement();
            break;

        case objectdefinition_or_js:
            switch (kind) {
            case Dot:           break;
            case Identifier:
                if (!m_currentLine.at(m_currentToken.begin()).isUpper()) {
                    turnInto(top_js);
                    continue;
                }
                break;
            case LeftBrace:     turnInto(binding_or_objectdefinition); continue;
            default:            turnInto(top_js); continue;
            } break;

        case import_start:
            enter(import_maybe_dot_or_version_or_as);
            break;

        case import_maybe_dot_or_version_or_as:
            switch (kind) {
            case Dot:           turnInto(import_dot); break;
            case As:            turnInto(import_as); break;
            case Number:        turnInto(import_maybe_as); break;
            default:            leave(); leave(); continue;
            } break;

        case import_maybe_as:
            switch (kind) {
            case As:            turnInto(import_as); break;
            default:            leave(); leave(); continue;
            } break;

        case import_dot:
            switch (kind) {
            case Identifier:    turnInto(import_maybe_dot_or_version_or_as); break;
            default:            leave(); leave(); continue;
            } break;

        case import_as:
            switch (kind) {
            case Identifier:    leave(); leave(); break;
            } break;

        case binding_or_objectdefinition:
            switch (kind) {
            case Colon:         enter(binding_assignment); break;
            case LeftBrace:     enter(objectdefinition_open); break;
            } break;

        case binding_assignment:
            switch (kind) {
            case Semicolon:     leave(true); break;
            case If:            enter(if_statement); break;
            case With:          enter(statement_with_condition); break;
            case Try:           enter(try_statement); break;
            case Switch:        enter(switch_statement); break;
            case LeftBrace:     enter(jsblock_open); break;
            case On:
            case As:
            case List:
            case Import:
            case Signal:
            case Property:
            case Identifier:    enter(expression_or_objectdefinition); break;

            // error recovery
            case RightBracket:
            case RightParenthesis:  leave(true); break;

            default:            enter(expression); continue;
            } break;

        case objectdefinition_open:
            switch (kind) {
            case RightBrace:    leave(true); break;
            case Default:       enter(default_property_start); break;
            case Property:      enter(property_start); break;
            case Function:      enter(function_start); break;
            case Signal:        enter(signal_start); break;
            case On:
            case As:
            case List:
            case Import:
            case Identifier:    enter(binding_or_objectdefinition); break;
            } break;

        case default_property_start:
            if (kind != Property)
                leave(true);
            else
                turnInto(property_start);
            break;

        case property_start:
            switch (kind) {
            case Colon:         enter(binding_assignment); break; // oops, was a binding
            case Var:
            case Identifier:    enter(property_name); break;
            case List:          enter(property_list_open); break;
            default:            leave(true); continue;
            } break;

        case property_name:
            turnInto(property_maybe_initializer);
            break;

        case property_list_open:
            if (m_currentLine.midRef(m_currentToken.begin(), m_currentToken.length) == QLatin1String(">"))
                turnInto(property_name);
            break;

        case property_maybe_initializer:
            switch (kind) {
            case Colon:         turnInto(binding_assignment); break;
            default:            leave(true); continue;
            } break;

        case signal_start:
            switch (kind) {
            case Colon:         enter(binding_assignment); break; // oops, was a binding
            default:            enter(signal_maybe_arglist); break;
            } break;

        case signal_maybe_arglist:
            switch (kind) {
            case LeftParenthesis:   turnInto(signal_arglist_open); break;
            default:                leave(true); continue;
            } break;

        case signal_arglist_open:
            switch (kind) {
            case RightParenthesis:  leave(true); break;
            } break;

        case function_start:
            switch (kind) {
            case LeftParenthesis:   enter(function_arglist_open); break;
            } break;

        case function_arglist_open:
            switch (kind) {
            case RightParenthesis:  turnInto(function_arglist_closed); break;
            } break;

        case function_arglist_closed:
            switch (kind) {
            case LeftBrace:         turnInto(jsblock_open); break;
            default:                leave(true); continue; // error recovery
            } break;

        case expression_or_objectdefinition:
            switch (kind) {
            case Dot:
            case Identifier:        break; // need to become an objectdefinition_open in cases like "width: Qt.Foo {"
            case LeftBrace:         turnInto(objectdefinition_open); break;

            // propagate 'leave' from expression state
            case RightBracket:
            case RightParenthesis:  leave(); continue;

            default:                enter(expression); continue; // really? identifier and more tokens might already be gone
            } break;

        case expression_or_label:
            switch (kind) {
            case Colon:             turnInto(labelled_statement); break;

            // propagate 'leave' from expression state
            case RightBracket:
            case RightParenthesis:  leave(); continue;

            default:                enter(expression); continue;
            } break;

        case ternary_op:
            if (kind == Colon) {
                enter(ternary_op_after_colon);
                enter(expression_continuation);
                break;
            }
            // fallthrough
        case ternary_op_after_colon:
        case expression:
            if (tryInsideExpression())
                break;
            switch (kind) {
            case Comma:
            case Delimiter:         enter(expression_continuation); break;
            case RightBracket:
            case RightParenthesis:  leave(); continue;
            case RightBrace:        leave(true); continue;
            case Semicolon:         leave(true); break;
            } break;

        case expression_continuation:
            leave();
            continue;

        case expression_maybe_continuation:
            switch (kind) {
            case Question:
            case Delimiter:
            case LeftBracket:
            case LeftParenthesis:  leave(); continue;
            default:               leave(true); continue;
            } break;

        case paren_open:
            if (tryInsideExpression())
                break;
            switch (kind) {
            case RightParenthesis:  leave(); break;
            } break;

        case bracket_open:
            if (tryInsideExpression())
                break;
            switch (kind) {
            case Comma:             enter(bracket_element_start); break;
            case RightBracket:      leave(); break;
            } break;

        case objectliteral_open:
            if (tryInsideExpression())
                break;
            switch (kind) {
            case Colon:             enter(objectliteral_assignment); break;
            case RightBracket:
            case RightParenthesis:  leave(); continue; // error recovery
            case RightBrace:        leave(true); break;
            } break;

        // pretty much like expression, but ends with , or }
        case objectliteral_assignment:
            if (tryInsideExpression())
                break;
            switch (kind) {
            case Delimiter:         enter(expression_continuation); break;
            case RightBracket:
            case RightParenthesis:  leave(); continue; // error recovery
            case RightBrace:        leave(); continue; // so we also leave objectliteral_open
            case Comma:             leave(); break;
            } break;

        case bracket_element_start:
            switch (kind) {
            case Identifier:        turnInto(bracket_element_maybe_objectdefinition); break;
            default:                leave(); continue;
            } break;

        case bracket_element_maybe_objectdefinition:
            switch (kind) {
            case LeftBrace:         turnInto(objectdefinition_open); break;
            default:                leave(); continue;
            } break;

        case jsblock_open:
        case substatement_open:
            if (tryStatement())
                break;
            switch (kind) {
            case RightBrace:        leave(true); break;
            } break;

        case labelled_statement:
            if (tryStatement())
                break;
            leave(true); // error recovery
            break;

        case substatement:
            // prefer substatement_open over block_open
            if (kind != LeftBrace) {
                if (tryStatement())
                    break;
            }
            switch (kind) {
            case LeftBrace:         turnInto(substatement_open); break;
            } break;

        case if_statement:
            switch (kind) {
            case LeftParenthesis:   enter(condition_open); break;
            default:                leave(true); break; // error recovery
            } break;

        case maybe_else:
            switch (kind) {
            case Else:              turnInto(else_clause); enter(substatement); break;
            default:                leave(true); continue;
            } break;

        case maybe_catch_or_finally:
            switch (kind) {
            case Catch:             turnInto(catch_statement); break;
            case Finally:           turnInto(finally_statement); break;
            default:                leave(true); continue;
            } break;

        case else_clause:
            // ### shouldn't happen
            dump();
            Q_ASSERT(false);
            leave(true);
            break;

        case condition_open:
            if (tryInsideExpression())
                break;
            switch (kind) {
            case RightParenthesis:  turnInto(substatement); break;
            } break;

        case switch_statement:
        case catch_statement:
        case statement_with_condition:
            switch (kind) {
            case LeftParenthesis:   enter(statement_with_condition_paren_open); break;
            default:                leave(true);
            } break;

        case statement_with_condition_paren_open:
            if (tryInsideExpression())
                break;
            switch (kind) {
            case RightParenthesis:  turnInto(substatement); break;
            } break;

        case try_statement:
        case finally_statement:
            switch (kind) {
            case LeftBrace:         enter(jsblock_open); break;
            default:                leave(true); break;
            } break;

        case do_statement:
            switch (kind) {
            case While:             break;
            case LeftParenthesis:   enter(do_statement_while_paren_open); break;
            default:                leave(true); continue; // error recovery
            } break;

        case do_statement_while_paren_open:
            if (tryInsideExpression())
                break;
            switch (kind) {
            case RightParenthesis:  leave(); leave(true); break;
            } break;

        case breakcontinue_statement:
            switch (kind) {
            case Identifier:        leave(true); break;
            default:                leave(true); continue; // try again
            } break;

        case case_start:
            switch (kind) {
            case Colon:             turnInto(case_cont); break;
            } break;

        case case_cont:
            if (kind != Case && kind != Default && tryStatement())
                break;
            switch (kind) {
            case RightBrace:        leave(); continue;
            case Default:
            case Case:              leave(); continue;
            } break;

        case multiline_comment_start:
        case multiline_comment_cont:
            if (kind != Comment) {
                leave();
                continue;
            } else if (m_tokenIndex == m_tokens.size() - 1
                       && (lexerState & Scanner::MultiLineMask) == Scanner::Normal) {
                leave();
            } else if (m_tokenIndex == 0) {
                // to allow enter/leave to update the indentDepth
                turnInto(multiline_comment_cont);
            }
            break;

        default:
            qWarning() << "Unhandled state" << m_currentState.top().type;
            break;
        } // end of state switch

        ++m_tokenIndex;
    }

    int topState = m_currentState.top().type;

    // if there's no colon on the same line, it's not a label
    if (topState == expression_or_label)
        enter(expression);
    // if not followed by an identifier on the same line, it's done
    else if (topState == breakcontinue_statement)
        leave(true);

    topState = m_currentState.top().type;

    // some states might be continued on the next line
    if (topState == expression
            || topState == expression_or_objectdefinition
            || topState == objectliteral_assignment
            || topState == ternary_op_after_colon) {
        enter(expression_maybe_continuation);
    }
    // multi-line comment start?
    if (topState != multiline_comment_start
            && topState != multiline_comment_cont
            && (lexerState & Scanner::MultiLineMask) == Scanner::MultiLineComment) {
        enter(multiline_comment_start);
    }

    saveCurrentState(block);
}

int CodeFormatter::indentFor(const QTextBlock &block)
{
//    qDebug() << "indenting for" << block.blockNumber() + 1;

    restoreCurrentState(block.previous());
    correctIndentation(block);
    return m_indentDepth;
}

int CodeFormatter::indentForNewLineAfter(const QTextBlock &block)
{
    restoreCurrentState(block);

    m_tokens.clear();
    m_currentLine.clear();
    const int startLexerState = loadLexerState(block.previous());
    adjustIndent(m_tokens, startLexerState, &m_indentDepth);

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
        if (previousState != blockData.m_beginState)
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

const Token &CodeFormatter::currentToken() const
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

    //qDebug() << "enter state" << stateToString(newState);

    if (newState == bracket_open)
        enter(bracket_element_start);
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

    //qDebug() << "left state" << stateToString(poppedState.type) << ", now in state" << stateToString(topState);

    // if statement is done, may need to leave recursively
    if (statementDone) {
        if (topState == if_statement) {
            if (poppedState.type != maybe_else)
                enter(maybe_else);
            else
                leave(true);
        } else if (topState == else_clause) {
            // leave the else *and* the surrounding if, to prevent another else
            leave();
            leave(true);
        } else if (topState == try_statement) {
            if (poppedState.type != maybe_catch_or_finally
                    && poppedState.type != finally_statement) {
                enter(maybe_catch_or_finally);
            } else {
                leave(true);
            }
        } else if (!isExpressionEndState(topState)) {
            leave(true);
        }
    }
}

void CodeFormatter::correctIndentation(const QTextBlock &block)
{
    tokenizeBlock(block);
    Q_ASSERT(m_currentState.size() >= 1);

    const int startLexerState = loadLexerState(block.previous());
    adjustIndent(m_tokens, startLexerState, &m_indentDepth);
}

bool CodeFormatter::tryInsideExpression(bool alsoExpression)
{
    int newState = -1;
    const int kind = extendedTokenKind(m_currentToken);
    switch (kind) {
    case LeftParenthesis:   newState = paren_open; break;
    case LeftBracket:       newState = bracket_open; break;
    case LeftBrace:         newState = objectliteral_open; break;
    case Function:          newState = function_start; break;
    case Question:          newState = ternary_op; break;
    }

    if (newState != -1) {
        if (alsoExpression)
            enter(expression);
        enter(newState);
        return true;
    }

    return false;
}

bool CodeFormatter::tryStatement()
{
    const int kind = extendedTokenKind(m_currentToken);
    switch (kind) {
    case Semicolon:
        enter(empty_statement);
        leave(true);
        return true;
    case Break:
    case Continue:
        enter(breakcontinue_statement);
        return true;
    case Throw:
        enter(throw_statement);
        enter(expression);
        return true;
    case Return:
        enter(return_statement);
        enter(expression);
        return true;
    case While:
    case For:
    case Catch:
        enter(statement_with_condition);
        return true;
    case Switch:
        enter(switch_statement);
        return true;
    case If:
        enter(if_statement);
        return true;
    case Do:
        enter(do_statement);
        enter(substatement);
        return true;
    case Case:
    case Default:
        enter(case_start);
        return true;
    case Try:
        enter(try_statement);
        return true;
    case LeftBrace:
        enter(jsblock_open);
        return true;
    case Identifier:
        enter(expression_or_label);
        return true;
    case Delimiter:
    case Var:
    case PlusPlus:
    case MinusMinus:
    case Import:
    case Signal:
    case On:
    case As:
    case List:
    case Property:
    case Function:
    case Number:
    case String:
    case LeftParenthesis:
        enter(expression);
        // look at the token again
        m_tokenIndex -= 1;
        return true;
    }
    return false;
}

bool CodeFormatter::isBracelessState(int type) const
{
    return
            type == if_statement ||
            type == else_clause ||
            type == substatement ||
            type == binding_assignment ||
            type == binding_or_objectdefinition;
}

bool CodeFormatter::isExpressionEndState(int type) const
{
    return
            type == topmost_intro ||
            type == top_js ||
            type == objectdefinition_open ||
            type == do_statement ||
            type == jsblock_open ||
            type == substatement_open ||
            type == bracket_open ||
            type == paren_open ||
            type == case_cont ||
            type == objectliteral_open;
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
        if (m_currentLine[i] == tab)
            col = ((col / m_tabSize) + 1) * m_tabSize;
        else
            col++;
    }
    return col;
}

QStringRef CodeFormatter::currentTokenText() const
{
    return m_currentLine.midRef(m_currentToken.begin(), m_currentToken.length);
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

int CodeFormatter::tokenizeBlock(const QTextBlock &block)
{
    int startState = loadLexerState(block.previous());
    if (block.blockNumber() == 0)
        startState = 0;
    Q_ASSERT(startState != -1);

    Scanner tokenize;
    tokenize.setScanComments(true);

    m_currentLine = block.text();
    // to determine whether a line was joined, Tokenizer needs a
    // newline character at the end
    m_currentLine.append(QLatin1Char('\n'));
    m_tokens = tokenize(m_currentLine, startState);

    const int lexerState = tokenize.state();
    QTextBlock saveableBlock(block);
    saveLexerState(&saveableBlock, lexerState);
    return lexerState;
}

CodeFormatter::TokenKind CodeFormatter::extendedTokenKind(const QmlJS::Token &token) const
{
    const int kind = token.kind;
    QStringRef text = m_currentLine.midRef(token.begin(), token.length);

    if (kind == Identifier) {
        if (text == QLatin1String("as"))
            return As;
        if (text == QLatin1String("import"))
            return Import;
        if (text == QLatin1String("signal"))
            return Signal;
        if (text == QLatin1String("property"))
            return Property;
        if (text == QLatin1String("on"))
            return On;
        if (text == QLatin1String("list"))
            return List;
    } else if (kind == Keyword) {
        const char char1 = text.at(0).toLatin1();
        const char char2 = text.at(1).toLatin1();
        const char char3 = (text.size() > 2 ? text.at(2).toLatin1() : 0);
        switch (char1) {
        case 'v':
            return Var;
        case 'i':
            if (char2 == 'f')
                return If;
            else if (char3 == 's')
                return Instanceof;
            else
                return In;
        case 'f':
            if (char2 == 'o')
                return For;
            else if (char2 == 'u')
                return Function;
            else
                return Finally;
        case 'e':
            return Else;
        case 'n':
            return New;
        case 'r':
            return Return;
        case 's':
            return Switch;
        case 'w':
            if (char2 == 'h')
                return While;
            return With;
        case 'c':
            if (char3 == 's')
                return Case;
            if (char3 == 't')
                return Catch;
            return Continue;
        case 'd':
            if (char3 == 'l')
                return Delete;
            if (char3 == 'f')
                return Default;
            if (char3 == 'b')
                return Debugger;
            return Do;
        case 't':
            if (char3 == 'i')
                return This;
            if (char3 == 'y')
                return Try;
            if (char3 == 'r')
                return Throw;
            return Typeof;
        case 'b':
            return Break;
        }
    } else if (kind == Delimiter) {
        if (text == QLatin1String("?"))
            return Question;
        else if (text == QLatin1String("++"))
            return PlusPlus;
        else if (text == QLatin1String("--"))
            return MinusMinus;
    }

    return static_cast<TokenKind>(kind);
}

void CodeFormatter::dump() const
{
    qDebug() << "Current token index" << m_tokenIndex;
    qDebug() << "Current state:";
    foreach (const State &s, m_currentState) {
        qDebug() << stateToString(s.type) << s.savedIndentDepth;
    }
    qDebug() << "Current indent depth:" << m_indentDepth;
}

QString CodeFormatter::stateToString(int type) const
{
    const QMetaEnum &metaEnum = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("StateType"));
    return QString::fromUtf8(metaEnum.valueToKey(type));
}

QtStyleCodeFormatter::QtStyleCodeFormatter()
    : m_indentSize(4)
{}

void QtStyleCodeFormatter::setIndentSize(int size)
{
    m_indentSize = size;
}

void QtStyleCodeFormatter::onEnter(int newState, int *indentDepth, int *savedIndentDepth) const
{
    const State &parentState = state();
    const Token &tk = currentToken();
    const int tokenPosition = column(tk.begin());
    const bool firstToken = (tokenIndex() == 0);
    const bool lastToken = (tokenIndex() == tokenCount() - 1);

    switch (newState) {
    case objectdefinition_open: {
        // special case for things like "gradient: Gradient {"
        if (parentState.type == binding_assignment)
            *savedIndentDepth = state(1).savedIndentDepth;

        if (firstToken)
            *savedIndentDepth = tokenPosition;

        *indentDepth = *savedIndentDepth + m_indentSize;
        break;
    }

    case binding_or_objectdefinition:
        if (firstToken)
            *indentDepth = *savedIndentDepth = tokenPosition;
        break;

    case binding_assignment:
    case objectliteral_assignment:
        if (lastToken)
            *indentDepth = *savedIndentDepth + 4;
        else
            *indentDepth = column(tokenAt(tokenIndex() + 1).begin());
        break;

    case expression_or_objectdefinition:
        *indentDepth = tokenPosition;
        break;

    case expression_or_label:
        if (*indentDepth == tokenPosition)
            *indentDepth += 2*m_indentSize;
        else
            *indentDepth = tokenPosition;
        break;

    case expression:
        if (*indentDepth == tokenPosition) {
            // expression_or_objectdefinition doesn't want the indent
            // expression_or_label already has it
            if (parentState.type != expression_or_objectdefinition
                    && parentState.type != expression_or_label
                    && parentState.type != binding_assignment) {
                *indentDepth += 2*m_indentSize;
            }
        }
        // expression_or_objectdefinition and expression_or_label have already consumed the first token
        else if (parentState.type != expression_or_objectdefinition
                 && parentState.type != expression_or_label) {
            *indentDepth = tokenPosition;
        }
        break;

    case expression_maybe_continuation:
        // set indent depth to indent we'd get if the expression ended here
        for (int i = 1; state(i).type != topmost_intro; ++i) {
            const int type = state(i).type;
            if (isExpressionEndState(type) && !isBracelessState(type)) {
                *indentDepth = state(i - 1).savedIndentDepth;
                break;
            }
        }
        break;

    case bracket_open:
        if (parentState.type == expression && state(1).type == binding_assignment) {
            *savedIndentDepth = state(2).savedIndentDepth;
            *indentDepth = *savedIndentDepth + m_indentSize;
        } else if (parentState.type == objectliteral_assignment) {
            *savedIndentDepth = parentState.savedIndentDepth;
            *indentDepth = *savedIndentDepth + m_indentSize;
        } else if (!lastToken) {
            *indentDepth = tokenPosition + 1;
        } else {
            *indentDepth = *savedIndentDepth + m_indentSize;
        }
        break;

    case function_start:
        // align to the beginning of the line
        *savedIndentDepth = *indentDepth = column(tokenAt(0).begin());
        break;

    case do_statement_while_paren_open:
    case statement_with_condition_paren_open:
    case signal_arglist_open:
    case function_arglist_open:
    case paren_open:
        if (!lastToken)
            *indentDepth = tokenPosition + 1;
        else
            *indentDepth += m_indentSize;
        break;

    case ternary_op:
        if (!lastToken)
            *indentDepth = tokenPosition + tk.length + 1;
        else
            *indentDepth += m_indentSize;
        break;

    case jsblock_open:
        // closing brace should be aligned to case
        if (parentState.type == case_cont) {
            *savedIndentDepth = parentState.savedIndentDepth;
            break;
        }
        // fallthrough
    case substatement_open:
        // special case for "foo: {" and "property int foo: {"
        if (parentState.type == binding_assignment)
            *savedIndentDepth = state(1).savedIndentDepth;
        *indentDepth = *savedIndentDepth + m_indentSize;
        break;

    case substatement:
        *indentDepth += m_indentSize;
        break;

    case objectliteral_open:
        if (parentState.type == expression
                || parentState.type == objectliteral_assignment) {
            // undo the continuation indent of the expression
            if (state(1).type == expression_or_label)
                *indentDepth = state(1).savedIndentDepth;
            else
                *indentDepth = parentState.savedIndentDepth;
            *savedIndentDepth = *indentDepth;
        }
        *indentDepth += m_indentSize;
        break;


    case statement_with_condition:
    case try_statement:
    case catch_statement:
    case finally_statement:
    case if_statement:
    case do_statement:
    case switch_statement:
        if (firstToken || parentState.type == binding_assignment)
            *savedIndentDepth = tokenPosition;
        // ### continuation
        *indentDepth = *savedIndentDepth; // + 2*m_indentSize;
        // special case for 'else if'
        if (!firstToken
                && newState == if_statement
                && parentState.type == substatement
                && state(1).type == else_clause) {
            *indentDepth = state(1).savedIndentDepth;
            *savedIndentDepth = *indentDepth;
        }
        break;

    case maybe_else:
    case maybe_catch_or_finally: {
        // set indent to where leave(true) would put it
        int lastNonEndState = 0;
        while (!isExpressionEndState(state(lastNonEndState + 1).type))
            ++lastNonEndState;
        *indentDepth = state(lastNonEndState).savedIndentDepth;
        break;
    }

    case condition_open:
        // fixed extra indent when continuing 'if (', but not for 'else if ('
        if (tokenPosition <= *indentDepth + m_indentSize)
            *indentDepth += 2*m_indentSize;
        else
            *indentDepth = tokenPosition + 1;
        break;

    case case_start:
        *savedIndentDepth = tokenPosition;
        break;

    case case_cont:
        *indentDepth += m_indentSize;
        break;

    case multiline_comment_start:
        *indentDepth = tokenPosition + 2;
        break;

    case multiline_comment_cont:
        *indentDepth = tokenPosition;
        break;
    }
}

void QtStyleCodeFormatter::adjustIndent(const QList<Token> &tokens, int startLexerState, int *indentDepth) const
{
    State topState = state();
    State previousState = state(1);

    // keep user-adjusted indent in multiline comments
    if (topState.type == multiline_comment_start
            || topState.type == multiline_comment_cont) {
        if (!tokens.isEmpty()) {
            *indentDepth = column(tokens.at(0).begin());
            return;
        }
    }
    // don't touch multi-line strings at all
    if ((startLexerState & Scanner::MultiLineMask) == Scanner::MultiLineStringDQuote
            || (startLexerState & Scanner::MultiLineMask) == Scanner::MultiLineStringSQuote) {
        *indentDepth = -1;
        return;
    }

    const int kind = extendedTokenKind(tokenAt(0));
    switch (kind) {
    case LeftBrace:
        if (topState.type == substatement
                || topState.type == binding_assignment
                || topState.type == case_cont) {
            *indentDepth = topState.savedIndentDepth;
        }
        break;
    case RightBrace: {
        if (topState.type == jsblock_open && previousState.type == case_cont) {
            *indentDepth = previousState.savedIndentDepth;
            break;
        }
        for (int i = 0; state(i).type != topmost_intro; ++i) {
            const int type = state(i).type;
            if (type == objectdefinition_open
                    || type == jsblock_open
                    || type == substatement_open
                    || type == objectliteral_open) {
                *indentDepth = state(i).savedIndentDepth;
                break;
            }
        }
        break;
    }
    case RightBracket:
        for (int i = 0; state(i).type != topmost_intro; ++i) {
            const int type = state(i).type;
            if (type == bracket_open) {
                *indentDepth = state(i).savedIndentDepth;
                break;
            }
        }
        break;

    case LeftBracket:
    case LeftParenthesis:
    case Delimiter:
        if (topState.type == expression_maybe_continuation)
            *indentDepth = topState.savedIndentDepth;
        break;

    case Else:
        if (topState.type == maybe_else) {
            *indentDepth = state(1).savedIndentDepth;
        } else if (topState.type == expression_maybe_continuation) {
            bool hasElse = false;
            for (int i = 1; state(i).type != topmost_intro; ++i) {
                const int type = state(i).type;
                if (type == else_clause)
                    hasElse = true;
                if (type == if_statement) {
                    if (hasElse) {
                        hasElse = false;
                    } else {
                        *indentDepth = state(i).savedIndentDepth;
                        break;
                    }
                }
            }
        }
        break;

    case Catch:
    case Finally:
        if (topState.type == maybe_catch_or_finally)
            *indentDepth = state(1).savedIndentDepth;
        break;

    case Colon:
        if (topState.type == ternary_op)
            *indentDepth -= 2;
        break;

    case Question:
        if (topState.type == expression_maybe_continuation)
            *indentDepth = topState.savedIndentDepth;
        break;

    case Default:
    case Case:
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
    }
}

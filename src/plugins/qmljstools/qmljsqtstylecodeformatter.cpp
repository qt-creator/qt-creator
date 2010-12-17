/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmljsqtstylecodeformatter.h"

#include <texteditor/tabsettings.h>

#include <QtCore/QDebug>

using namespace QmlJS;
using namespace QmlJSTools;
using namespace TextEditor;

QtStyleCodeFormatter::QtStyleCodeFormatter()
    : m_indentSize(4)
{
}

QtStyleCodeFormatter::QtStyleCodeFormatter(const TextEditor::TabSettings &tabSettings)
    : m_indentSize(tabSettings.m_indentSize)
{
    setTabSize(tabSettings.m_tabSize);
}

void QtStyleCodeFormatter::setIndentSize(int size)
{
    m_indentSize = size;
}

void QtStyleCodeFormatter::saveBlockData(QTextBlock *block, const BlockData &data) const
{
    TextBlockUserData *userData = BaseTextDocumentLayout::userData(*block);
    QmlJSCodeFormatterData *cppData = static_cast<QmlJSCodeFormatterData *>(userData->codeFormatterData());
    if (!cppData) {
        cppData = new QmlJSCodeFormatterData;
        userData->setCodeFormatterData(cppData);
    }
    cppData->m_data = data;
}

bool QtStyleCodeFormatter::loadBlockData(const QTextBlock &block, BlockData *data) const
{
    TextBlockUserData *userData = BaseTextDocumentLayout::testUserData(block);
    if (!userData)
        return false;
    QmlJSCodeFormatterData *cppData = static_cast<QmlJSCodeFormatterData *>(userData->codeFormatterData());
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
        if (lastToken)
            *indentDepth = *savedIndentDepth + 4;
        else
            *indentDepth = column(tokenAt(tokenIndex() + 1).begin());
        break;

    case expression_or_objectdefinition:
        *indentDepth = tokenPosition;
        break;

    case expression:
        // expression_or_objectdefinition has already consumed the first token
        // ternary already adjusts indents nicely
        if (parentState.type != expression_or_objectdefinition
                && parentState.type != binding_assignment
                && parentState.type != ternary_op) {
            *indentDepth += 2 * m_indentSize;
        }
        if (!firstToken && parentState.type != expression_or_objectdefinition) {
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
        } else if (!lastToken) {
            *indentDepth = tokenPosition + 1;
        } else {
            *indentDepth = *savedIndentDepth + m_indentSize;
        }
        break;

    case function_start:
        if (parentState.type == expression) {
            // undo the continuation indent of the expression
            *indentDepth = parentState.savedIndentDepth;
            *savedIndentDepth = *indentDepth;
        }
        break;

    case do_statement_while_paren_open:
    case statement_with_condition_paren_open:
    case signal_arglist_open:
    case function_arglist_open:
    case paren_open:
    case condition_paren_open:
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
        // special case for foo: {
        if (parentState.type == binding_assignment && state(1).type == binding_or_objectdefinition)
            *savedIndentDepth = state(1).savedIndentDepth;
        *indentDepth = *savedIndentDepth + m_indentSize;
        break;

    case statement_with_condition:
    case statement_with_block:
    case if_statement:
    case do_statement:
    case switch_statement:
        if (firstToken || parentState.type == binding_assignment)
            *savedIndentDepth = tokenPosition;
        // ### continuation
        *indentDepth = *savedIndentDepth; // + 2*m_indentSize;
        break;

    case maybe_else: {
        // set indent to outermost braceless savedIndent
        int outermostBraceless = 0;
        while (isBracelessState(state(outermostBraceless + 1).type))
            ++outermostBraceless;
        *indentDepth = state(outermostBraceless).savedIndentDepth;
        // this is where the else should go, if one appears - aligned to if_statement
        *savedIndentDepth = state().savedIndentDepth;
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

void QtStyleCodeFormatter::adjustIndent(const QList<Token> &tokens, int lexerState, int *indentDepth) const
{
    Q_UNUSED(lexerState)

    State topState = state();
    State previousState = state(1);

    // adjusting the indentDepth here instead of in enter() gives 'else if' the correct indentation
    // ### could be moved?
    if (topState.type == substatement)
        *indentDepth += m_indentSize;

    // keep user-adjusted indent in multiline comments
    if (topState.type == multiline_comment_start
            || topState.type == multiline_comment_cont) {
        if (!tokens.isEmpty()) {
            *indentDepth = column(tokens.at(0).begin());
            return;
        }
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
                    || type == substatement_open) {
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
            *indentDepth = topState.savedIndentDepth;
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

    case Colon:
        if (topState.type == ternary_op) {
            *indentDepth -= 2;
        }
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

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

#include "basetextdocumentlayout.h"

using namespace TextEditor;

bool Parenthesis::hasClosingCollapse(const Parentheses &parentheses)
{
    return closeCollapseAtPos(parentheses) >= 0;
}

int Parenthesis::closeCollapseAtPos(const Parentheses &parentheses, QChar *character)
{
    int depth = 0;
    for (int i = 0; i < parentheses.size(); ++i) {
        const Parenthesis &p = parentheses.at(i);
        if (p.chr == QLatin1Char('{')
            || p.chr == QLatin1Char('+')
            || p.chr == QLatin1Char('[')) {
            ++depth;
        } else if (p.chr == QLatin1Char('}')
            || p.chr == QLatin1Char('-')
            || p.chr == QLatin1Char(']')) {
            if (--depth < 0) {
                if (character)
                    *character = p.chr;
                return p.pos;
            }
        }
    }
    return -1;
}

int Parenthesis::collapseAtPos(const Parentheses &parentheses, QChar *character)
{
    int result = -1;
    QChar c;

    int depth = 0;
    for (int i = 0; i < parentheses.size(); ++i) {
        const Parenthesis &p = parentheses.at(i);
        if (p.chr == QLatin1Char('{')
            || p.chr == QLatin1Char('+')
            || p.chr == QLatin1Char('[')) {
            if (depth == 0) {
                result = p.pos;
                c = p.chr;
            }
            ++depth;
        } else if (p.chr == QLatin1Char('}')
            || p.chr == QLatin1Char('-')
            || p.chr == QLatin1Char(']')) {
            if (--depth < 0)
                depth = 0;
            result = -1;
        }
    }
    if (result >= 0 && character)
        *character = c;
    return result;
}


TextBlockUserData::~TextBlockUserData()
{
    TextMarks marks = m_marks;
    m_marks.clear();
    foreach (ITextMark *mrk, marks) {
        mrk->removedFromEditor();
    }
}

int TextBlockUserData::collapseAtPos(QChar *character) const
{
    return Parenthesis::collapseAtPos(m_parentheses, character);
}

int TextBlockUserData::braceDepthDelta() const
{
    int delta = 0;
    for (int i = 0; i < m_parentheses.size(); ++i) {
        switch (m_parentheses.at(i).chr.unicode()) {
        case '{': case '+': case '[': ++delta; break;
        case '}': case '-': case ']': --delta; break;
        default: break;
        }
    }
    return delta;
}


QTextBlock TextBlockUserData::testCollapse(const QTextBlock& block)
{
    QTextBlock info = block;
    if (block.userData() && static_cast<TextBlockUserData*>(block.userData())->collapseMode() == CollapseAfter)
        ;
    else if (block.next().userData()
             && static_cast<TextBlockUserData*>(block.next().userData())->collapseMode()
             == TextBlockUserData::CollapseThis)
        info = block.next();
    else
        return QTextBlock();
    int pos = static_cast<TextBlockUserData*>(info.userData())->collapseAtPos();
    if (pos < 0)
        return QTextBlock();
    QTextCursor cursor(info);
    cursor.setPosition(cursor.position() + pos);
    matchCursorForward(&cursor);
    return cursor.block();
}

void TextBlockUserData::doCollapse(const QTextBlock& block, bool visible)
{
    QTextBlock info = block;
    if (block.userData() && static_cast<TextBlockUserData*>(block.userData())->collapseMode() == CollapseAfter)
        ;
    else if (block.next().userData()
             && static_cast<TextBlockUserData*>(block.next().userData())->collapseMode()
             == TextBlockUserData::CollapseThis)
        info = block.next();
    else {
        if (visible && !block.next().isVisible()) {
            // no match, at least unfold!
            QTextBlock b = block.next();
            while (b.isValid() && !b.isVisible()) {
                b.setVisible(true);
                b.setLineCount(visible ? qMax(1, b.layout()->lineCount()) : 0);
                b = b.next();
            }
        }
        return;
    }
    int pos = static_cast<TextBlockUserData*>(info.userData())->collapseAtPos();
    if (pos < 0)
        return;
    QTextCursor cursor(info);
    cursor.setPosition(cursor.position() + pos);
    if (matchCursorForward(&cursor) != Match) {
        if (visible) {
            // no match, at least unfold!
            QTextBlock b = block.next();
            while (b.isValid() && !b.isVisible()) {
                b.setVisible(true);
                b.setLineCount(visible ? qMax(1, b.layout()->lineCount()) : 0);
                b = b.next();
            }
        }
        return;
    }

    QTextBlock b = block.next();
    while (b < cursor.block()) {
        b.setVisible(visible);
        b.setLineCount(visible ? qMax(1, b.layout()->lineCount()) : 0);
        if (visible) {
            TextBlockUserData *data = canCollapse(b);
            if (data && data->collapsed()) {
                QTextBlock end =  testCollapse(b);
                if (data->collapseIncludesClosure())
                    end = end.next();
                if (end.isValid()) {
                    b = end;
                    continue;
                }
            }
        }
        b = b.next();
    }

    bool collapseIncludesClosure = hasClosingCollapseAtEnd(b);
    if (collapseIncludesClosure) {
        b.setVisible(visible);
        b.setLineCount(visible ? qMax(1, b.layout()->lineCount()) : 0);
    }
    static_cast<TextBlockUserData*>(info.userData())->setCollapseIncludesClosure(collapseIncludesClosure);
    static_cast<TextBlockUserData*>(info.userData())->setCollapsed(!block.next().isVisible());

}


TextBlockUserData::MatchType TextBlockUserData::checkOpenParenthesis(QTextCursor *cursor, QChar c)
{
    QTextBlock block = cursor->block();
    if (!BaseTextDocumentLayout::hasParentheses(block) || BaseTextDocumentLayout::ifdefedOut(block))
        return NoMatch;

    Parentheses parenList = BaseTextDocumentLayout::parentheses(block);
    Parenthesis openParen, closedParen;
    QTextBlock closedParenParag = block;

    const int cursorPos = cursor->position() - closedParenParag.position();
    int i = 0;
    int ignore = 0;
    bool foundOpen = false;
    for (;;) {
        if (!foundOpen) {
            if (i >= parenList.count())
                return NoMatch;
            openParen = parenList.at(i);
            if (openParen.pos != cursorPos) {
                ++i;
                continue;
            } else {
                foundOpen = true;
                ++i;
            }
        }

        if (i >= parenList.count()) {
            for (;;) {
                closedParenParag = closedParenParag.next();
                if (!closedParenParag.isValid())
                    return NoMatch;
                if (BaseTextDocumentLayout::hasParentheses(closedParenParag)
                    && !BaseTextDocumentLayout::ifdefedOut(closedParenParag)) {
                    parenList = BaseTextDocumentLayout::parentheses(closedParenParag);
                    break;
                }
            }
            i = 0;
        }

        closedParen = parenList.at(i);
        if (closedParen.type == Parenthesis::Opened) {
            ignore++;
            ++i;
            continue;
        } else {
            if (ignore > 0) {
                ignore--;
                ++i;
                continue;
            }

            cursor->clearSelection();
            cursor->setPosition(closedParenParag.position() + closedParen.pos + 1, QTextCursor::KeepAnchor);

            if ((c == QLatin1Char('{') && closedParen.chr != QLatin1Char('}'))
                || (c == QLatin1Char('(') && closedParen.chr != QLatin1Char(')'))
                || (c == QLatin1Char('[') && closedParen.chr != QLatin1Char(']'))
                || (c == QLatin1Char('+') && closedParen.chr != QLatin1Char('-'))
               )
                return Mismatch;

            return Match;
        }
    }
}

TextBlockUserData::MatchType TextBlockUserData::checkClosedParenthesis(QTextCursor *cursor, QChar c)
{
    QTextBlock block = cursor->block();
    if (!BaseTextDocumentLayout::hasParentheses(block) || BaseTextDocumentLayout::ifdefedOut(block))
        return NoMatch;

    Parentheses parenList = BaseTextDocumentLayout::parentheses(block);
    Parenthesis openParen, closedParen;
    QTextBlock openParenParag = block;

    const int cursorPos = cursor->position() - openParenParag.position();
    int i = parenList.count() - 1;
    int ignore = 0;
    bool foundClosed = false;
    for (;;) {
        if (!foundClosed) {
            if (i < 0)
                return NoMatch;
            closedParen = parenList.at(i);
            if (closedParen.pos != cursorPos - 1) {
                --i;
                continue;
            } else {
                foundClosed = true;
                --i;
            }
        }

        if (i < 0) {
            for (;;) {
                openParenParag = openParenParag.previous();
                if (!openParenParag.isValid())
                    return NoMatch;

                if (BaseTextDocumentLayout::hasParentheses(openParenParag)
                    && !BaseTextDocumentLayout::ifdefedOut(openParenParag)) {
                    parenList = BaseTextDocumentLayout::parentheses(openParenParag);
                    break;
                }
            }
            i = parenList.count() - 1;
        }

        openParen = parenList.at(i);
        if (openParen.type == Parenthesis::Closed) {
            ignore++;
            --i;
            continue;
        } else {
            if (ignore > 0) {
                ignore--;
                --i;
                continue;
            }

            cursor->clearSelection();
            cursor->setPosition(openParenParag.position() + openParen.pos, QTextCursor::KeepAnchor);

            if ((c == '}' && openParen.chr != '{')    ||
                 (c == ')' && openParen.chr != '(')   ||
                 (c == ']' && openParen.chr != '[')   ||
                 (c == '-' && openParen.chr != '+'))
                return Mismatch;

            return Match;
        }
    }
}

bool TextBlockUserData::findPreviousOpenParenthesis(QTextCursor *cursor, bool select)
{
    QTextBlock block = cursor->block();
    int position = cursor->position();
    int ignore = 0;
    while (block.isValid()) {
        Parentheses parenList = BaseTextDocumentLayout::parentheses(block);
        if (!parenList.isEmpty() && !BaseTextDocumentLayout::ifdefedOut(block)) {
            for (int i = parenList.count()-1; i >= 0; --i) {
                Parenthesis paren = parenList.at(i);
                if (block == cursor->block() &&
                    (position - block.position() <= paren.pos + (paren.type == Parenthesis::Closed ? 1 : 0)))
                        continue;
                if (paren.type == Parenthesis::Closed) {
                    ++ignore;
                } else if (ignore > 0) {
                    --ignore;
                } else {
                    cursor->setPosition(block.position() + paren.pos, select ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
                    return true;
                }
            }
        }
        block = block.previous();
    }
    return false;
}

bool TextBlockUserData::findPreviousBlockOpenParenthesis(QTextCursor *cursor, bool checkStartPosition)
{
    QTextBlock block = cursor->block();
    int position = cursor->position();
    int ignore = 0;
    while (block.isValid()) {
        Parentheses parenList = BaseTextDocumentLayout::parentheses(block);
        if (!parenList.isEmpty() && !BaseTextDocumentLayout::ifdefedOut(block)) {
            for (int i = parenList.count()-1; i >= 0; --i) {
                Parenthesis paren = parenList.at(i);
                if (paren.chr != QLatin1Char('{') && paren.chr != QLatin1Char('}')
                    && paren.chr != QLatin1Char('+') && paren.chr != QLatin1Char('-')
                    && paren.chr != QLatin1Char('[') && paren.chr != QLatin1Char(']'))
                    continue;
                if (block == cursor->block()) {
                    if (position - block.position() <= paren.pos + (paren.type == Parenthesis::Closed ? 1 : 0))
                        continue;
                    if (checkStartPosition && paren.type == Parenthesis::Opened && paren.pos== cursor->position()) {
                        return true;
                    }
                }
                if (paren.type == Parenthesis::Closed) {
                    ++ignore;
                } else if (ignore > 0) {
                    --ignore;
                } else {
                    cursor->setPosition(block.position() + paren.pos);
                    return true;
                }
            }
        }
        block = block.previous();
    }
    return false;
}

bool TextBlockUserData::findNextClosingParenthesis(QTextCursor *cursor, bool select)
{
    QTextBlock block = cursor->block();
    int position = cursor->position();
    int ignore = 0;
    while (block.isValid()) {
        Parentheses parenList = BaseTextDocumentLayout::parentheses(block);
        if (!parenList.isEmpty() && !BaseTextDocumentLayout::ifdefedOut(block)) {
            for (int i = 0; i < parenList.count(); ++i) {
                Parenthesis paren = parenList.at(i);
                if (block == cursor->block() &&
                    (position - block.position() > paren.pos - (paren.type == Parenthesis::Opened ? 1 : 0)))
                    continue;
                if (paren.type == Parenthesis::Opened) {
                    ++ignore;
                } else if (ignore > 0) {
                    --ignore;
                } else {
                    cursor->setPosition(block.position() + paren.pos+1, select ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
                    return true;
                }
            }
        }
        block = block.next();
    }
    return false;
}

bool TextBlockUserData::findNextBlockClosingParenthesis(QTextCursor *cursor)
{
    QTextBlock block = cursor->block();
    int position = cursor->position();
    int ignore = 0;
    while (block.isValid()) {
        Parentheses parenList = BaseTextDocumentLayout::parentheses(block);
        if (!parenList.isEmpty() && !BaseTextDocumentLayout::ifdefedOut(block)) {
            for (int i = 0; i < parenList.count(); ++i) {
                Parenthesis paren = parenList.at(i);
                if (paren.chr != QLatin1Char('{') && paren.chr != QLatin1Char('}')
                    && paren.chr != QLatin1Char('+') && paren.chr != QLatin1Char('-')
                    && paren.chr != QLatin1Char('[') && paren.chr != QLatin1Char(']'))
                    continue;
                if (block == cursor->block() &&
                    (position - block.position() > paren.pos - (paren.type == Parenthesis::Opened ? 1 : 0)))
                    continue;
                if (paren.type == Parenthesis::Opened) {
                    ++ignore;
                } else if (ignore > 0) {
                    --ignore;
                } else {
                    cursor->setPosition(block.position() + paren.pos+1);
                    return true;
                }
            }
        }
        block = block.next();
    }
    return false;
}

TextBlockUserData::MatchType TextBlockUserData::matchCursorBackward(QTextCursor *cursor)
{
    cursor->clearSelection();
    const QTextBlock block = cursor->block();

    if (!BaseTextDocumentLayout::hasParentheses(block) || BaseTextDocumentLayout::ifdefedOut(block))
        return NoMatch;

    const int relPos = cursor->position() - block.position();

    Parentheses parentheses = BaseTextDocumentLayout::parentheses(block);
    const Parentheses::const_iterator cend = parentheses.constEnd();
    for (Parentheses::const_iterator it = parentheses.constBegin();it != cend; ++it) {
        const Parenthesis &paren = *it;
        if (paren.pos == relPos - 1
            && paren.type == Parenthesis::Closed) {
            return checkClosedParenthesis(cursor, paren.chr);
        }
    }
    return NoMatch;
}

TextBlockUserData::MatchType TextBlockUserData::matchCursorForward(QTextCursor *cursor)
{
    cursor->clearSelection();
    const QTextBlock block = cursor->block();

    if (!BaseTextDocumentLayout::hasParentheses(block) || BaseTextDocumentLayout::ifdefedOut(block))
        return NoMatch;

    const int relPos = cursor->position() - block.position();

    Parentheses parentheses = BaseTextDocumentLayout::parentheses(block);
    const Parentheses::const_iterator cend = parentheses.constEnd();
    for (Parentheses::const_iterator it = parentheses.constBegin();it != cend; ++it) {
        const Parenthesis &paren = *it;
        if (paren.pos == relPos
            && paren.type == Parenthesis::Opened) {
            return checkOpenParenthesis(cursor, paren.chr);
        }
    }
    return NoMatch;
}


BaseTextDocumentLayout::BaseTextDocumentLayout(QTextDocument *doc)
    :QPlainTextDocumentLayout(doc) {
    lastSaveRevision = 0;
    hasMarks = 0;
}

BaseTextDocumentLayout::~BaseTextDocumentLayout()
{
}

void BaseTextDocumentLayout::setParentheses(const QTextBlock &block, const Parentheses &parentheses)
{
    if (parentheses.isEmpty()) {
        if (TextBlockUserData *userData = testUserData(block))
            userData->clearParentheses();
    } else {
        userData(block)->setParentheses(parentheses);
    }
}

Parentheses BaseTextDocumentLayout::parentheses(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->parentheses();
    return Parentheses();
}

bool BaseTextDocumentLayout::hasParentheses(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->hasParentheses();
    return false;
}

bool BaseTextDocumentLayout::setIfdefedOut(const QTextBlock &block)
{
    return userData(block)->setIfdefedOut();
}

bool BaseTextDocumentLayout::clearIfdefedOut(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->clearIfdefedOut();
    return false;
}

bool BaseTextDocumentLayout::ifdefedOut(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->ifdefedOut();
    return false;
}

int BaseTextDocumentLayout::braceDepthDelta(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->braceDepthDelta();
    return 0;
}

int BaseTextDocumentLayout::braceDepth(const QTextBlock &block)
{
    int state = block.userState();
    if (state == -1)
        return 0;
    return state >> 8;
}

void BaseTextDocumentLayout::setBraceDepth(QTextBlock &block, int depth)
{
    int state = block.userState();
    if (state == -1)
        state = 0;
    state = state & 0xff;
    block.setUserState((depth << 8) | state);
}

void BaseTextDocumentLayout::changeBraceDepth(QTextBlock &block, int delta)
{
    if (delta)
        setBraceDepth(block, braceDepth(block) + delta);
}

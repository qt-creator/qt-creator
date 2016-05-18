/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "textdocumentlayout.h"
#include "textdocument.h"
#include <utils/qtcassert.h>
#include <QDebug>

using namespace TextEditor;


CodeFormatterData::~CodeFormatterData()
{
}

TextBlockUserData::~TextBlockUserData()
{
    foreach (TextMark *mrk, m_marks) {
        mrk->baseTextDocument()->removeMarkFromMarksCache(mrk);
        mrk->setBaseTextDocument(0);
        mrk->removedFromEditor();
    }

    if (m_codeFormatterData)
        delete m_codeFormatterData;
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

TextBlockUserData::MatchType TextBlockUserData::checkOpenParenthesis(QTextCursor *cursor, QChar c)
{
    QTextBlock block = cursor->block();
    if (!TextDocumentLayout::hasParentheses(block) || TextDocumentLayout::ifdefedOut(block))
        return NoMatch;

    Parentheses parenList = TextDocumentLayout::parentheses(block);
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
                if (TextDocumentLayout::hasParentheses(closedParenParag)
                    && !TextDocumentLayout::ifdefedOut(closedParenParag)) {
                    parenList = TextDocumentLayout::parentheses(closedParenParag);
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
    if (!TextDocumentLayout::hasParentheses(block) || TextDocumentLayout::ifdefedOut(block))
        return NoMatch;

    Parentheses parenList = TextDocumentLayout::parentheses(block);
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

                if (TextDocumentLayout::hasParentheses(openParenParag)
                    && !TextDocumentLayout::ifdefedOut(openParenParag)) {
                    parenList = TextDocumentLayout::parentheses(openParenParag);
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

            if ((c == QLatin1Char('}') && openParen.chr != QLatin1Char('{'))    ||
                 (c == QLatin1Char(')') && openParen.chr != QLatin1Char('('))   ||
                 (c == QLatin1Char(']') && openParen.chr != QLatin1Char('['))   ||
                 (c == QLatin1Char('-') && openParen.chr != QLatin1Char('+')))
                return Mismatch;

            return Match;
        }
    }
}

bool TextBlockUserData::findPreviousOpenParenthesis(QTextCursor *cursor, bool select, bool onlyInCurrentBlock)
{
    QTextBlock block = cursor->block();
    int position = cursor->position();
    int ignore = 0;
    while (block.isValid()) {
        Parentheses parenList = TextDocumentLayout::parentheses(block);
        if (!parenList.isEmpty() && !TextDocumentLayout::ifdefedOut(block)) {
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
        if (onlyInCurrentBlock)
            return false;
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
        Parentheses parenList = TextDocumentLayout::parentheses(block);
        if (!parenList.isEmpty() && !TextDocumentLayout::ifdefedOut(block)) {
            for (int i = parenList.count()-1; i >= 0; --i) {
                Parenthesis paren = parenList.at(i);
                if (paren.chr != QLatin1Char('{') && paren.chr != QLatin1Char('}')
                    && paren.chr != QLatin1Char('+') && paren.chr != QLatin1Char('-'))
                    continue;
                if (block == cursor->block()) {
                    if (position - block.position() <= paren.pos + (paren.type == Parenthesis::Closed ? 1 : 0))
                        continue;
                    if (checkStartPosition && paren.type == Parenthesis::Opened && paren.pos== cursor->position())
                        return true;
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
        Parentheses parenList = TextDocumentLayout::parentheses(block);
        if (!parenList.isEmpty() && !TextDocumentLayout::ifdefedOut(block)) {
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
        Parentheses parenList = TextDocumentLayout::parentheses(block);
        if (!parenList.isEmpty() && !TextDocumentLayout::ifdefedOut(block)) {
            for (int i = 0; i < parenList.count(); ++i) {
                Parenthesis paren = parenList.at(i);
                if (paren.chr != QLatin1Char('{') && paren.chr != QLatin1Char('}')
                    && paren.chr != QLatin1Char('+') && paren.chr != QLatin1Char('-'))
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

    if (!TextDocumentLayout::hasParentheses(block) || TextDocumentLayout::ifdefedOut(block))
        return NoMatch;

    const int relPos = cursor->position() - block.position();

    Parentheses parentheses = TextDocumentLayout::parentheses(block);
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

    if (!TextDocumentLayout::hasParentheses(block) || TextDocumentLayout::ifdefedOut(block))
        return NoMatch;

    const int relPos = cursor->position() - block.position();

    Parentheses parentheses = TextDocumentLayout::parentheses(block);
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

void TextBlockUserData::setCodeFormatterData(CodeFormatterData *data)
{
    if (m_codeFormatterData)
        delete m_codeFormatterData;

    m_codeFormatterData = data;
}

void TextBlockUserData::addMark(TextMark *mark)
{
    int i = 0;
    for ( ; i < m_marks.size(); ++i) {
        if (mark->priority() < m_marks.at(i)->priority())
            break;
    }
    m_marks.insert(i, mark);
}


TextDocumentLayout::TextDocumentLayout(QTextDocument *doc)
    : QPlainTextDocumentLayout(doc),
      lastSaveRevision(0),
      hasMarks(false),
      maxMarkWidthFactor(1.0),
      m_requiredWidth(0)
{}

TextDocumentLayout::~TextDocumentLayout()
{
    documentClosing();
}

void TextDocumentLayout::setParentheses(const QTextBlock &block, const Parentheses &parentheses)
{
    if (parentheses.isEmpty()) {
        if (TextBlockUserData *userData = testUserData(block))
            userData->clearParentheses();
    } else {
        userData(block)->setParentheses(parentheses);
    }
}

Parentheses TextDocumentLayout::parentheses(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->parentheses();
    return Parentheses();
}

bool TextDocumentLayout::hasParentheses(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->hasParentheses();
    return false;
}

bool TextDocumentLayout::setIfdefedOut(const QTextBlock &block)
{
    return userData(block)->setIfdefedOut();
}

bool TextDocumentLayout::clearIfdefedOut(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->clearIfdefedOut();
    return false;
}

bool TextDocumentLayout::ifdefedOut(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->ifdefedOut();
    return false;
}

int TextDocumentLayout::braceDepthDelta(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->braceDepthDelta();
    return 0;
}

int TextDocumentLayout::braceDepth(const QTextBlock &block)
{
    int state = block.userState();
    if (state == -1)
        return 0;
    return state >> 8;
}

void TextDocumentLayout::setBraceDepth(QTextBlock &block, int depth)
{
    int state = block.userState();
    if (state == -1)
        state = 0;
    state = state & 0xff;
    block.setUserState((depth << 8) | state);
}

void TextDocumentLayout::changeBraceDepth(QTextBlock &block, int delta)
{
    if (delta)
        setBraceDepth(block, braceDepth(block) + delta);
}

void TextDocumentLayout::setLexerState(const QTextBlock &block, int state)
{
    if (state == 0) {
        if (TextBlockUserData *userData = testUserData(block))
            userData->setLexerState(0);
    } else {
        userData(block)->setLexerState(qMax(0,state));
    }
}

int TextDocumentLayout::lexerState(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->lexerState();
    return 0;
}

void TextDocumentLayout::setFoldingIndent(const QTextBlock &block, int indent)
{
    if (indent == 0) {
        if (TextBlockUserData *userData = testUserData(block))
            userData->setFoldingIndent(0);
    } else {
        userData(block)->setFoldingIndent(indent);
    }
}

int TextDocumentLayout::foldingIndent(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->foldingIndent();
    return 0;
}

void TextDocumentLayout::changeFoldingIndent(QTextBlock &block, int delta)
{
    if (delta)
        setFoldingIndent(block, foldingIndent(block) + delta);
}

bool TextDocumentLayout::canFold(const QTextBlock &block)
{
    return (block.next().isValid() && foldingIndent(block.next()) > foldingIndent(block));
}

bool TextDocumentLayout::isFolded(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->folded();
    return false;
}

void TextDocumentLayout::setFolded(const QTextBlock &block, bool folded)
{
    if (folded)
        userData(block)->setFolded(true);
    else if (TextBlockUserData *userData = testUserData(block))
        return userData->setFolded(false);
}

void TextDocumentLayout::requestExtraAreaUpdate()
{
    emit updateExtraArea();
}

void TextDocumentLayout::doFoldOrUnfold(const QTextBlock& block, bool unfold)
{
    if (!canFold(block))
        return;
    QTextBlock b = block.next();

    int indent = foldingIndent(block);
    while (b.isValid() && foldingIndent(b) > indent && (unfold || b.next().isValid())) {
        b.setVisible(unfold);
        b.setLineCount(unfold? qMax(1, b.layout()->lineCount()) : 0);
        if (unfold) { // do not unfold folded sub-blocks
            if (isFolded(b) && b.next().isValid()) {
                int jndent = foldingIndent(b);
                b = b.next();
                while (b.isValid() && foldingIndent(b) > jndent)
                    b = b.next();
                continue;
            }
        }
        b = b.next();
    }
    setFolded(block, !unfold);
}

void TextDocumentLayout::setRequiredWidth(int width)
{
    int oldw = m_requiredWidth;
    m_requiredWidth = width;
    int dw = QPlainTextDocumentLayout::documentSize().width();
    if (oldw > dw || width > dw)
        emitDocumentSizeChanged();
}


QSizeF TextDocumentLayout::documentSize() const
{
    QSizeF size = QPlainTextDocumentLayout::documentSize();
    size.setWidth(qMax((qreal)m_requiredWidth, size.width()));
    return size;
}

TextMarks TextDocumentLayout::documentClosing()
{
    TextMarks marks;
    QTextBlock block = document()->begin();
    while (block.isValid()) {
        if (TextBlockUserData *data = static_cast<TextBlockUserData *>(block.userData()))
            marks.append(data->documentClosing());
        block = block.next();
    }
    return marks;
}

void TextDocumentLayout::documentReloaded(TextMarks marks, TextDocument *baseTextDocument)
{
    foreach (TextMark *mark, marks) {
        int blockNumber = mark->lineNumber() - 1;
        QTextBlock block = document()->findBlockByNumber(blockNumber);
        if (block.isValid()) {
            TextBlockUserData *userData = TextDocumentLayout::userData(block);
            userData->addMark(mark);
            mark->setBaseTextDocument(baseTextDocument);
            mark->updateBlock(block);
        } else {
            baseTextDocument->removeMarkFromMarksCache(mark);
            mark->setBaseTextDocument(0);
            mark->removedFromEditor();
        }
    }
    requestUpdate();
}

void TextDocumentLayout::updateMarksLineNumber()
{
    // Note: the breakpointmanger deletes breakpoint marks and readds them
    // if it doesn't agree with our updating
    QTextBlock block = document()->begin();
    int blockNumber = 0;
    while (block.isValid()) {
        if (const TextBlockUserData *userData = testUserData(block))
            foreach (TextMark *mrk, userData->marks())
                mrk->updateLineNumber(blockNumber + 1);
        block = block.next();
        ++blockNumber;
    }
}

void TextDocumentLayout::updateMarksBlock(const QTextBlock &block)
{
    if (const TextBlockUserData *userData = testUserData(block))
        foreach (TextMark *mrk, userData->marks())
            mrk->updateBlock(block);
}

TextDocumentLayout::FoldValidator::FoldValidator()
    : m_layout(0)
    , m_requestDocUpdate(false)
    , m_insideFold(0)
{}

void TextDocumentLayout::FoldValidator::setup(TextDocumentLayout *layout)
{
    m_layout = layout;
}

void TextDocumentLayout::FoldValidator::reset()
{
    m_insideFold = 0;
    m_requestDocUpdate = false;
}

void TextDocumentLayout::FoldValidator::process(QTextBlock block)
{
    if (!m_layout)
        return;

    const QTextBlock &previous = block.previous();
    if (!previous.isValid())
        return;

    const bool preIsFolded = isFolded(previous);
    const bool preCanFold = canFold(previous);
    const bool isVisible = block.isVisible();

    if (preIsFolded && !preCanFold)
        setFolded(previous, false);
    else if (!preIsFolded && preCanFold && previous.isVisible() && !isVisible)
        setFolded(previous, true);

    if (isFolded(previous) && !m_insideFold)
        m_insideFold = foldingIndent(block);

    bool shouldBeVisible = m_insideFold == 0;
    if (!shouldBeVisible) {
        shouldBeVisible = foldingIndent(block) < m_insideFold;
        if (shouldBeVisible)
            m_insideFold = 0;
    }

    if (shouldBeVisible != isVisible) {
        block.setVisible(shouldBeVisible);
        block.setLineCount(block.isVisible() ? qMax(1, block.layout()->lineCount()) : 0);
        m_requestDocUpdate = true;
    }
}

void TextDocumentLayout::FoldValidator::finalize()
{
    if (m_requestDocUpdate && m_layout) {
        m_layout->requestUpdate();
        m_layout->emitDocumentSizeChanged();
    }
}

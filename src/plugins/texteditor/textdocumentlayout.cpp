// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textdocumentlayout.h"
#include "fontsettings.h"
#include "textdocument.h"
#include "texteditorplugin.h"
#include "texteditorsettings.h"

#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>

#include <QDebug>
#ifdef WITH_TESTS
#include <QTest>
#endif

namespace TextEditor {

CodeFormatterData::~CodeFormatterData() = default;

TextBlockUserData::~TextBlockUserData()
{
    for (TextMark *mrk : std::as_const(m_marks)) {
        mrk->baseTextDocument()->removeMarkFromMarksCache(mrk);
        mrk->setBaseTextDocument(nullptr);
        mrk->removedFromEditor();
    }

    delete m_codeFormatterData;
}

int TextBlockUserData::braceDepthDelta() const
{
    int delta = 0;
    for (auto &parenthesis : m_parentheses) {
        switch (parenthesis.chr.unicode()) {
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
                if (paren.chr != QLatin1Char('+') && paren.chr != QLatin1Char('-'))
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
                if (paren.chr != QLatin1Char('+') && paren.chr != QLatin1Char('-'))
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
        if (paren.pos == relPos - 1 && paren.type == Parenthesis::Closed)
            return checkClosedParenthesis(cursor, paren.chr);
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

void TextBlockUserData::insertSuggestion(std::unique_ptr<TextSuggestion> &&suggestion)
{
    m_suggestion = std::move(suggestion);
}

TextSuggestion *TextBlockUserData::suggestion() const
{
    return m_suggestion.get();
}

void TextBlockUserData::clearSuggestion()
{
    m_suggestion.reset();
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
    : QPlainTextDocumentLayout(doc)
{}

TextDocumentLayout::~TextDocumentLayout()
{
    documentClosing();
}

void TextDocumentLayout::setParentheses(const QTextBlock &block, const Parentheses &parentheses)
{
    if (TextDocumentLayout::parentheses(block) == parentheses)
        return;

    userData(block)->setParentheses(parentheses);
    if (auto layout = qobject_cast<TextDocumentLayout *>(block.document()->documentLayout()))
        emit layout->parenthesesChanged(block);
}

Parentheses TextDocumentLayout::parentheses(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->parentheses();
    return Parentheses();
}

bool TextDocumentLayout::hasParentheses(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->hasParentheses();
    return false;
}

bool TextDocumentLayout::setIfdefedOut(const QTextBlock &block)
{
    return userData(block)->setIfdefedOut();
}

bool TextDocumentLayout::clearIfdefedOut(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->clearIfdefedOut();
    return false;
}

bool TextDocumentLayout::ifdefedOut(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->ifdefedOut();
    return false;
}

int TextDocumentLayout::braceDepthDelta(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
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
        if (TextBlockUserData *userData = textUserData(block))
            userData->setLexerState(0);
    } else {
        userData(block)->setLexerState(qMax(0,state));
    }
}

int TextDocumentLayout::lexerState(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->lexerState();
    return 0;
}

void TextDocumentLayout::setFoldingIndent(const QTextBlock &block, int indent)
{
    if (indent == 0) {
        if (TextBlockUserData *userData = textUserData(block))
            userData->setFoldingIndent(0);
    } else {
        userData(block)->setFoldingIndent(indent);
    }
}

int TextDocumentLayout::foldingIndent(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
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
    if (TextBlockUserData *userData = textUserData(block))
        return userData->folded();
    return false;
}

void TextDocumentLayout::setFolded(const QTextBlock &block, bool folded)
{
    if (folded)
        userData(block)->setFolded(true);
    else if (TextBlockUserData *userData = textUserData(block))
        userData->setFolded(false);
    else
        return;

    if (auto layout = qobject_cast<TextDocumentLayout *>(block.document()->documentLayout()))
        emit layout->foldChanged(block.blockNumber(), folded);
}

void TextDocumentLayout::setExpectedRawStringSuffix(const QTextBlock &block,
                                                    const QByteArray &suffix)
{
    if (TextBlockUserData * const data = textUserData(block))
        data->setExpectedRawStringSuffix(suffix);
    else if (!suffix.isEmpty())
        userData(block)->setExpectedRawStringSuffix(suffix);
}

QByteArray TextDocumentLayout::expectedRawStringSuffix(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->expectedRawStringSuffix();
    return {};
}

TextSuggestion *TextDocumentLayout::suggestion(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->suggestion();
    return nullptr;
}

void TextDocumentLayout::updateSuggestionFormats(const QTextBlock &block,
                                                 const FontSettings &fontSettings)
{
    if (TextSuggestion *suggestion = TextDocumentLayout::suggestion(block)) {
        QTextDocument *suggestionDoc = suggestion->document();
        const QTextCharFormat replacementFormat = fontSettings.toTextCharFormat(
            TextStyles{C_TEXT, {C_DISABLED_CODE}});
        QList<QTextLayout::FormatRange> formats = block.layout()->formats();
        QTextCursor cursor(suggestionDoc);
        cursor.select(QTextCursor::Document);
        cursor.setCharFormat(fontSettings.toTextCharFormat(C_TEXT));
        const int position = suggestion->currentPosition() - block.position();
        cursor.setPosition(position);
        const QString trailingText = block.text().mid(position);
        if (!trailingText.isEmpty()) {
            const int trailingIndex = suggestionDoc->firstBlock().text().indexOf(trailingText,
                                                                               position);
            if (trailingIndex >= 0) {
                cursor.setPosition(trailingIndex, QTextCursor::KeepAnchor);
                cursor.setCharFormat(replacementFormat);
                cursor.setPosition(trailingIndex + trailingText.size());
                const int length = std::max(trailingIndex - position, 0);
                if (length) {
                    // we have a replacement in the middle of the line adjust all formats that are
                    // behind the replacement
                    QTextLayout::FormatRange rest;
                    rest.start = -1;
                    for (QTextLayout::FormatRange &range : formats) {
                        if (range.start >= position) {
                            range.start += length;
                        } else if (range.start + range.length > position) {
                            // the format range starts before and ends after the position so we need to
                            // split the format into before and after the suggestion format ranges
                            rest.start = trailingIndex;
                            rest.length = range.length - (position - range.start);
                            rest.format = range.format;
                            range.length = position - range.start;
                        }
                    }
                    if (rest.start >= 0)
                        formats += rest;
                }
            }
        }
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        cursor.setCharFormat(replacementFormat);
        suggestionDoc->firstBlock().layout()->setFormats(formats);
    }
}

bool TextDocumentLayout::updateSuggestion(const QTextBlock &block,
                                          int position,
                                          const FontSettings &fontSettings)
{
    if (TextSuggestion *suggestion = TextDocumentLayout::suggestion(block)) {
        auto positionInBlock = position - block.position();
        if (position < suggestion->position())
            return false;
        const QString start = block.text().left(positionInBlock);
        const QString end = block.text().mid(positionInBlock);
        const QString replacement = suggestion->document()->firstBlock().text();
        if (replacement.startsWith(start) && replacement.indexOf(end, start.size()) >= 0) {
            suggestion->setCurrentPosition(position);
            TextDocumentLayout::updateSuggestionFormats(block, fontSettings);
            return true;
        }
    }
    return false;
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
    int dw = int(QPlainTextDocumentLayout::documentSize().width());
    if (oldw > dw || width > dw)
        emitDocumentSizeChanged();
}


QSizeF TextDocumentLayout::documentSize() const
{
    QSizeF size = QPlainTextDocumentLayout::documentSize();
    size.setWidth(qMax(qreal(m_requiredWidth), size.width()));
    return size;
}

TextMarks TextDocumentLayout::documentClosing()
{
    QTC_ASSERT(m_reloadMarks.isEmpty(), resetReloadMarks());
    TextMarks marks;
    for (QTextBlock block = document()->begin(); block.isValid(); block = block.next()) {
        if (auto data = static_cast<TextBlockUserData *>(block.userData()))
            marks.append(data->documentClosing());
    }
    return marks;
}

void TextDocumentLayout::documentAboutToReload(TextDocument *baseTextDocument)
{
    m_reloadMarks = documentClosing();
    for (TextMark *mark : std::as_const(m_reloadMarks)) {
        mark->setDeleteCallback([this, mark, baseTextDocument] {
            baseTextDocument->removeMarkFromMarksCache(mark);
            m_reloadMarks.removeOne(mark);
        });
    }
}

void TextDocumentLayout::documentReloaded(TextDocument *baseTextDocument)
{
    const TextMarks marks = m_reloadMarks;
    resetReloadMarks();
    for (TextMark *mark : marks) {
        int blockNumber = mark->lineNumber() - 1;
        QTextBlock block = document()->findBlockByNumber(blockNumber);
        if (block.isValid()) {
            TextBlockUserData *userData = TextDocumentLayout::userData(block);
            userData->addMark(mark);
            mark->setBaseTextDocument(baseTextDocument);
            mark->updateBlock(block);
        } else {
            baseTextDocument->removeMarkFromMarksCache(mark);
            mark->setBaseTextDocument(nullptr);
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
        if (const TextBlockUserData *userData = textUserData(block)) {
            for (TextMark *mrk : userData->marks())
                mrk->updateLineNumber(blockNumber + 1);
        }
        block = block.next();
        ++blockNumber;
    }
}

void TextDocumentLayout::updateMarksBlock(const QTextBlock &block)
{
    if (const TextBlockUserData *userData = textUserData(block)) {
        for (TextMark *mrk : userData->marks())
            mrk->updateBlock(block);
    }
}

void TextDocumentLayout::scheduleUpdate()
{
    if (m_updateScheduled)
        return;
    m_updateScheduled = true;
    QMetaObject::invokeMethod(this, &TextDocumentLayout::requestUpdateNow, Qt::QueuedConnection);
}

void TextDocumentLayout::requestUpdateNow()
{
    m_updateScheduled = false;
    requestUpdate();
}

void TextDocumentLayout::resetReloadMarks()
{
    for (TextMark *mark : std::as_const(m_reloadMarks))
        mark->setDeleteCallback({});
    m_reloadMarks.clear();
}

static QRectF replacementBoundingRect(const QTextDocument *replacement)
{
    QTC_ASSERT(replacement, return {});
    auto *layout = static_cast<QPlainTextDocumentLayout *>(replacement->documentLayout());
    QRectF boundingRect;
    QTextBlock block = replacement->firstBlock();
    while (block.isValid()) {
        const QRectF blockBoundingRect = layout->blockBoundingRect(block);
        boundingRect.setWidth(std::max(boundingRect.width(), blockBoundingRect.width()));
        boundingRect.setHeight(boundingRect.height() + blockBoundingRect.height());
        block = block.next();
    }
    return boundingRect;
}

QRectF TextDocumentLayout::blockBoundingRect(const QTextBlock &block) const
{
    if (TextSuggestion *suggestion = TextDocumentLayout::suggestion(block)) {
        // since multiple code paths expects that we have a valid block layout after requesting the
        // block bounding rect explicitly create that layout here
        ensureBlockLayout(block);
        return replacementBoundingRect(suggestion->document());
    }

    QRectF boundingRect = QPlainTextDocumentLayout::blockBoundingRect(block);

    if (TextEditorSettings::fontSettings().relativeLineSpacing() != 100) {
        if (boundingRect.isNull())
            return boundingRect;
        boundingRect.setHeight(TextEditorSettings::fontSettings().lineSpacing());
    }

    if (TextBlockUserData *userData = textUserData(block))
        boundingRect.adjust(0, 0, 0, userData->additionalAnnotationHeight());
    return boundingRect;
}

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

QDebug operator<<(QDebug debug, const Parenthesis &parenthesis)
{
    QDebugStateSaver saver(debug);
    debug << (parenthesis.type == Parenthesis::Opened ? "Opening " : "Closing ") << parenthesis.chr
          << " at " << parenthesis.pos;

    return debug;
}

bool Parenthesis::operator==(const Parenthesis &other) const
{
    return pos == other.pos && chr == other.chr && source == other.source && type == other.type;
}

void insertSorted(Parentheses &list, const Parenthesis &elem)
{
    const auto it = std::lower_bound(list.constBegin(), list.constEnd(), elem,
            [](const auto &p1, const auto &p2) { return p1.pos < p2.pos; });
    list.insert(it, elem);
}

TextSuggestion::TextSuggestion()
{
    m_replacementDocument.setDocumentLayout(new TextDocumentLayout(&m_replacementDocument));
    m_replacementDocument.setDocumentMargin(0);
}

TextSuggestion::~TextSuggestion() = default;

#ifdef WITH_TESTS

void Internal::TextEditorPlugin::testDeletingMarkOnReload()
{
    auto doc = new TextDocument();
    doc->setFilePath(Utils::TemporaryDirectory::masterDirectoryFilePath() / "TestMarkDoc.txt");
    doc->setPlainText("asd");
    auto documentLayout = qobject_cast<TextDocumentLayout *>(doc->document()->documentLayout());
    QVERIFY(documentLayout);
    auto mark = new TextMark(doc, 1, TextMarkCategory{"testMark","testMark"});
    QVERIFY(doc->marks().contains(mark));
    documentLayout->documentAboutToReload(doc); // removes text marks non-permanently
    delete mark;
    documentLayout->documentReloaded(doc); // re-adds text marks
    QVERIFY(!doc->marks().contains(mark));
}

#endif

} // namespace TextEditor

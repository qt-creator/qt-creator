// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fontsettings.h"
#include "textdocument.h"
#include "textdocumentlayout.h"
#include "texteditorsettings.h"

#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>

#include <QDebug>

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

TextBlockUserData::MatchType TextBlockUserData::checkOpenParenthesis(QTextCursor *cursor, QChar c)
{
    QTextBlock block = cursor->block();
    if (!hasParentheses(block) || ifdefedOut(block))
        return NoMatch;

    Parentheses parenList = parentheses(block);
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
                if (hasParentheses(closedParenParag) && !ifdefedOut(closedParenParag)) {
                    parenList = parentheses(closedParenParag);
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
    if (!hasParentheses(block) || ifdefedOut(block))
        return NoMatch;

    Parentheses parenList = parentheses(block);
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

                if (hasParentheses(openParenParag) && !ifdefedOut(openParenParag)) {
                    parenList = parentheses(openParenParag);
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
        Parentheses parenList = parentheses(block);
        if (!parenList.isEmpty() && !ifdefedOut(block)) {
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
        Parentheses parenList = parentheses(block);
        if (!parenList.isEmpty() && !ifdefedOut(block)) {
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
        Parentheses parenList = parentheses(block);
        if (!parenList.isEmpty() && !ifdefedOut(block)) {
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
        Parentheses parenList = parentheses(block);
        if (!parenList.isEmpty() && !ifdefedOut(block)) {
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

    if (!hasParentheses(block) || ifdefedOut(block))
        return NoMatch;

    const int relPos = cursor->position() - block.position();

    Parentheses parentheses = TextBlockUserData::parentheses(block);
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

    if (!hasParentheses(block) || ifdefedOut(block))
        return NoMatch;

    const int relPos = cursor->position() - block.position();

    Parentheses parentheses = TextBlockUserData::parentheses(block);
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

void TextBlockUserData::insertSuggestion(
    const QTextBlock &block, std::unique_ptr<TextSuggestion> &&suggestion)
{
    userData(block)->m_suggestion = std::move(suggestion);
}

void TextBlockUserData::clearSuggestion(const QTextBlock &block)
{
    if (auto data = textUserData(block))
        return data->m_suggestion.reset();
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
    : PlainTextDocumentLayout(doc)
{}

TextDocumentLayout::~TextDocumentLayout()
{
    documentClosing();
}

void TextBlockUserData::setParentheses(const QTextBlock &block, const Parentheses &parentheses)
{
    if (TextBlockUserData::parentheses(block) == parentheses)
        return;

    userData(block)->m_parentheses = parentheses;
    if (auto layout = qobject_cast<TextDocumentLayout *>(block.document()->documentLayout()))
        emit layout->parenthesesChanged(block);
}

Parentheses TextBlockUserData::parentheses(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->m_parentheses;
    return Parentheses();
}

bool TextBlockUserData::hasParentheses(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return !userData->m_parentheses.empty();
    return false;
}

void TextBlockUserData::setIfdefedOut(const QTextBlock &block)
{
    userData(block)->m_ifdefedOut = true;
}

void TextBlockUserData::clearIfdefedOut(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        userData->m_ifdefedOut = false;
}

bool TextBlockUserData::ifdefedOut(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->m_ifdefedOut;
    return false;
}

int TextBlockUserData::braceDepthDelta(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block)) {
        int delta = 0;
        for (auto &parenthesis : userData->m_parentheses) {
            switch (parenthesis.chr.unicode()) {
            case '{': case '+': case '[': ++delta; break;
            case '}': case '-': case ']': --delta; break;
            default: break;
            }
        }
        return delta;
    }
    return 0;
}

int TextBlockUserData::braceDepth(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->m_braceDepth;
    return 0;
}

void TextBlockUserData::setBraceDepth(const QTextBlock &block, int depth)
{
    if (depth == 0) {
        if (TextBlockUserData *userData = textUserData(block))
            userData->m_braceDepth = depth;
    } else {
        userData(block)->m_braceDepth = depth;
    }
}

void TextBlockUserData::changeBraceDepth(const QTextBlock &block, int delta)
{
    if (delta)
        setBraceDepth(block, braceDepth(block) + delta);
}

void TextBlockUserData::setLexerState(const QTextBlock &block, int state)
{
    if (state == 0) {
        if (TextBlockUserData *userData = textUserData(block))
            userData->m_lexerState = 0;
    } else {
        userData(block)->m_lexerState = qMax(0,state);
    }
}

int TextBlockUserData::lexerState(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->m_lexerState;
    return 0;
}

void TextBlockUserData::setFoldingIndent(const QTextBlock &block, int indent)
{
    if (indent == 0) {
        if (TextBlockUserData *userData = textUserData(block))
            userData->m_foldingIndent = 0;
    } else {
        userData(block)->m_foldingIndent = indent;
    }
}

int TextBlockUserData::foldingIndent(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->m_foldingIndent;
    return 0;
}

void TextBlockUserData::changeFoldingIndent(QTextBlock &block, int delta)
{
    if (delta)
        setFoldingIndent(block, foldingIndent(block) + delta);
}

bool TextBlockUserData::canFold(const QTextBlock &block)
{
    return (block.next().isValid() && foldingIndent(block.next()) > foldingIndent(block));
}

bool TextBlockUserData::isFolded(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->m_folded;
    return false;
}

void TextBlockUserData::setFolded(const QTextBlock &block, bool folded)
{
    if (folded)
        userData(block)->m_folded = true;
    else if (TextBlockUserData *userData = textUserData(block))
        userData->m_folded = false;
    else
        return;

    if (auto layout = qobject_cast<TextDocumentLayout *>(block.document()->documentLayout()))
        emit layout->foldChanged(block.blockNumber(), folded);
}

void TextBlockUserData::setExpectedRawStringSuffix(const QTextBlock &block,
                                                    const QByteArray &suffix)
{
    if (TextBlockUserData * const data = textUserData(block))
        data->m_expectedRawStringSuffix = suffix;
    else if (!suffix.isEmpty())
        userData(block)->m_expectedRawStringSuffix = suffix;
}

QByteArray TextBlockUserData::expectedRawStringSuffix(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->m_expectedRawStringSuffix;
    return {};
}

TextSuggestion *TextBlockUserData::suggestion(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->m_suggestion.get();
    return nullptr;
}

void TextBlockUserData::setAttributeState(const QTextBlock &block, quint8 attrState)
{
    if (TextBlockUserData * const data = textUserData(block))
        data->m_attrState = attrState;
    else if (attrState)
        userData(block)->m_attrState = attrState;
}

quint8 TextBlockUserData::attributeState(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->m_attrState;
    return 0;
}

void TextBlockUserData::updateSuggestionFormats(const QTextBlock &block,
                                                 const FontSettings &fontSettings)
{
    if (TextSuggestion *suggestion = TextBlockUserData::suggestion(block)) {
        QTextDocument *suggestionDoc = suggestion->replacementDocument();
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

KSyntaxHighlighting::State TextBlockUserData::syntaxState(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->m_syntaxState;
    return {};
}

void TextBlockUserData::setSyntaxState(const QTextBlock &block, KSyntaxHighlighting::State state)
{
    if (state != KSyntaxHighlighting::State())
        userData(block)->m_syntaxState = state;
    else if (auto data = textUserData(block))
        data->m_syntaxState = state;
}

void TextBlockUserData::setFoldingStartIncluded(const QTextBlock &block, bool included)
{
    if (included)
        userData(block)->m_foldingStartIncluded = true;
    else if (auto data = textUserData(block))
        data->m_foldingStartIncluded = false;
}

bool TextBlockUserData::foldingStartIncluded(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->m_foldingStartIncluded;
    return false;
}

void TextBlockUserData::setFoldingEndIncluded(const QTextBlock &block, bool included)
{
    if (included)
        userData(block)->m_foldingEndIncluded = true;
    else if (auto data = textUserData(block))
        data->m_foldingEndIncluded = false;
}

bool TextBlockUserData::foldingEndIncluded(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->m_foldingEndIncluded;
    return false;
}

CodeFormatterData *TextBlockUserData::codeFormatterData(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->m_codeFormatterData;
    return nullptr;
}

void TextBlockUserData::setCodeFormatterData(const QTextBlock &block, CodeFormatterData *data)
{
    if (TextBlockUserData *blockUserData = textUserData(block)) {
        delete blockUserData->m_codeFormatterData;
        blockUserData->m_codeFormatterData = data;
    } else if (data) {
        userData(block)->m_codeFormatterData = data;
    }
}

void TextBlockUserData::addEmbeddedWidget(const QTextBlock &block, QWidget *widget)
{
    userData(block)->m_embeddedWidgets.append(widget);
}

void TextBlockUserData::removeEmbeddedWidget(const QTextBlock &block, QWidget *widget)
{
    if (TextBlockUserData *userData = textUserData(block))
        userData->m_embeddedWidgets.removeAll(widget);
}

QList<QPointer<QWidget>> TextBlockUserData::embeddedWidgets(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->m_embeddedWidgets;
    return {};
}

void TextBlockUserData::setAdditionalAnnotationHeight(const QTextBlock &block, int annotationHeight)
{
    if (annotationHeight != 0)
        userData(block)->m_additionalAnnotationHeight = annotationHeight;
    else if (TextBlockUserData *userData = textUserData(block))
        userData->m_additionalAnnotationHeight = annotationHeight;
}

int TextBlockUserData::additionalAnnotationHeight(const QTextBlock &block)
{
    if (TextBlockUserData *userData = textUserData(block))
        return userData->m_additionalAnnotationHeight;
    return 0;
}

void TextDocumentLayout::requestExtraAreaUpdate()
{
    emit updateExtraArea();
}

void TextBlockUserData::doFoldOrUnfold(const QTextBlock &block, bool unfold, bool recursive)
{
    if (!canFold(block))
        return;
    QTextBlock b = block.next();

    int indent = foldingIndent(block);
    while (b.isValid() && foldingIndent(b) > indent && (unfold || b.next().isValid())) {
        b.setVisible(unfold);
        b.setLineCount(unfold? qMax(1, b.layout()->lineCount()) : 0);
        if (recursive) {
            if ((unfold && isFolded(b)) || (!unfold && canFold(b)))
                setFolded(b, !unfold);
        } else if (unfold) { // do not unfold folded sub-blocks
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
    int dw = int(PlainTextDocumentLayout::documentSize().width());
    if (oldw > dw || width > dw)
        emitDocumentSizeChanged();
}


QSizeF TextDocumentLayout::documentSize() const
{
    QSizeF size = PlainTextDocumentLayout::documentSize();
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
            TextBlockUserData *userData = TextBlockUserData::userData(block);
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
        if (const TextBlockUserData *userData = TextBlockUserData::textUserData(block)) {
            for (TextMark *mrk : userData->marks())
                mrk->updateLineNumber(blockNumber + 1);
        }
        block = block.next();
        ++blockNumber;
    }
}

void TextDocumentLayout::updateMarksBlock(const QTextBlock &block)
{
    if (const TextBlockUserData *userData = TextBlockUserData::textUserData(block)) {
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

int TextDocumentLayout::embeddedWidgetOffset(const QTextBlock &block, QWidget *widget)
{
    if (auto userData = TextBlockUserData::textUserData(block)) {
        int offset = PlainTextDocumentLayout::blockBoundingRect(block).height();
        for (auto embeddedWidget : userData->embeddedWidgets(block)) {
            if (embeddedWidget == widget)
                return offset;
            offset += embeddedWidget->height();
        }
    }
    return -1;
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
    auto *layout = static_cast<Utils::PlainTextDocumentLayout *>(replacement->documentLayout());
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
    if (TextSuggestion *suggestion = TextBlockUserData::suggestion(block)) {
        // since multiple code paths expects that we have a valid block layout after requesting the
        // block bounding rect explicitly create that layout here
        ensureBlockLayout(block);
        return replacementBoundingRect(suggestion->replacementDocument());
    }

    QRectF boundingRect = PlainTextDocumentLayout::blockBoundingRect(block);

    if (TextEditorSettings::fontSettings().relativeLineSpacing() != 100) {
        if (boundingRect.isNull())
            return boundingRect;
        boundingRect.setHeight(TextEditorSettings::fontSettings().lineSpacing());
    }

    int additionalHeight = 0;
    for (const QPointer<QWidget> &wdgt : TextBlockUserData::embeddedWidgets(block)) {
        if (wdgt && wdgt->isVisible())
            additionalHeight += wdgt->height();
    }
    boundingRect
        .adjust(0, 0, 0, TextBlockUserData::additionalAnnotationHeight(block) + additionalHeight);

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

    const bool preIsFolded = TextBlockUserData::isFolded(previous);
    const bool preCanFold = TextBlockUserData::canFold(previous);
    const bool isVisible = block.isVisible();

    if (preIsFolded && !preCanFold)
        TextBlockUserData::setFolded(previous, false);
    else if (!preIsFolded && preCanFold && previous.isVisible() && !isVisible)
        TextBlockUserData::setFolded(previous, true);

    if (TextBlockUserData::isFolded(previous) && !m_insideFold)
        m_insideFold = TextBlockUserData::foldingIndent(block);

    bool shouldBeVisible = m_insideFold == 0;
    if (!shouldBeVisible) {
        shouldBeVisible = TextBlockUserData::foldingIndent(block) < m_insideFold;
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

} // TextEditor


#ifdef WITH_TESTS

#include <QTest>

namespace TextEditor::Internal {

class TextDocumentLayoutTest final : public QObject
{
    Q_OBJECT

private slots:
    void testDeletingMarkOnReload();
};

void TextDocumentLayoutTest::testDeletingMarkOnReload()
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

QObject *createTextDocumentTest()
{
    return new TextDocumentLayoutTest;
}

} // TextEditor::Internal

#include "textdocumentlayout.moc"

#endif // WITH_TESTS

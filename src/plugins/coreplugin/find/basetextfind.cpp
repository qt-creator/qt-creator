// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "basetextfind.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/filesearch.h>

#include <QPointer>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>

using namespace Utils;

namespace Core {

QRegularExpression BaseTextFindBase::regularExpression(const QString &txt, FindFlags flags)
{
    return QRegularExpression((flags & FindRegularExpression) ? txt
                                                              : QRegularExpression::escape(txt),
                              (flags & FindCaseSensitively)
                                  ? QRegularExpression::NoPatternOption
                                  : QRegularExpression::CaseInsensitiveOption);
}

struct BaseTextFindPrivate
{
    BaseTextFindPrivate();

    Utils::MultiTextCursor m_scope;
    std::function<Utils::MultiTextCursor()> m_cursorProvider;
    int m_incrementalStartPos;
    bool m_incrementalWrappedState;
};

BaseTextFindPrivate::BaseTextFindPrivate()
    : m_incrementalStartPos(-1)
    , m_incrementalWrappedState(false)
{
}

/*!
    \class Core::BaseTextFind
    \inheaderfile coreplugin/find/basetextfind.h
    \inmodule QtCreator

    \brief The BaseTextFind class implements a find filter for QPlainTextEdit
    and QTextEdit based widgets.

    \sa Core::IFindFilter
*/

/*!
    \internal

    This signal is emitted when the search
    scope changes to \a cursor.
*/

/*!
    \internal

    This signal is emitted when the search results for \a txt using the given
    \a findFlags should be highlighted in the editor widget.
*/

/*!
    \internal
*/
BaseTextFindBase::BaseTextFindBase()
    : d(new BaseTextFindPrivate)
{
}

/*!
    \internal
*/
BaseTextFindBase::~BaseTextFindBase()
{
    delete d;
}

/*!
    \reimp
*/
bool BaseTextFindBase::supportsReplace() const
{
    return !isReadOnly();
}

/*!
    \reimp
*/
FindFlags BaseTextFindBase::supportedFindFlags() const
{
    return FindBackward | FindCaseSensitively | FindRegularExpression
            | FindWholeWords | FindPreserveCase;
}

/*!
    \reimp
*/
void BaseTextFindBase::resetIncrementalSearch()
{
    d->m_incrementalStartPos = -1;
    d->m_incrementalWrappedState = false;
}

/*!
    \reimp
*/
void BaseTextFindBase::clearHighlights()
{
    highlightAll(QString(), {});
}

/*!
    \reimp
*/
QString BaseTextFindBase::currentFindString() const
{
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection() && cursor.block() != cursor.document()->findBlock(cursor.anchor()))
        return QString(); // multi block selection

    if (cursor.hasSelection())
        return cursor.selectedText();

    if (!cursor.atBlockEnd() && !cursor.hasSelection()) {
        cursor.movePosition(QTextCursor::StartOfWord);
        cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        QString s = cursor.selectedText();
        for (const QChar c : std::as_const(s)) {
            if (!c.isLetterOrNumber() && c != QLatin1Char('_')) {
                s.clear();
                break;
            }
        }
        return s;
    }

    return QString();
}

/*!
    \reimp
*/
QString BaseTextFindBase::completedFindString() const
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(textCursor().selectionStart());
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    return cursor.selectedText();
}

/*!
    \reimp
*/
IFindSupport::Result BaseTextFindBase::findIncremental(const QString &txt, FindFlags findFlags)
{
    QTextCursor cursor = textCursor();
    if (d->m_incrementalStartPos < 0)
        d->m_incrementalStartPos = cursor.selectionStart();
    cursor.setPosition(d->m_incrementalStartPos);
    bool wrapped = false;
    bool found =  find(txt, findFlags, cursor, &wrapped);
    if (wrapped != d->m_incrementalWrappedState && found) {
        d->m_incrementalWrappedState = wrapped;
        showWrapIndicator(widget());
    }
    if (found)
        highlightAll(txt, findFlags);
    else
        highlightAll(QString(), {});
    return found ? Found : NotFound;
}

/*!
    \reimp
*/
IFindSupport::Result BaseTextFindBase::findStep(const QString &txt, FindFlags findFlags)
{
    bool wrapped = false;
    bool found = find(txt, findFlags, textCursor(), &wrapped);
    if (wrapped)
        showWrapIndicator(widget());
    if (found) {
        d->m_incrementalStartPos = textCursor().selectionStart();
        d->m_incrementalWrappedState = false;
    }
    return found ? Found : NotFound;
}

/*!
    \reimp
*/
void BaseTextFindBase::replace(const QString &before, const QString &after, FindFlags findFlags)
{
    QTextCursor cursor = replaceInternal(before, after, findFlags);
    setTextCursor(cursor);
}

// QTextCursor::insert moves all other QTextCursors that are the insertion point forward.
// We do not want that for the replace operation, because then e.g. the find scope would move when
// replacing a match at the start.
static void insertTextAfterSelection(const QString &text, QTextCursor &cursor)
{
    // first insert after the cursor's selection end, then remove selection
    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();
    QTextCursor insertCursor = cursor;
    insertCursor.beginEditBlock();
    insertCursor.setPosition(end);
    insertCursor.insertText(text);
    // change cursor to be behind the inserted text, like it would be when directly inserting
    cursor = insertCursor;
    // redo the selection, because that changed when inserting the text at the end...
    insertCursor.setPosition(start);
    insertCursor.setPosition(end, QTextCursor::KeepAnchor);
    insertCursor.removeSelectedText();
    insertCursor.endEditBlock();
}

QTextCursor BaseTextFindBase::replaceInternal(
    const QString &before, const QString &after, FindFlags findFlags)
{
    QTextCursor cursor = textCursor();
    bool usesRegExp = (findFlags & FindRegularExpression);
    bool preserveCase = (findFlags & FindPreserveCase);
    QRegularExpression regexp = regularExpression(before, findFlags);
    QRegularExpressionMatch match = regexp.match(cursor.selectedText());
    if (match.hasMatch()) {
        QString realAfter;
        if (usesRegExp)
            realAfter = Utils::expandRegExpReplacement(after, match.capturedTexts());
        else if (preserveCase)
            realAfter = Utils::matchCaseReplacement(cursor.selectedText(), after);
        else
            realAfter = after;
        int start = cursor.selectionStart();
        insertTextAfterSelection(realAfter, cursor);
        if ((findFlags & FindBackward) != 0)
            cursor.setPosition(start);
    }
    return cursor;
}

Utils::MultiTextCursor BaseTextFindBase::multiTextCursor() const
{
    if (d->m_cursorProvider)
        return d->m_cursorProvider();
    return Utils::MultiTextCursor({textCursor()});
}

/*!
    \reimp
*/
bool BaseTextFindBase::replaceStep(const QString &before, const QString &after, FindFlags findFlags)
{
    QTextCursor cursor = replaceInternal(before, after, findFlags);
    bool wrapped = false;
    bool found = find(before, findFlags, cursor, &wrapped);
    if (wrapped)
        showWrapIndicator(widget());
    return found;
}

/*!
    \reimp
    Returns the number of search hits replaced.
*/
int BaseTextFindBase::replaceAll(const QString &before, const QString &after, FindFlags findFlags)
{
    QTextCursor editCursor = textCursor();
    if (findFlags.testFlag(FindBackward))
        editCursor.movePosition(QTextCursor::End);
    else
        editCursor.movePosition(QTextCursor::Start);
    editCursor.movePosition(QTextCursor::Start);
    editCursor.beginEditBlock();
    int count = 0;
    bool usesRegExp = (findFlags & FindRegularExpression);
    bool preserveCase = (findFlags & FindPreserveCase);
    QRegularExpression regexp = regularExpression(before, findFlags);
    QTextCursor found = findOne(regexp, editCursor,
                                Utils::textDocumentFlagsForFindFlags(findFlags));
    bool first = true;
    while (!found.isNull()) {
        if (found == editCursor && !first) {
            if (editCursor.atEnd())
                break;
            // If the newly found QTextCursor is the same as recently edit one we have to move on,
            // otherwise we would run into an endless loop for some regular expressions
            // like ^ or \b.
            QTextCursor newPosCursor = editCursor;
            newPosCursor.movePosition(findFlags & FindBackward ?
                                          QTextCursor::PreviousCharacter :
                                          QTextCursor::NextCharacter);
            found = findOne(regexp, newPosCursor, Utils::textDocumentFlagsForFindFlags(findFlags));
            continue;
        }
        if (first)
            first = false;
        ++count;
        editCursor.setPosition(found.selectionStart());
        editCursor.setPosition(found.selectionEnd(), QTextCursor::KeepAnchor);
        QRegularExpressionMatch match = regexp.match(found.selectedText());

        QString realAfter;
        if (usesRegExp)
            realAfter = Utils::expandRegExpReplacement(after, match.capturedTexts());
        else if (preserveCase)
            realAfter = Utils::matchCaseReplacement(found.selectedText(), after);
        else
            realAfter = after;
        insertTextAfterSelection(realAfter, editCursor);
        found = findOne(regexp, editCursor, Utils::textDocumentFlagsForFindFlags(findFlags));
    }
    editCursor.endEditBlock();
    return count;
}

bool BaseTextFindBase::find(const QString &txt, FindFlags findFlags, QTextCursor start, bool *wrapped)
{
    if (txt.isEmpty()) {
        setTextCursor(start);
        return true;
    }
    QRegularExpression regexp = regularExpression(txt, findFlags);
    QTextCursor found = findOne(regexp, start, Utils::textDocumentFlagsForFindFlags(findFlags));
    if (wrapped)
        *wrapped = false;

    if (found.isNull()) {
        if ((findFlags & FindBackward) == 0)
            start.movePosition(QTextCursor::Start);
        else
            start.movePosition(QTextCursor::End);
        found = findOne(regexp, start, Utils::textDocumentFlagsForFindFlags(findFlags));
        if (found.isNull())
            return false;
        if (wrapped)
            *wrapped = true;
    }
    setTextCursor(found);
    return true;
}

QTextCursor BaseTextFindBase::findOne(
    const QRegularExpression &expr, QTextCursor from, QTextDocument::FindFlags options) const
{
    QTextCursor found = document()->find(expr, from, options);
    while (!found.isNull() && !inScope(found)) {
        if (!found.hasSelection()) {
            if (found.movePosition(options & QTextDocument::FindBackward
                                       ? QTextCursor::PreviousCharacter
                                       : QTextCursor::NextCharacter)) {
                from = found;
            } else {
                return {};
            }
        } else {
            from.setPosition(options & QTextDocument::FindBackward ? found.selectionStart()
                                                                   : found.selectionEnd());
        }
        found = document()->find(expr, from, options);
    }

    return found;
}

bool BaseTextFindBase::inScope(const QTextCursor &candidate) const
{
    if (candidate.isNull())
        return false;
    if (d->m_scope.isNull())
        return true;
    return inScope(candidate.selectionStart(), candidate.selectionEnd());
}

bool BaseTextFindBase::inScope(int candidateStart, int candidateEnd) const
{
    if (d->m_scope.isNull())
        return true;
    if (candidateStart > candidateEnd)
        std::swap(candidateStart, candidateEnd);
    return Utils::anyOf(d->m_scope, [&](const QTextCursor &scope) {
        return candidateStart >= scope.selectionStart() && candidateEnd <= scope.selectionEnd();
    });
}

/*!
    \reimp
*/
void BaseTextFindBase::defineFindScope()
{
    Utils::MultiTextCursor multiCursor = multiTextCursor();
    bool foundSelection = false;
    for (const QTextCursor &c : multiCursor) {
        if (c.hasSelection()) {
            if (foundSelection || c.block() != c.document()->findBlock(c.anchor())) {
                d->m_scope = Utils::MultiTextCursor(Utils::sorted(multiCursor.cursors()));
                QTextCursor cursor = textCursor();
                cursor.clearSelection();
                setTextCursor(cursor);
                emit findScopeChanged(d->m_scope);
                return;
            }
            foundSelection = true;
        }
    }
    clearFindScope();
}

/*!
    \reimp
*/
void BaseTextFindBase::clearFindScope()
{
    d->m_scope = Utils::MultiTextCursor();
    emit findScopeChanged(d->m_scope);
}

/*!
    \reimp
    Emits highlightAllRequested().
*/
void BaseTextFindBase::highlightAll(const QString &txt, FindFlags findFlags)
{
    emit highlightAllRequested(txt, findFlags);
}

void BaseTextFindBase::setMultiTextCursorProvider(const CursorProvider &provider)
{
    d->m_cursorProvider = provider;
}

} // namespace Core

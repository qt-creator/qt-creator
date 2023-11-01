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

QRegularExpression BaseTextFind::regularExpression(const QString &txt, FindFlags flags)
{
    return QRegularExpression((flags & FindRegularExpression) ? txt
                                                              : QRegularExpression::escape(txt),
                              (flags & FindCaseSensitively)
                                  ? QRegularExpression::NoPatternOption
                                  : QRegularExpression::CaseInsensitiveOption);
}

struct BaseTextFindPrivate
{
    explicit BaseTextFindPrivate(QPlainTextEdit *editor);
    explicit BaseTextFindPrivate(QTextEdit *editor);

    QPointer<QTextEdit> m_editor;
    QPointer<QPlainTextEdit> m_plaineditor;
    QPointer<QWidget> m_widget;
    Utils::MultiTextCursor m_scope;
    std::function<Utils::MultiTextCursor()> m_cursorProvider;
    int m_incrementalStartPos;
    bool m_incrementalWrappedState;
};

BaseTextFindPrivate::BaseTextFindPrivate(QTextEdit *editor)
    : m_editor(editor)
    , m_widget(editor)
    , m_incrementalStartPos(-1)
    , m_incrementalWrappedState(false)
{
}

BaseTextFindPrivate::BaseTextFindPrivate(QPlainTextEdit *editor)
    : m_plaineditor(editor)
    , m_widget(editor)
    , m_incrementalStartPos(-1)
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
    \fn void Core::BaseTextFind::findScopeChanged(const Utils::MultiTextCursor &cursor)

    This signal is emitted when the search
    scope changes to \a cursor.
*/

/*!
    \fn void Core::BaseTextFind::highlightAllRequested(const QString &txt, Utils::FindFlags findFlags)

    This signal is emitted when the search results for \a txt using the given
    \a findFlags should be highlighted in the editor widget.
*/

/*!
    \internal
*/
BaseTextFind::BaseTextFind(QTextEdit *editor)
    : d(new BaseTextFindPrivate(editor))
{
}

/*!
    \internal
*/
BaseTextFind::BaseTextFind(QPlainTextEdit *editor)
    : d(new BaseTextFindPrivate(editor))
{
}

/*!
    \internal
*/
BaseTextFind::~BaseTextFind()
{
    delete d;
}

QTextCursor BaseTextFind::textCursor() const
{
    QTC_ASSERT(d->m_editor || d->m_plaineditor, return QTextCursor());
    return d->m_editor ? d->m_editor->textCursor() : d->m_plaineditor->textCursor();
}

void BaseTextFind::setTextCursor(const QTextCursor &cursor)
{
    QTC_ASSERT(d->m_editor || d->m_plaineditor, return);
    d->m_editor ? d->m_editor->setTextCursor(cursor) : d->m_plaineditor->setTextCursor(cursor);
}

QTextDocument *BaseTextFind::document() const
{
    QTC_ASSERT(d->m_editor || d->m_plaineditor, return nullptr);
    return d->m_editor ? d->m_editor->document() : d->m_plaineditor->document();
}

bool BaseTextFind::isReadOnly() const
{
    QTC_ASSERT(d->m_editor || d->m_plaineditor, return true);
    return d->m_editor ? d->m_editor->isReadOnly() : d->m_plaineditor->isReadOnly();
}

/*!
    \reimp
*/
bool BaseTextFind::supportsReplace() const
{
    return !isReadOnly();
}

/*!
    \reimp
*/
FindFlags BaseTextFind::supportedFindFlags() const
{
    return FindBackward | FindCaseSensitively | FindRegularExpression
            | FindWholeWords | FindPreserveCase;
}

/*!
    \reimp
*/
void BaseTextFind::resetIncrementalSearch()
{
    d->m_incrementalStartPos = -1;
    d->m_incrementalWrappedState = false;
}

/*!
    \reimp
*/
void BaseTextFind::clearHighlights()
{
    highlightAll(QString(), {});
}

/*!
    \reimp
*/
QString BaseTextFind::currentFindString() const
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
        for (const QChar c : s) {
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
QString BaseTextFind::completedFindString() const
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(textCursor().selectionStart());
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    return cursor.selectedText();
}

/*!
    \reimp
*/
IFindSupport::Result BaseTextFind::findIncremental(const QString &txt, FindFlags findFlags)
{
    QTextCursor cursor = textCursor();
    if (d->m_incrementalStartPos < 0)
        d->m_incrementalStartPos = cursor.selectionStart();
    cursor.setPosition(d->m_incrementalStartPos);
    bool wrapped = false;
    bool found =  find(txt, findFlags, cursor, &wrapped);
    if (wrapped != d->m_incrementalWrappedState && found) {
        d->m_incrementalWrappedState = wrapped;
        showWrapIndicator(d->m_widget);
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
IFindSupport::Result BaseTextFind::findStep(const QString &txt, FindFlags findFlags)
{
    bool wrapped = false;
    bool found = find(txt, findFlags, textCursor(), &wrapped);
    if (wrapped)
        showWrapIndicator(d->m_widget);
    if (found) {
        d->m_incrementalStartPos = textCursor().selectionStart();
        d->m_incrementalWrappedState = false;
    }
    return found ? Found : NotFound;
}

/*!
    \reimp
*/
void BaseTextFind::replace(const QString &before, const QString &after, FindFlags findFlags)
{
    QTextCursor cursor = replaceInternal(before, after, findFlags);
    setTextCursor(cursor);
}

// QTextCursor::insert moves all other QTextCursors that are the the insertion point forward.
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

QTextCursor BaseTextFind::replaceInternal(const QString &before, const QString &after,
                                          FindFlags findFlags)
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

Utils::MultiTextCursor BaseTextFind::multiTextCursor() const
{
    if (d->m_cursorProvider)
        return d->m_cursorProvider();
    return Utils::MultiTextCursor({textCursor()});
}

/*!
    \reimp
*/
bool BaseTextFind::replaceStep(const QString &before, const QString &after, FindFlags findFlags)
{
    QTextCursor cursor = replaceInternal(before, after, findFlags);
    bool wrapped = false;
    bool found = find(before, findFlags, cursor, &wrapped);
    if (wrapped)
        showWrapIndicator(d->m_widget);
    return found;
}

/*!
    \reimp
    Returns the number of search hits replaced.
*/
int BaseTextFind::replaceAll(const QString &before, const QString &after, FindFlags findFlags)
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

bool BaseTextFind::find(const QString &txt, FindFlags findFlags, QTextCursor start, bool *wrapped)
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

QTextCursor BaseTextFind::findOne(const QRegularExpression &expr,
                                  QTextCursor from,
                                  QTextDocument::FindFlags options) const
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

bool BaseTextFind::inScope(const QTextCursor &candidate) const
{
    if (candidate.isNull())
        return false;
    if (d->m_scope.isNull())
        return true;
    return inScope(candidate.selectionStart(), candidate.selectionEnd());
}

bool BaseTextFind::inScope(int candidateStart, int candidateEnd) const
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
void BaseTextFind::defineFindScope()
{
    Utils::MultiTextCursor multiCursor = multiTextCursor();
    bool foundSelection = false;
    for (const QTextCursor &c : multiCursor) {
        if (c.hasSelection()) {
            if (foundSelection || c.block() != c.document()->findBlock(c.anchor())) {
                const QList<QTextCursor> sortedCursors = Utils::sorted(multiCursor.cursors());
                d->m_scope = Utils::MultiTextCursor(sortedCursors);
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
void BaseTextFind::clearFindScope()
{
    d->m_scope = Utils::MultiTextCursor();
    emit findScopeChanged(d->m_scope);
}

/*!
    \reimp
    Emits highlightAllRequested().
*/
void BaseTextFind::highlightAll(const QString &txt, FindFlags findFlags)
{
    emit highlightAllRequested(txt, findFlags);
}

void BaseTextFind::setMultiTextCursorProvider(const CursorProvider &provider)
{
    d->m_cursorProvider = provider;
}

} // namespace Core

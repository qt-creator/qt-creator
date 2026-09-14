// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "spellcheckmenu.h"

#include "syntaxhighlighter.h"
#include "texteditortr.h"

#include <utils/spellchecker.h>

#include <QAction>
#include <QMenu>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextLayout>

using namespace Utils;

namespace TextEditor {

// The word the highlighter has marked as misspelled at the position of cursor, null when
// there is none. The marks come off the text layout rather than out of the dictionary
// again: only the highlighter knows which parts of a text are prose and which word is
// being written right now, and the menu is to offer a correction for exactly the words
// the editor shows one for.
static QTextCursor misspelledWord(const QTextCursor &cursor)
{
    const QTextBlock block = cursor.block();
    const QTextLayout *layout = block.layout();
    if (!block.isValid() || !layout)
        return {};

    const int position = cursor.position() - block.position();
    const QList<QTextLayout::FormatRange> ranges = layout->formats();
    for (const QTextLayout::FormatRange &range : ranges) {
        if (position < range.start || position > range.start + range.length)
            continue;
        if (!SyntaxHighlighter::isSpellingError(range.format))
            continue;
        QTextCursor word = cursor;
        word.setPosition(block.position() + range.start);
        word.setPosition(block.position() + range.start + range.length, QTextCursor::KeepAnchor);
        return word;
    }
    return {};
}

void addSpellingActions(QMenu *menu, SyntaxHighlighter *highlighter, const QTextCursor &cursor)
{
    if (!highlighter)
        return;
    const QString language = highlighter->spellCheckLanguage();
    if (language.isEmpty())
        return;

    QTextCursor word = misspelledWord(cursor);
    if (word.isNull())
        return;
    const QString text = word.selectedText();

    const QList<QAction *> actions = menu->actions();
    QAction *before = actions.isEmpty() ? nullptr : actions.first();

    SpellChecker *checker = SpellChecker::instance();
    const QStringList suggestions = checker->suggestions(text, language);
    for (const QString &suggestion : suggestions) {
        auto action = new QAction(suggestion, menu);
        QObject::connect(action, &QAction::triggered, word.document(),
                         [word, suggestion]() mutable { word.insertText(suggestion); });
        menu->insertAction(before, action);
    }
    if (suggestions.isEmpty()) {
        auto action = new QAction(Tr::tr("No Spelling Suggestions"), menu);
        action->setEnabled(false);
        menu->insertAction(before, action);
    }
    menu->insertSeparator(before);

    auto learn = new QAction(Tr::tr("Add \"%1\" to Dictionary").arg(text), menu);
    QObject::connect(learn, &QAction::triggered, menu, [text, language, checker] {
        checker->learnWord(text, language);
    });
    menu->insertAction(before, learn);

    auto ignore = new QAction(Tr::tr("Ignore \"%1\"").arg(text), menu);
    QObject::connect(ignore, &QAction::triggered, menu, [text, language, checker] {
        checker->ignoreWord(text, language);
    });
    menu->insertAction(before, ignore);
    menu->insertSeparator(before);
}

} // namespace TextEditor

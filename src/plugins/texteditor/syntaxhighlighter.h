// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <texteditor/texteditorconstants.h>

#include <QObject>
#include <QTextLayout>

#include <functional>
#include <climits>
#include <utility>

QT_BEGIN_NAMESPACE
class QTextDocument;
class QSyntaxHighlighterPrivate;
class QTextCharFormat;
class QFont;
class QColor;
class QTextBlockUserData;
class QTextEdit;
QT_END_NAMESPACE

namespace TextEditor {

class FontSettingsData;
class SyntaxHighlighterPrivate;

class TEXTEDITOR_EXPORT SyntaxHighlighter : public QObject
{
    Q_OBJECT
public:
    SyntaxHighlighter(QObject *parent = nullptr);
    SyntaxHighlighter(QTextDocument *parent);
    SyntaxHighlighter(QTextEdit *parent);
    ~SyntaxHighlighter() override;

    void setDocument(QTextDocument *doc);
    QTextDocument *document() const;

    void setMimeType(const QString &mimeType);
    QString mimeType() const;

    static QList<QColor> generateColors(int n, const QColor &background);

    // Don't call in constructors of derived classes
    virtual void setFontSettings(const TextEditor::FontSettingsData &fontSettings);
    TextEditor::FontSettingsData fontSettings() const;

    void setExtraFormats(const QTextBlock &block, const QList<QTextLayout::FormatRange> &formats);
    virtual void setLanguageFeaturesFlags(unsigned int /*flags*/) {}; // needed for CppHighlighting
    virtual void setEnabled(bool /*enabled*/) {}; // needed for DiffAndLogHighlighter

    bool syntaxHighlighterUpToDate() const;

    void setIgnoreFolding(bool ignore);
    bool ignoresFolding() const;

    // An empty language turns spell checking off, which is the default.
    void setSpellCheckLanguage(const QString &language);
    QString spellCheckLanguage() const;
    // Whether the strings of a text are prose too, which they are not by default.
    void setSpellCheckStrings(bool check);
    bool spellCheckStrings() const;
    // A word that the text cursor is on is being written and is not marked as
    // misspelled. Pass -1 for no such word.
    void setSpellCheckCursorPosition(int position);
    // The blocks that viewer shows, as the ranges of block numbers they span. Asking
    // the dictionary about a block costs a call into the spell checking service of the
    // platform, too much to spend on a block nobody is looking at, so only the prose of
    // a block some viewer shows is checked. A viewer that leaves blocks out in the
    // middle of what it shows - the unchanged lines an inline diff collapses - reports
    // a range per run of the blocks it does show. A block a fold hides takes up a
    // number in a range without being shown, and is left unchecked until the fold
    // opens. No range at all says that the viewer shows none of them for now. A
    // highlighter that no viewer registered with checks every block, which is what a
    // text without a viewport of its own - a submit message - needs.
    void setVisibleBlocks(QObject *viewer, const QList<std::pair<int, int>> &ranges);
    // viewer shows this document no more. The marks it asked for stay where they are:
    // a block carries them until it is highlighted again.
    void removeViewer(QObject *viewer);
    // Whether a format is the one a misspelled word is marked with.
    static bool isSpellingError(const QTextCharFormat &format);

public slots:
    virtual void rehighlight();
    virtual void scheduleRehighlight();
    void rehighlightBlock(const QTextBlock &block);
    void clearExtraFormats(const QTextBlock &block);
    void reformatBlocks(int from, int charsRemoved, int charsAdded);
    void clearAllExtraFormats();

protected:
    void setDefaultTextFormatCategories();
    void setTextFormatCategories(int count, std::function<TextStyle(int)> formatMapping);
    QTextCharFormat formatForCategory(int categoryIndex) const;
    QTextCharFormat whitespacified(const QTextCharFormat &fmt);
    QTextCharFormat asSyntaxHighlight(const QTextCharFormat &fmt);

    // implement in subclasses
    // default implementation highlights whitespace
    virtual void highlightBlock(const QString &text);

    void setFormat(int start, int count, const QTextCharFormat &format);
    void setFormat(int start, int count, const QColor &color);
    void setFormat(int start, int count, const QFont &font);
    QTextCharFormat format(int pos) const;

    void formatSpaces(const QString &text, int start = 0, int count = INT_MAX);
    // Marks the count characters at start of the current text block as prose, the part
    // of a text a dictionary has something to say about: the comments of a source file,
    // or the whole of a text that is prose to begin with.
    void addProseRange(int start, int count);
    void spellCheck(const QString &text);
    void setFormatWithSpaces(const QString &text, int start, int count,
                             const QTextCharFormat &format);

    int previousBlockState() const;
    int currentBlockState() const;
    void setCurrentBlockState(int newState);

    void setCurrentBlockUserData(QTextBlockUserData *data);
    QTextBlockUserData *currentBlockUserData() const;

    QTextBlock currentBlock() const;
    void forceRehighlightBlock(const QTextBlock &block);

    void setFoldingIndent(const QTextBlock &block, int indent);
    void setFoldingStartIncluded(const QTextBlock &block, bool included);
    void setFoldingEndIncluded(const QTextBlock &block, bool included);

    virtual void documentChanged(QTextDocument * /*oldDoc*/, QTextDocument * /*newDoc*/) {};

signals:
    void finished();

private:
    void setTextFormatCategories(const QList<std::pair<int, TextStyle>> &categories);
    void delayedRehighlight();
    void continueRehighlight();

    friend class SyntaxHighlighterPrivate;
    std::unique_ptr<SyntaxHighlighterPrivate> d;

#ifdef WITH_TESTS
    friend class tst_highlighter;
    SyntaxHighlighter(QTextDocument *parent, const FontSettingsData &fontsettings);
#endif
};

} // namespace TextEditor

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <texteditor/texteditorconstants.h>
#include <texteditor/textdocumentlayout.h>

#include <KSyntaxHighlighting/Definition>

#include <QObject>
#include <QTextLayout>

#include <functional>
#include <climits>

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

class FontSettings;
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
    virtual void setFontSettings(const TextEditor::FontSettings &fontSettings);
    TextEditor::FontSettings fontSettings() const;

    void setExtraFormats(const QTextBlock &block, const QList<QTextLayout::FormatRange> &formats);
    virtual void setLanguageFeaturesFlags(unsigned int /*flags*/) {}; // needed for CppHighlighting
    virtual void setEnabled(bool /*enabled*/) {}; // needed for DiffAndLogHighlighter
    virtual void setDefinitionName(const QString & /*definitionName*/) {} // needed for Highlighter

    bool syntaxHighlighterUpToDate() const;

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
    void setFormatWithSpaces(const QString &text, int start, int count,
                             const QTextCharFormat &format);

    int previousBlockState() const;
    int currentBlockState() const;
    void setCurrentBlockState(int newState);

    void setCurrentBlockUserData(QTextBlockUserData *data);
    QTextBlockUserData *currentBlockUserData() const;

    QTextBlock currentBlock() const;

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
    SyntaxHighlighter(QTextDocument *parent, const FontSettings &fontsettings);
#endif
};

} // namespace TextEditor

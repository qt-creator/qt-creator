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
    Q_DECLARE_PRIVATE(SyntaxHighlighter)
public:
    SyntaxHighlighter(QObject *parent = nullptr);
    SyntaxHighlighter(QTextDocument *parent);
    SyntaxHighlighter(QTextEdit *parent);
    ~SyntaxHighlighter() override;

    void setDocument(QTextDocument *doc);
    QTextDocument *document() const;

    static QList<QColor> generateColors(int n, const QColor &background);

    // Don't call in constructors of derived classes
    virtual void setFontSettings(const TextEditor::FontSettings &fontSettings);
    TextEditor::FontSettings fontSettings() const;

    void setNoAutomaticHighlighting(bool noAutomatic);

    struct Result
    {
        void fillByBlock(const QTextBlock &block)
        {
            m_blockNumber = block.position();
            m_userState = block.userState();

            TextBlockUserData *userDate = TextDocumentLayout::textUserData(block);
            if (!userDate)
                return;

            m_hasBlockUserData = true;
            m_foldingIndent = userDate->foldingIndent();
            m_folded = userDate->folded();
            m_ifdefedOut = userDate->ifdefedOut();
            m_foldingStartIncluded = userDate->foldingStartIncluded();
            m_foldingEndIncluded = userDate->foldingEndIncluded();
            m_parentheses = userDate->parentheses();
            m_expectedRawStringSuffix = userDate->expectedRawStringSuffix();
        }

        void copyToBlock(QTextBlock &block) const
        {
            block.setUserState(m_userState);

            if (m_hasBlockUserData) {
                TextBlockUserData *data = TextDocumentLayout::userData(block);
                data->setExpectedRawStringSuffix(m_expectedRawStringSuffix);
                data->setFolded(m_folded);
                data->setFoldingIndent(m_foldingIndent);
                data->setFoldingStartIncluded(m_foldingStartIncluded);
                data->setFoldingEndIncluded(m_foldingEndIncluded);

                if (m_ifdefedOut)
                    data->setIfdefedOut();
                else
                    data->clearIfdefedOut();

                data->setParentheses(m_parentheses);
            }
        }

        int m_blockNumber;
        bool m_hasBlockUserData = false;

        int m_foldingIndent : 16;
        uint m_folded : 1;
        uint m_ifdefedOut : 1;
        uint m_foldingStartIncluded : 1;
        uint m_foldingEndIncluded : 1;

        Parentheses m_parentheses;
        QByteArray m_expectedRawStringSuffix;
        int m_userState = -1;
        QList<QTextLayout::FormatRange> m_formatRanges;
    };

    void setExtraFormats(const QTextBlock &block, const QList<QTextLayout::FormatRange> &formats);
    virtual void setLanguageFeaturesFlags(unsigned int /*flags*/) {}; // needed for CppHighlighting
    virtual void setEnabled(bool /*enabled*/) {}; // needed for DiffAndLogHighlighter
    virtual KSyntaxHighlighting::Definition getDefinition() { return {}; }

public slots:
    virtual void rehighlight();
    void rehighlightBlock(const QTextBlock &block);
    void clearExtraFormats(const QTextBlock &block);
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

protected:
    virtual void documentChanged(QTextDocument * /*oldDoc*/, QTextDocument * /*newDoc*/) {};

signals:
    void resultsReady(const QList<Result> &result);

private:
    void setTextFormatCategories(const QList<std::pair<int, TextStyle>> &categories);
    void reformatBlocks(int from, int charsRemoved, int charsAdded);
    void delayedRehighlight();

    QScopedPointer<SyntaxHighlighterPrivate> d_ptr;

#ifdef WITH_TESTS
    friend class tst_highlighter;
    SyntaxHighlighter(QTextDocument *parent, const FontSettings &fontsettings);
#endif
};

} // namespace TextEditor

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diffandloghighlighter.h"

#include <texteditor/textdocumentlayout.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QRegularExpression>

/*!
    \class VcsBase::DiffAndLogHighlighter

    \brief The DiffAndLogHighlighter class provides a highlighter for diffs and log editors.

    Diff is parametrizable by the file indicator, which is for example '^====' in case of p4:
    \code
    ==== //depot/research/main/qdynamicmainwindow3/qdynamicdockwidgetlayout_p.h#34 (text) ====
    \endcode

    Or  '--- a/|'+++ b/' in case of git:
    \code
    diff --git a/src/plugins/plugins.pro b/src/plugins/plugins.pro
    index 9401ee7..ef35c3b 100644
    --- a/src/plugins/plugins.pro
    +++ b/src/plugins/plugins.pro
    @@ -10,6 +10,7 @@ SUBDIRS   = plugin_coreplugin
    \endcode

    Log is parametrizable by change indicator. For example '^commit ([0-9a-f]{8})[0-9a-f]{32}'
    in Git:
    \code
    commit a3398841a24b24c73b47759c4bffdc8b78a34936 (HEAD, master)
    \code

    Also highlights trailing blanks.
 */

static const int BASE_LEVEL = 0;
static const int FILE_LEVEL = 1;
static const int LOCATION_LEVEL = 2;

namespace VcsBase {
namespace Internal {

enum FoldingState {
    StartOfFile,
    Header,
    File,
    Location
};

} // namespace Internal;

static inline QTextCharFormat invertedColorFormat(const QTextCharFormat &in)
{
    QTextCharFormat rc = in;
    rc.setForeground(in.background());
    rc.setBackground(in.foreground());
    return rc;
}

// --- DiffAndLogHighlighterPrivate
class DiffAndLogHighlighterPrivate
{
public:
    DiffAndLogHighlighterPrivate(DiffAndLogHighlighter *q_,
                                 const QRegularExpression &filePattern,
                                 const QRegularExpression &changePattern) :
        q(q_),
        m_filePattern(filePattern),
        m_changePattern(changePattern),
        m_locationIndicator(QLatin1String("@@")),
        m_diffInIndicator(QLatin1Char('+')),
        m_diffOutIndicator(QLatin1Char('-')),
        m_foldingState(Internal::StartOfFile)
    {
        QTC_CHECK(filePattern.isValid());
    }

    TextEditor::TextStyle analyzeLine(const QString &block) const;
    void updateOtherFormats();

    DiffAndLogHighlighter *const q;

    const QRegularExpression m_filePattern;
    const QRegularExpression m_changePattern;
    const QString m_locationIndicator;
    const QChar m_diffInIndicator;
    const QChar m_diffOutIndicator;
    QTextCharFormat m_addedTrailingWhiteSpaceFormat;

    Internal::FoldingState m_foldingState;
    bool m_enabled = true;
};

TextEditor::TextStyle DiffAndLogHighlighterPrivate::analyzeLine(const QString &text) const
{
    // Do not match on git "--- a/" as a deleted line, check
    // file first
    if (m_filePattern.match(text).capturedStart() == 0)
        return TextEditor::C_DIFF_FILE;
    if (m_changePattern.match(text).capturedStart() == 0)
        return TextEditor::C_LOG_CHANGE_LINE;
    if (text.startsWith(m_diffInIndicator))
        return TextEditor::C_ADDED_LINE;
    if (text.startsWith(m_diffOutIndicator))
        return TextEditor::C_REMOVED_LINE;
    if (text.startsWith(m_locationIndicator))
        return TextEditor::C_DIFF_LOCATION;
    return TextEditor::C_TEXT;
}

void DiffAndLogHighlighterPrivate::updateOtherFormats()
{
    m_addedTrailingWhiteSpaceFormat =
            invertedColorFormat(q->formatForCategory(TextEditor::C_ADDED_LINE));

}

// --- DiffAndLogHighlighter
DiffAndLogHighlighter::DiffAndLogHighlighter(const QRegularExpression &filePattern,
                                             const QRegularExpression &changePattern) :
    TextEditor::SyntaxHighlighter(static_cast<QTextDocument *>(nullptr)),
    d(new DiffAndLogHighlighterPrivate(this, filePattern, changePattern))
{
    setDefaultTextFormatCategories();
    d->updateOtherFormats();
}

DiffAndLogHighlighter::~DiffAndLogHighlighter()
{
    delete d;
}

// Check trailing spaces
static inline int trimmedLength(const QString &in)
{
    for (int pos = in.length() - 1; pos >= 0; pos--)
        if (!in.at(pos).isSpace())
            return pos + 1;
    return 0;
}

/*
 * This sets the folding indent:
 * 0 for the first line of the diff header.
 * 1 for all the following lines of the diff header and all @@ lines.
 * 2 for everything else
 */
void DiffAndLogHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty())
        return;

    const int length = text.length();
    const TextEditor::TextStyle format = d->analyzeLine(text);

    if (d->m_enabled) {
        if (format == TextEditor::C_ADDED_LINE) {
            // Mark trailing whitespace.
            const int trimmedLen = trimmedLength(text);
            setFormatWithSpaces(text, 0, trimmedLen, formatForCategory(format));
            if (trimmedLen != length)
                setFormat(trimmedLen, length - trimmedLen, d->m_addedTrailingWhiteSpaceFormat);
        } else if (format != TextEditor::C_TEXT) {
            setFormatWithSpaces(text, 0, length, formatForCategory(format));
        } else {
            formatSpaces(text);
        }
    }

    // codefolding:
    TextEditor::TextBlockUserData *data =
            TextEditor::TextBlockUserData::userData(currentBlock());
    QTC_ASSERT(data, return; );
    if (!TextEditor::TextBlockUserData::textUserData(currentBlock().previous()))
        d->m_foldingState = Internal::StartOfFile;

    switch (d->m_foldingState) {
    case Internal::StartOfFile:
    case Internal::Header:
        if (format == TextEditor::C_DIFF_FILE) {
            d->m_foldingState = Internal::File;
            TextEditor::TextBlockUserData::setFoldingIndent(currentBlock(), BASE_LEVEL);
        } else if (format == TextEditor::C_DIFF_LOCATION) {
            d->m_foldingState = Internal::Location;
            TextEditor::TextBlockUserData::setFoldingIndent(currentBlock(), FILE_LEVEL);
        } else {
            d->m_foldingState = Internal::Header;
            TextEditor::TextBlockUserData::setFoldingIndent(currentBlock(), BASE_LEVEL);
        }
        break;
    case Internal::File:
        if (format == TextEditor::C_DIFF_FILE) {
            TextEditor::TextBlockUserData::setFoldingIndent(currentBlock(), FILE_LEVEL);
        } else if (format == TextEditor::C_DIFF_LOCATION) {
            d->m_foldingState = Internal::Location;
            TextEditor::TextBlockUserData::setFoldingIndent(currentBlock(), FILE_LEVEL);
        } else {
            TextEditor::TextBlockUserData::setFoldingIndent(currentBlock(), FILE_LEVEL);
        }
        break;
    case Internal::Location:
        if (format == TextEditor::C_DIFF_FILE) {
            d->m_foldingState = Internal::File;
            TextEditor::TextBlockUserData::setFoldingIndent(currentBlock(), BASE_LEVEL);
        } else if (format == TextEditor::C_DIFF_LOCATION) {
            TextEditor::TextBlockUserData::setFoldingIndent(currentBlock(), FILE_LEVEL);
        } else {
            TextEditor::TextBlockUserData::setFoldingIndent(currentBlock(), LOCATION_LEVEL);
        }
        break;
    }
}

void DiffAndLogHighlighter::setFontSettings(const TextEditor::FontSettings &fontSettings)
{
    SyntaxHighlighter::setFontSettings(fontSettings);
    d->updateOtherFormats();
}

void DiffAndLogHighlighter::setEnabled(bool enabled)
{
    d->m_enabled = enabled;
}

} // namespace VcsBase

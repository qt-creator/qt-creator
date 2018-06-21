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

#include "diffandloghighlighter.h"

#include <texteditor/textdocumentlayout.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QRegExp>

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
    DiffAndLogHighlighterPrivate(DiffAndLogHighlighter *q_, const QRegExp &filePattern,
                                 const QRegExp &changePattern) :
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

    mutable QRegExp m_filePattern;
    mutable QRegExp m_changePattern;
    const QString m_locationIndicator;
    const QChar m_diffInIndicator;
    const QChar m_diffOutIndicator;
    QTextCharFormat m_addedTrailingWhiteSpaceFormat;

    Internal::FoldingState m_foldingState;
};

TextEditor::TextStyle DiffAndLogHighlighterPrivate::analyzeLine(const QString &text) const
{
    // Do not match on git "--- a/" as a deleted line, check
    // file first
    if (m_filePattern.indexIn(text) == 0)
        return TextEditor::C_DIFF_FILE;
    if (m_changePattern.indexIn(text) == 0)
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
DiffAndLogHighlighter::DiffAndLogHighlighter(const QRegExp &filePattern, const QRegExp &changePattern) :
    TextEditor::SyntaxHighlighter(static_cast<QTextDocument *>(0)),
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

    // codefolding:
    TextEditor::TextBlockUserData *data =
            TextEditor::TextDocumentLayout::userData(currentBlock());
    QTC_ASSERT(data, return; );
    if (!TextEditor::TextDocumentLayout::testUserData(currentBlock().previous()))
        d->m_foldingState = Internal::StartOfFile;

    switch (d->m_foldingState) {
    case Internal::StartOfFile:
    case Internal::Header:
        if (format == TextEditor::C_DIFF_FILE) {
            d->m_foldingState = Internal::File;
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), BASE_LEVEL);
        } else if (format == TextEditor::C_DIFF_LOCATION) {
            d->m_foldingState = Internal::Location;
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
        } else {
            d->m_foldingState = Internal::Header;
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), BASE_LEVEL);
        }
        break;
    case Internal::File:
        if (format == TextEditor::C_DIFF_FILE) {
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
        } else if (format == TextEditor::C_DIFF_LOCATION) {
            d->m_foldingState = Internal::Location;
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
        } else {
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
        }
        break;
    case Internal::Location:
        if (format == TextEditor::C_DIFF_FILE) {
            d->m_foldingState = Internal::File;
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), BASE_LEVEL);
        } else if (format == TextEditor::C_DIFF_LOCATION) {
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
        } else {
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), LOCATION_LEVEL);
        }
        break;
    }
}

void DiffAndLogHighlighter::setFontSettings(const TextEditor::FontSettings &fontSettings)
{
    SyntaxHighlighter::setFontSettings(fontSettings);
    d->updateOtherFormats();
}

} // namespace VcsBase

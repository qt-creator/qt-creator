/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

// Formats used by DiffAndLogHighlighter
enum DiffFormats {
    DiffTextFormat,
    DiffInFormat,
    DiffOutFormat,
    DiffFileFormat,
    DiffLocationFormat,
    ChangeTextFormat
};

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
    DiffAndLogHighlighter *q_ptr;
    Q_DECLARE_PUBLIC(DiffAndLogHighlighter)
public:
    DiffAndLogHighlighterPrivate(const QRegExp &filePattern, const QRegExp &changePattern);

    Internal::DiffFormats analyzeLine(const QString &block) const;
    void updateOtherFormats();

    mutable QRegExp m_filePattern;
    mutable QRegExp m_changePattern;
    const QString m_locationIndicator;
    const QChar m_diffInIndicator;
    const QChar m_diffOutIndicator;
    QTextCharFormat m_addedTrailingWhiteSpaceFormat;

    Internal::FoldingState m_foldingState;
};

DiffAndLogHighlighterPrivate::DiffAndLogHighlighterPrivate(const QRegExp &filePattern, const QRegExp &changePattern) :
    q_ptr(0),
    m_filePattern(filePattern),
    m_changePattern(changePattern),
    m_locationIndicator(QLatin1String("@@")),
    m_diffInIndicator(QLatin1Char('+')),
    m_diffOutIndicator(QLatin1Char('-')),
    m_foldingState(Internal::StartOfFile)
{
    QTC_CHECK(filePattern.isValid());
}

Internal::DiffFormats DiffAndLogHighlighterPrivate::analyzeLine(const QString &text) const
{
    // Do not match on git "--- a/" as a deleted line, check
    // file first
    if (m_filePattern.indexIn(text) == 0)
        return Internal::DiffFileFormat;
    if (m_changePattern.indexIn(text) == 0)
        return Internal::ChangeTextFormat;
    if (text.startsWith(m_diffInIndicator))
        return Internal::DiffInFormat;
    if (text.startsWith(m_diffOutIndicator))
        return Internal::DiffOutFormat;
    if (text.startsWith(m_locationIndicator))
        return Internal::DiffLocationFormat;
    return Internal::DiffTextFormat;
}

void DiffAndLogHighlighterPrivate::updateOtherFormats()
{
    Q_Q(DiffAndLogHighlighter);
    m_addedTrailingWhiteSpaceFormat =
            invertedColorFormat(q->formatForCategory(Internal::DiffInFormat));

}

// --- DiffAndLogHighlighter
DiffAndLogHighlighter::DiffAndLogHighlighter(const QRegExp &filePattern, const QRegExp &changePattern) :
    TextEditor::SyntaxHighlighter(static_cast<QTextDocument *>(0)),
    d_ptr(new DiffAndLogHighlighterPrivate(filePattern, changePattern))
{
    d_ptr->q_ptr = this;
    Q_D(DiffAndLogHighlighter);

    static QVector<TextEditor::TextStyle> categories;
    if (categories.isEmpty()) {
        categories << TextEditor::C_TEXT
                   << TextEditor::C_ADDED_LINE
                   << TextEditor::C_REMOVED_LINE
                   << TextEditor::C_DIFF_FILE
                   << TextEditor::C_DIFF_LOCATION
                   << TextEditor::C_LOG_CHANGE_LINE;
    }
    setTextFormatCategories(categories);
    d->updateOtherFormats();
}

DiffAndLogHighlighter::~DiffAndLogHighlighter()
{
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
    Q_D(DiffAndLogHighlighter);
    if (text.isEmpty())
        return;

    const int length = text.length();
    const Internal::DiffFormats format = d->analyzeLine(text);
    switch (format) {
    case Internal::DiffTextFormat:
        break;
    case Internal::DiffInFormat: {
            // Mark trailing whitespace.
            const int trimmedLen = trimmedLength(text);
            setFormat(0, trimmedLen, formatForCategory(format));
            if (trimmedLen != length)
                setFormat(trimmedLen, length - trimmedLen, d->m_addedTrailingWhiteSpaceFormat);
        }
        break;
    default:
        setFormat(0, length, formatForCategory(format));
        break;
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
        switch (format) {
        case Internal::DiffFileFormat:
            d->m_foldingState = Internal::File;
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), BASE_LEVEL);
            break;
        case Internal::DiffLocationFormat:
            d->m_foldingState = Internal::Location;
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
            break;
        default:
            d->m_foldingState = Internal::Header;
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), BASE_LEVEL);
            break;
        }
        break;
    case Internal::File:
        switch (format) {
        case Internal::DiffFileFormat:
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
            break;
        case Internal::DiffLocationFormat:
            d->m_foldingState = Internal::Location;
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
            break;
        default:
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
            break;
        }
        break;
    case Internal::Location:
        switch (format) {
        case Internal::DiffFileFormat:
            d->m_foldingState = Internal::File;
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), BASE_LEVEL);
            break;
        case Internal::DiffLocationFormat:
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
            break;
        default:
            TextEditor::TextDocumentLayout::setFoldingIndent(currentBlock(), LOCATION_LEVEL);
            break;
        }
        break;
    }
}

void DiffAndLogHighlighter::setFontSettings(const TextEditor::FontSettings &fontSettings)
{
    Q_D(DiffAndLogHighlighter);
    SyntaxHighlighter::setFontSettings(fontSettings);
    d->updateOtherFormats();
}

} // namespace VcsBase

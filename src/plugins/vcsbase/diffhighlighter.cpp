/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "diffhighlighter.h"

#include <texteditor/basetextdocumentlayout.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QtAlgorithms>
#include <QRegExp>
#include <QBrush>

/*!
    \class VcsBase::DiffHighlighter

    \brief A highlighter for diffs.

    Parametrizable by the file indicator, which is for example '^====' in case of p4:
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

    Also highlights trailing blanks.
 */

static const int BASE_LEVEL = 0;
static const int FILE_LEVEL = 1;
static const int LOCATION_LEVEL = 2;

namespace VcsBase {
namespace Internal {

// Formats used by DiffHighlighter
enum DiffFormats {
    DiffTextFormat,
    DiffInFormat,
    DiffOutFormat,
    DiffFileFormat,
    DiffLocationFormat,
    NumDiffFormats
};

enum FoldingState {
    StartOfFile,
    Header,
    File,
    Location
};

// --- DiffHighlighterPrivate
class DiffHighlighterPrivate
{
public:
    DiffHighlighterPrivate(const QRegExp &filePattern);

    DiffFormats analyzeLine(const QString &block) const;

    mutable QRegExp m_filePattern;
    const QString m_locationIndicator;
    const QChar m_diffInIndicator;
    const QChar m_diffOutIndicator;
    QTextCharFormat m_formats[NumDiffFormats];
    QTextCharFormat m_addedTrailingWhiteSpaceFormat;

    FoldingState m_foldingState;
};

DiffHighlighterPrivate::DiffHighlighterPrivate(const QRegExp &filePattern) :
    m_filePattern(filePattern),
    m_locationIndicator(QLatin1String("@@")),
    m_diffInIndicator(QLatin1Char('+')),
    m_diffOutIndicator(QLatin1Char('-')),
    m_foldingState(StartOfFile)
{
    QTC_CHECK(filePattern.isValid());
}

DiffFormats DiffHighlighterPrivate::analyzeLine(const QString &text) const
{
    // Do not match on git "--- a/" as a deleted line, check
    // file first
    if (m_filePattern.indexIn(text) == 0)
        return DiffFileFormat;
    if (text.startsWith(m_diffInIndicator))
        return DiffInFormat;
    if (text.startsWith(m_diffOutIndicator))
        return DiffOutFormat;
    if (text.startsWith(m_locationIndicator))
        return DiffLocationFormat;
    return DiffTextFormat;
}

} // namespace Internal

// --- DiffHighlighter
DiffHighlighter::DiffHighlighter(const QRegExp &filePattern) :
    TextEditor::SyntaxHighlighter(static_cast<QTextDocument *>(0)),
    d(new Internal::DiffHighlighterPrivate(filePattern))
{
}

DiffHighlighter::~DiffHighlighter()
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
void DiffHighlighter::highlightBlock(const QString &text)
{
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
            setFormat(0, trimmedLen, d->m_formats[format]);
            if (trimmedLen != length)
                setFormat(trimmedLen, length - trimmedLen, d->m_addedTrailingWhiteSpaceFormat);
        }
        break;
    default:
        setFormat(0, length, d->m_formats[format]);
        break;
    }

    // codefolding:
    TextEditor::TextBlockUserData *data =
            TextEditor::BaseTextDocumentLayout::userData(currentBlock());
    QTC_ASSERT(data, return; );
    if (!TextEditor::BaseTextDocumentLayout::testUserData(currentBlock().previous()))
        d->m_foldingState = Internal::StartOfFile;

    switch (d->m_foldingState) {
    case Internal::StartOfFile:
    case Internal::Header:
        switch (format) {
        case Internal::DiffFileFormat:
            d->m_foldingState = Internal::File;
            TextEditor::BaseTextDocumentLayout::setFoldingIndent(currentBlock(), BASE_LEVEL);
            break;
        case Internal::DiffLocationFormat:
            d->m_foldingState = Internal::Location;
            TextEditor::BaseTextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
            break;
        default:
            d->m_foldingState = Internal::Header;
            TextEditor::BaseTextDocumentLayout::setFoldingIndent(currentBlock(), BASE_LEVEL);
            break;
        }
        break;
    case Internal::File:
        switch (format) {
        case Internal::DiffFileFormat:
            TextEditor::BaseTextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
            break;
        case Internal::DiffLocationFormat:
            d->m_foldingState = Internal::Location;
            TextEditor::BaseTextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
            break;
        default:
            TextEditor::BaseTextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
            break;
        }
        break;
    case Internal::Location:
        switch (format) {
        case Internal::DiffFileFormat:
            d->m_foldingState = Internal::File;
            TextEditor::BaseTextDocumentLayout::setFoldingIndent(currentBlock(), BASE_LEVEL);
            break;
        case Internal::DiffLocationFormat:
            TextEditor::BaseTextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
            break;
        default:
            TextEditor::BaseTextDocumentLayout::setFoldingIndent(currentBlock(), LOCATION_LEVEL);
            break;
        }
        break;
    }
}

static inline QTextCharFormat invertedColorFormat(const QTextCharFormat &in)
{
    QTextCharFormat rc = in;
    rc.setForeground(in.background());
    rc.setBackground(in.foreground());
    return rc;
}

void DiffHighlighter::setFormats(const QVector<QTextCharFormat> &s)
{
    if (s.size() == Internal::NumDiffFormats) {
        qCopy(s.constBegin(), s.constEnd(), d->m_formats);
        // Display trailing blanks with colors swapped
        d->m_addedTrailingWhiteSpaceFormat =
                invertedColorFormat(d->m_formats[Internal::DiffInFormat]);
    } else {
        qWarning("%s: insufficient setting size: %d", Q_FUNC_INFO, s.size());
    }
}

} // namespace VcsBase

/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "diffhighlighter.h"

#include <texteditor/basetextdocumentlayout.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QtAlgorithms>
#include <QtCore/QRegExp>
#include <QtGui/QBrush>

static const int BASE_LEVEL = 0;
static const int FILE_LEVEL = 1;
static const int LOCATION_LEVEL = 2;

namespace VCSBase {
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
class DiffHighlighterPrivate {
public:
    DiffHighlighterPrivate(const QRegExp &filePattern);
    inline DiffFormats analyzeLine(const QString &block) const;

    const QRegExp m_filePattern;
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
    QTC_ASSERT(filePattern.isValid(), /**/);
}

DiffFormats DiffHighlighterPrivate::analyzeLine(const QString &text) const
{
    // Do not match on git "--- a/" as a deleted line, check
    // file first
    if (m_filePattern.exactMatch(text))
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
DiffHighlighter::DiffHighlighter(const QRegExp &filePattern,
                                 QTextDocument *document) :
    TextEditor::SyntaxHighlighter(document),
    m_d(new Internal::DiffHighlighterPrivate(filePattern))
{
}

DiffHighlighter::~DiffHighlighter()
{
    delete m_d;
}

// Check trailing spaces
static inline int trimmedLength(const QString &in)
{
    for (int pos = in.length() - 1; pos >= 0; pos--)
        if (!in.at(pos).isSpace())
            return pos + 1;
    return 0;
}

void DiffHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty())
        return;

    const int length = text.length();
    const Internal::DiffFormats format = m_d->analyzeLine(text);
    switch (format) {
    case Internal::DiffTextFormat:
        break;
    case Internal::DiffInFormat: {
            // Mark trailing whitespace.
            const int trimmedLen = trimmedLength(text);
            setFormat(0, trimmedLen, m_d->m_formats[format]);
            if (trimmedLen != length)
                setFormat(trimmedLen, length - trimmedLen, m_d->m_addedTrailingWhiteSpaceFormat);
        }
        break;
    default:
        setFormat(0, length, m_d->m_formats[format]);
        break;
    }

    // codefolding:
    TextEditor::TextBlockUserData *data =
            TextEditor::BaseTextDocumentLayout::userData(currentBlock());
    Q_ASSERT(data);
    if (!TextEditor::BaseTextDocumentLayout::testUserData(currentBlock().previous()))
        m_d->m_foldingState = Internal::StartOfFile;

    switch (m_d->m_foldingState) {
    case Internal::StartOfFile:
    case Internal::Header:
        switch (format) {
        case Internal::DiffFileFormat:
            m_d->m_foldingState = Internal::File;
            TextEditor::BaseTextDocumentLayout::setFoldingIndent(currentBlock(), BASE_LEVEL);
            break;
        case Internal::DiffLocationFormat:
            m_d->m_foldingState = Internal::Location;
            TextEditor::BaseTextDocumentLayout::setFoldingIndent(currentBlock(), FILE_LEVEL);
            break;
        default:
            m_d->m_foldingState = Internal::Header;
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
            m_d->m_foldingState = Internal::Location;
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
            m_d->m_foldingState = Internal::File;
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
        qCopy(s.constBegin(), s.constEnd(), m_d->m_formats);
        // Display trailing blanks with colors swapped
        m_d->m_addedTrailingWhiteSpaceFormat =
                invertedColorFormat(m_d->m_formats[Internal::DiffInFormat]);
    } else {
        qWarning("%s: insufficient setting size: %d", Q_FUNC_INFO, s.size());
    }
}

QRegExp DiffHighlighter::filePattern() const
{
    return m_d->m_filePattern;
}

} // namespace VCSBase

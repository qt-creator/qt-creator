/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "diffhighlighter.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QtAlgorithms>
#include <QtCore/QRegExp>

namespace VCSBase {

// Formats used by DiffHighlighter
enum DiffFormats {
    DiffTextFormat,
    DiffInFormat,
    DiffOutFormat,
    DiffFileFormat,
    DiffLocationFormat,
    NumDiffFormats
};

// --- DiffHighlighterPrivate
struct DiffHighlighterPrivate {
    DiffHighlighterPrivate(const QRegExp &filePattern);
    inline DiffFormats analyzeLine(const QString &block) const;

    const QRegExp m_filePattern;
    const QString m_locationIndicator;
    const QChar m_diffInIndicator;
    const QChar m_diffOutIndicator;
    QTextCharFormat m_formats[NumDiffFormats];
};

DiffHighlighterPrivate::DiffHighlighterPrivate(const QRegExp &filePattern) :
    m_filePattern(filePattern),
    m_locationIndicator(QLatin1String("@@")),
    m_diffInIndicator(QLatin1Char('+')),
    m_diffOutIndicator(QLatin1Char('-'))
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

// --- DiffHighlighter
DiffHighlighter::DiffHighlighter(const QRegExp &filePattern,
                                 QTextDocument *document) :
    QSyntaxHighlighter(document),
    m_d(new DiffHighlighterPrivate(filePattern))
{
}

DiffHighlighter::~DiffHighlighter()
{
    delete m_d;
}

void DiffHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty())
        return;

    const DiffFormats format = m_d->analyzeLine(text);
    if (format != DiffTextFormat)
        setFormat(0, text.length(), m_d->m_formats[format]);
}

void DiffHighlighter::setFormats(const QVector<QTextCharFormat> &s)
{
    if (s.size() == NumDiffFormats) {
        qCopy(s.constBegin(), s.constEnd(), m_d->m_formats);
    } else {
        qWarning("%s: insufficient setting size: %d", Q_FUNC_INFO, s.size());
    }
}

} // namespace VCSBase

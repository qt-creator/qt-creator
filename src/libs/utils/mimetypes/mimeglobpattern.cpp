/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mimeglobpattern_p.h"

#include <QRegExp>
#include <QStringList>
#include <QDebug>

using namespace Utils;
using namespace Utils::Internal;

/*!
    \internal
    \class MimeGlobMatchResult
    \inmodule QtCore
    \brief The MimeGlobMatchResult class accumulates results from glob matching.

    Handles glob weights, and preferring longer matches over shorter matches.
*/

void MimeGlobMatchResult::addMatch(const QString &mimeType, int weight, const QString &pattern)
{
    // Is this a lower-weight pattern than the last match? Skip this match then.
    if (weight < m_weight)
        return;
    bool replace = weight > m_weight;
    if (!replace) {
        // Compare the length of the match
        if (pattern.length() < m_matchingPatternLength)
            return; // too short, ignore
        else if (pattern.length() > m_matchingPatternLength) {
            // longer: clear any previous match (like *.bz2, when pattern is *.tar.bz2)
            replace = true;
        }
    }
    if (replace) {
        m_matchingMimeTypes.clear();
        // remember the new "longer" length
        m_matchingPatternLength = pattern.length();
        m_weight = weight;
    }
    if (!m_matchingMimeTypes.contains(mimeType)) {
        m_matchingMimeTypes.append(mimeType);
        if (pattern.startsWith(QLatin1String("*.")))
            m_foundSuffix = pattern.mid(2);
    }
}

/*!
    \internal
    \class MimeGlobPattern
    \inmodule QtCore
    \brief The MimeGlobPattern class contains the glob pattern for file names for MIME type matching.

    \sa MimeType, MimeDatabase, MimeMagicRuleMatcher, MimeMagicRule
*/

bool MimeGlobPattern::matchFileName(const QString &inputFilename) const
{
    // "Applications MUST match globs case-insensitively, except when the case-sensitive
    // attribute is set to true."
    // The constructor takes care of putting case-insensitive patterns in lowercase.
    const QString filename = m_caseSensitivity == Qt::CaseInsensitive ? inputFilename.toLower() : inputFilename;

    const int pattern_len = m_pattern.length();
    if (!pattern_len)
        return false;
    const int len = filename.length();

    const int starCount = m_pattern.count(QLatin1Char('*'));

    // Patterns like "*~", "*.extension"
    if (m_pattern[0] == QLatin1Char('*') && m_pattern.indexOf(QLatin1Char('[')) == -1 && starCount == 1)
    {
        if (len + 1 < pattern_len) return false;

        const QChar *c1 = m_pattern.unicode() + pattern_len - 1;
        const QChar *c2 = filename.unicode() + len - 1;
        int cnt = 1;
        while (cnt < pattern_len && *c1-- == *c2--)
            ++cnt;
        return cnt == pattern_len;
    }

    // Patterns like "README*" (well this is currently the only one like that...)
    if (starCount == 1 && m_pattern.at(pattern_len - 1) == QLatin1Char('*')) {
        if (len + 1 < pattern_len) return false;
        if (m_pattern.at(0) == QLatin1Char('*'))
            return filename.indexOf(m_pattern.mid(1, pattern_len - 2)) != -1;

        const QChar *c1 = m_pattern.unicode();
        const QChar *c2 = filename.unicode();
        int cnt = 1;
        while (cnt < pattern_len && *c1++ == *c2++)
           ++cnt;
        return cnt == pattern_len;
    }

    // Names without any wildcards like "README"
    if (m_pattern.indexOf(QLatin1Char('[')) == -1 && starCount == 0 && m_pattern.indexOf(QLatin1Char('?')))
        return (m_pattern == filename);

    // Other (quite rare) patterns, like "*.anim[1-9j]": use slow but correct method
    QRegExp rx(m_pattern, Qt::CaseSensitive, QRegExp::WildcardUnix);
    return rx.exactMatch(filename);
}

static bool isFastPattern(const QString &pattern)
{
   // starts with "*.", has no other '*' and no other '.'
   return pattern.lastIndexOf(QLatin1Char('*')) == 0
      && pattern.lastIndexOf(QLatin1Char('.')) == 1
      // and contains no other special character
      && !pattern.contains(QLatin1Char('?'))
      && !pattern.contains(QLatin1Char('['))
      ;
}

void MimeAllGlobPatterns::addGlob(const MimeGlobPattern &glob)
{
    const QString &pattern = glob.pattern();
    Q_ASSERT(!pattern.isEmpty());

    // Store each patterns into either m_fastPatternDict (*.txt, *.html etc. with default weight 50)
    // or for the rest, like core.*, *.tar.bz2, *~, into highWeightPatternOffset (>50)
    // or lowWeightPatternOffset (<=50)

    if (glob.weight() == 50 && isFastPattern(pattern) && !glob.isCaseSensitive()) {
        // The bulk of the patterns is *.foo with weight 50 --> those go into the fast patterns hash.
        const QString extension = pattern.mid(2).toLower();
        QStringList &patterns = m_fastPatterns[extension]; // find or create
        if (!patterns.contains(glob.mimeType()))
            patterns.append(glob.mimeType());
    } else {
        if (glob.weight() > 50) {
            if (!m_highWeightGlobs.hasPattern(glob.mimeType(), glob.pattern()))
                m_highWeightGlobs.append(glob);
        } else {
            if (!m_lowWeightGlobs.hasPattern(glob.mimeType(), glob.pattern()))
                m_lowWeightGlobs.append(glob);
        }
    }
}

void MimeAllGlobPatterns::removeMimeType(const QString &mimeType)
{
    QMutableHashIterator<QString, QStringList> it(m_fastPatterns);
    while (it.hasNext()) {
        it.next().value().removeAll(mimeType);
    }
    m_highWeightGlobs.removeMimeType(mimeType);
    m_lowWeightGlobs.removeMimeType(mimeType);
}

void MimeGlobPatternList::match(MimeGlobMatchResult &result,
                                 const QString &fileName) const
{

    MimeGlobPatternList::const_iterator it = this->constBegin();
    const MimeGlobPatternList::const_iterator endIt = this->constEnd();
    for (; it != endIt; ++it) {
        const MimeGlobPattern &glob = *it;
        if (glob.matchFileName(fileName))
            result.addMatch(glob.mimeType(), glob.weight(), glob.pattern());
    }
}

QStringList MimeAllGlobPatterns::matchingGlobs(const QString &fileName, QString *foundSuffix) const
{
    // First try the high weight matches (>50), if any.
    MimeGlobMatchResult result;
    m_highWeightGlobs.match(result, fileName);
    if (result.m_matchingMimeTypes.isEmpty()) {

        // Now use the "fast patterns" dict, for simple *.foo patterns with weight 50
        // (which is most of them, so this optimization is definitely worth it)
        const int lastDot = fileName.lastIndexOf(QLatin1Char('.'));
        if (lastDot != -1) { // if no '.', skip the extension lookup
            const int ext_len = fileName.length() - lastDot - 1;
            const QString simpleExtension = fileName.right(ext_len).toLower();
            // (toLower because fast patterns are always case-insensitive and saved as lowercase)

            const QStringList matchingMimeTypes = m_fastPatterns.value(simpleExtension);
            foreach (const QString &mime, matchingMimeTypes) {
                result.addMatch(mime, 50, QLatin1String("*.") + simpleExtension);
            }
            // Can't return yet; *.tar.bz2 has to win over *.bz2, so we need the low-weight mimetypes anyway,
            // at least those with weight 50.
        }

        // Finally, try the low weight matches (<=50)
        m_lowWeightGlobs.match(result, fileName);
    }
    if (foundSuffix)
        *foundSuffix = result.m_foundSuffix;
    return result.m_matchingMimeTypes;
}

void MimeAllGlobPatterns::clear()
{
    m_fastPatterns.clear();
    m_highWeightGlobs.clear();
    m_lowWeightGlobs.clear();
}

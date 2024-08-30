// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qstringlist.h>
#include <QtCore/qhash.h>

#include <algorithm.h>

namespace Utils {

struct MimeGlobMatchResult
{
    void addMatch(const QString &mimeType, int weight, const QString &pattern,
                  qsizetype knownSuffixLength = 0);

    QStringList m_matchingMimeTypes; // only those with highest weight
    QStringList m_allMatchingMimeTypes;
    int m_weight = 0;
    qsizetype m_matchingPatternLength = 0;
    qsizetype m_knownSuffixLength = 0;
};

class MimeGlobPattern
{
public:
    static const unsigned MaxWeight = 100;
    static const unsigned DefaultWeight = 50;
    static const unsigned MinWeight = 1;

    explicit MimeGlobPattern(const QString &thePattern, const QString &theMimeType, unsigned theWeight = DefaultWeight, Qt::CaseSensitivity s = Qt::CaseInsensitive) :
        m_pattern(s == Qt::CaseInsensitive ? thePattern.toLower() : thePattern),
        m_mimeType(theMimeType),
        m_weight(theWeight),
        m_caseSensitivity(s),
        m_patternType(detectPatternType(m_pattern))
    {
    }

    void swap(MimeGlobPattern &other) noexcept
    {
        qSwap(m_pattern,         other.m_pattern);
        qSwap(m_mimeType,        other.m_mimeType);
        qSwap(m_weight,          other.m_weight);
        qSwap(m_caseSensitivity, other.m_caseSensitivity);
        qSwap(m_patternType,     other.m_patternType);
    }

    bool matchFileName(const QString &inputFileName) const;

    inline const QString &pattern() const { return m_pattern; }
    inline unsigned weight() const { return m_weight; }
    inline const QString &mimeType() const { return m_mimeType; }
    inline bool isCaseSensitive() const { return m_caseSensitivity == Qt::CaseSensitive; }

private:
    enum PatternType {
        SuffixPattern,
        PrefixPattern,
        LiteralPattern,
        VdrPattern,        // special handling for "[0-9][0-9][0-9].vdr" pattern
        AnimPattern,       // special handling for "*.anim[1-9j]" pattern
        OtherPattern
    };
    PatternType detectPatternType(QStringView pattern) const;

    QString m_pattern;
    QString m_mimeType;
    int m_weight;
    Qt::CaseSensitivity m_caseSensitivity;
    PatternType m_patternType;
};

using AddMatchFilterFunc = std::function<bool(const QString &)>;

class MimeGlobPatternList : public QList<MimeGlobPattern>
{
public:
    bool hasPattern(QStringView mimeType, QStringView pattern) const
    {
        auto matchesMimeAndPattern = [mimeType, pattern](const MimeGlobPattern &e) {
            return e.pattern() == pattern && e.mimeType() == mimeType;
        };
        return std::any_of(begin(), end(), matchesMimeAndPattern);
    }

    /*!
        "noglobs" is very rare occurrence, so it's ok if it's slow
     */
    void removeMimeType(QStringView mimeType)
    {
        auto isMimeTypeEqual = [mimeType](const MimeGlobPattern &pattern) {
            return pattern.mimeType() == mimeType;
        };
        erase(std::remove_if(begin(), end(), isMimeTypeEqual), end());
    }

    void match(MimeGlobMatchResult &result, const QString &fileName,
               const AddMatchFilterFunc &filterFunc) const;
};

/*!
    Result of the globs parsing, as data structures ready for efficient MIME type matching.
    This contains:
    1) a map of fast regular patterns (e.g. *.txt is stored as "txt" in a qhash's key)
    2) a linear list of high-weight globs
    3) a linear list of low-weight globs
 */
class MimeAllGlobPatterns
{
public:
    typedef QHash<QString, QStringList> PatternsMap; // MIME type -> patterns

    void addGlob(const MimeGlobPattern &glob);
    void removeMimeType(const QString &mimeType);
    void matchingGlobs(const QString &fileName, MimeGlobMatchResult &result,
                       const AddMatchFilterFunc &filterFunc) const;
    void clear();

    PatternsMap m_fastPatterns; // example: "doc" -> "application/msword", "text/plain"
    MimeGlobPatternList m_highWeightGlobs;
    MimeGlobPatternList m_lowWeightGlobs; // <= 50, including the non-fast 50 patterns
};

} // namespace Utils

QT_BEGIN_NAMESPACE
Q_DECLARE_SHARED(Utils::MimeGlobPattern)
QT_END_NAMESPACE

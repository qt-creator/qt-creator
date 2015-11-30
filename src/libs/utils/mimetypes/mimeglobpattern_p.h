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

#ifndef MIMEGLOBPATTERN_P_H
#define MIMEGLOBPATTERN_P_H

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

namespace Utils {
namespace Internal {

struct MimeGlobMatchResult
{
    MimeGlobMatchResult()
    : m_weight(0), m_matchingPatternLength(0)
    {}

    void addMatch(const QString &mimeType, int weight, const QString &pattern);

    QStringList m_matchingMimeTypes;
    int m_weight;
    int m_matchingPatternLength;
    QString m_foundSuffix;
};

class MimeGlobPattern
{
public:
    static const unsigned MaxWeight = 100;
    static const unsigned DefaultWeight = 50;
    static const unsigned MinWeight = 1;

    explicit MimeGlobPattern(const QString &thePattern, const QString &theMimeType, unsigned theWeight = DefaultWeight, Qt::CaseSensitivity s = Qt::CaseInsensitive) :
        m_pattern(thePattern), m_mimeType(theMimeType), m_weight(theWeight), m_caseSensitivity(s)
    {
        if (s == Qt::CaseInsensitive) {
            m_pattern = m_pattern.toLower();
        }
    }
    ~MimeGlobPattern() {}

    bool matchFileName(const QString &filename) const;

    inline const QString &pattern() const { return m_pattern; }
    inline unsigned weight() const { return m_weight; }
    inline const QString &mimeType() const { return m_mimeType; }
    inline bool isCaseSensitive() const { return m_caseSensitivity == Qt::CaseSensitive; }

private:
    QString m_pattern;
    QString m_mimeType;
    int m_weight;
    Qt::CaseSensitivity m_caseSensitivity;
};

class MimeGlobPatternList : public QList<MimeGlobPattern>
{
public:
    bool hasPattern(const QString &mimeType, const QString &pattern) const
    {
        const_iterator it = begin();
        const const_iterator myend = end();
        for (; it != myend; ++it)
            if ((*it).pattern() == pattern && (*it).mimeType() == mimeType)
                return true;
        return false;
    }

    /*!
        "noglobs" is very rare occurrence, so it's ok if it's slow
     */
    void removeMimeType(const QString &mimeType)
    {
        QMutableListIterator<MimeGlobPattern> it(*this);
        while (it.hasNext()) {
            if (it.next().mimeType() == mimeType)
                it.remove();
        }
    }

    void match(MimeGlobMatchResult &result, const QString &fileName) const;
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
    QStringList matchingGlobs(const QString &fileName, QString *foundSuffix) const;
    void clear();

    PatternsMap m_fastPatterns; // example: "doc" -> "application/msword", "text/plain"
    MimeGlobPatternList m_highWeightGlobs;
    MimeGlobPatternList m_lowWeightGlobs; // <= 50, including the non-fast 50 patterns
};

} // Internal
} // Utils

#endif // MIMEGLOBPATTERN_P_H

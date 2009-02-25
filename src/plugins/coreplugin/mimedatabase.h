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

#ifndef MIMEDATABASE_H
#define MIMEDATABASE_H

#include <coreplugin/core_global.h>
#include <QtCore/QStringList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QByteArray>

QT_BEGIN_NAMESPACE
class QIODevice;
class QRegExp;
class QDebug;
class QFileInfo;
QT_END_NAMESPACE

namespace Core {

class MimeTypeData;
class MimeDatabasePrivate;

namespace Internal {
    class BaseMimeTypeParser;
    class FileMatchContext;
}

/* Magic (file contents) matcher interface. */
class CORE_EXPORT IMagicMatcher
{
    Q_DISABLE_COPY(IMagicMatcher)
protected:
    IMagicMatcher() {}
public:
    // Check for a match on contents of a file
    virtual bool matches(const QByteArray &data) const = 0;
    // Return a priority value from 1..100
    virtual int priority() const = 0;
    virtual ~IMagicMatcher() {}
};

/* Utility class: A standard Magic match rule based on contents. Provides
 * static factory methods for creation (currently only for "string". This can
 * be extended to handle "little16"/"big16", etc.). */
class CORE_EXPORT MagicRule
{
    Q_DISABLE_COPY(MagicRule)
public:
    explicit MagicRule(const QByteArray &pattern, int startPos, int endPos);
    bool matches(const QByteArray &data) const;

    // Convenience factory methods
    static MagicRule *createStringRule(const QString &c, int startPos, int endPos);

private:
    const QByteArray m_pattern;
    const int m_startPos;
    const int m_endPos;
};

/* Utility class: A Magic matcher that checks a number of rules based on
 * operator "or". It is used for rules parsed from XML files. */
class CORE_EXPORT MagicRuleMatcher : public IMagicMatcher
{
    Q_DISABLE_COPY(MagicRuleMatcher)
public:
    typedef  QSharedPointer<MagicRule> MagicRuleSharedPointer;

    MagicRuleMatcher();
    void add(const MagicRuleSharedPointer &rule);
    virtual bool matches(const QByteArray &data) const;

    virtual int priority() const;
    void setPriority(int p);

private:
    typedef QList<MagicRuleSharedPointer> MagicRuleList;
    MagicRuleList m_list;
    int m_priority;
};

/* Mime type data used in Qt Creator. Contains most information from
 * standard mime type XML database files.
 * Omissions:
 * - Only magic of type "string" is supported. In addition, C++ classes
 *   derived from IMagicMatcher can be added to check on contents
 * - acronyms, language-specific comments
 * Extensions:
 * - List of suffixes and preferred suffix (derived from glob patterns).
 */
class CORE_EXPORT MimeType
{
public:
    /* Return value of a glob match, which is higher than magic */
    enum { GlobMatchPriority = 101 };

    MimeType();
    MimeType(const MimeType&);
    MimeType &operator=(const MimeType&);
    ~MimeType();

    void clear();
    bool isNull() const;
    operator bool() const;

    bool isTopLevel() const;

    QString type() const;
    void setType(const QString &type);

    QStringList aliases() const;
    void setAliases(const QStringList &);

    QString comment() const;
    void setComment(const QString &comment);

    QString localeComment(const QString &locale = QString() /* en, de...*/) const;
    void setLocaleComment(const QString &locale, const QString &comment);

    QList<QRegExp> globPatterns() const;
    void setGlobPatterns(const QList<QRegExp> &);

    QStringList subClassesOf() const;
    void setSubClassesOf(const QStringList &);

    // Extension over standard mime data
    QStringList suffixes() const;
    void setSuffixes(const QStringList &);

    // Extension over standard mime data
    QString preferredSuffix() const;
    bool setPreferredSuffix(const QString&);

    // Check for type or one of the aliases
    bool matchesType(const QString &type) const;
    // Check glob patterns and magic. Returns the match priority (0 no match,
    // 1..100 indicating a magic match or GlobMatchPriority indicating an
    // exact glob match).
    unsigned matchesFile(const QFileInfo &file) const;

    // Return a filter string usable for a file dialog
    QString filterString() const;

    // Add magic matcher
    void addMagicMatcher(const QSharedPointer<IMagicMatcher> &matcher);

    friend QDebug operator<<(QDebug d, const MimeType &mt);

private:
    explicit MimeType(const MimeTypeData &d);
    unsigned matchesFile(Internal::FileMatchContext &c) const;

    friend class Internal::BaseMimeTypeParser;
    friend class MimeDatabasePrivate;
    QSharedDataPointer<MimeTypeData> m_d;
};

/* A Mime data base to which the plugins can add the mime types they handle.
 * When adding a "text/plain" to it, the mimetype will receive a magic matcher
 * that checks for text files that do not match the globs by heuristics.
 *
 * A good testcase is to run it over '/usr/share/mime/<*>/<*>.xml' on Linux. */

class CORE_EXPORT MimeDatabase
{
    Q_DISABLE_COPY(MimeDatabase)
public:
    MimeDatabase();

    ~MimeDatabase();

    bool addMimeTypes(const QString &fileName, QString *errorMessage);
    bool addMimeTypes(QIODevice *device, QString *errorMessage);
    bool addMimeType(const  MimeType &mt);

    // Returns a mime type or Null one if none found
    MimeType findByType(const QString &type) const;
    // Returns a mime type or Null one if none found
    MimeType findByFile(const QFileInfo &f) const;

    // Convenience
    QString preferredSuffixByType(const QString &type) const;
    QString preferredSuffixByFile(const QFileInfo &f) const;

    // Return all known suffixes
    QStringList suffixes() const;
    bool setPreferredSuffix(const QString &typeOrAlias, const QString &suffix);

    QStringList filterStrings() const;

    friend QDebug operator<<(QDebug d, const MimeDatabase &mt);

private:
    MimeDatabasePrivate *m_d;
};

} // namespace Core

#endif // MIMEDATABASE_H

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

#include "mimedatabase.h"
#include "coreconstants.h"
#include "icore.h"

#include <utils/qtcassert.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QHash>
#include <QMultiHash>
#include <QRegExp>
#include <QSharedData>
#include <QSharedPointer>
#include <QStringList>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>
#include <functional>

enum { debugMimeDB = 0 };

// XML tags in mime files
static const char mimeInfoTagC[] = "mime-info";
static const char mimeTypeTagC[] = "mime-type";
static const char mimeTypeAttributeC[] = "type";
static const char subClassTagC[] = "sub-class-of";
static const char commentTagC[] = "comment";
static const char globTagC[] = "glob";
static const char aliasTagC[] = "alias";
static const char patternAttributeC[] = "pattern";
static const char weightAttributeC[] = "weight";
static const char localeAttributeC[] = "xml:lang";

static const char magicTagC[] = "magic";
static const char priorityAttributeC[] = "priority";
static const char matchTagC[] = "match";
static const char matchValueAttributeC[] = "value";
static const char matchTypeAttributeC[] = "type";
static const char matchStringTypeValueC[] = "string";
static const char matchByteTypeValueC[] = "byte";
static const char matchOffsetAttributeC[] = "offset";

// Types
static const char textTypeC[] = "text/plain";
static const char binaryTypeC[] = "application/octet-stream";

// UTF16 byte order marks
static const char bigEndianByteOrderMarkC[] = "\xFE\xFF";
static const char littleEndianByteOrderMarkC[] = "\xFF\xFE";

// Fallback priorities, must be low.
enum {
    BinaryMatchPriority = Core::MimeGlobPattern::MinWeight + 1,
    TextMatchPriority
};

/*!
    \class Core::IMagicMatcher

    \brief Interface for a Mime type magic matcher (examinig file contents).

    \sa Core::MimeType, Core::MimeDatabase, Core::MagicRuleMatcher, Core::MagicRule, Core::MagicStringRule, Core::MagicByteRule, Core::GlobPattern
    \sa Core::Internal::FileMatchContext, Core::Internal::BinaryMatcher, Core::Internal::HeuristicTextMagicMatcher
    \sa Core::Internal::BaseMimeTypeParser, Core::Internal::MimeTypeParser
*/

namespace Core {

typedef QSharedPointer<MagicRuleMatcher> MagicRuleMatcherPtr;

namespace Internal {

/*!
    \class Core::Internal::FileMatchContext

    \brief Context passed on to the mime types when looking for a file match.

    It exists to enable reading the file contents "on demand"
    (as opposed to each mime type trying to open and read while checking).

    \sa Core::MimeType, Core::MimeDatabase, Core::IMagicMatcher, Core::MagicRuleMatcher, Core::MagicRule, Core::MagicStringRule, Core::MagicByteRule, Core::GlobPattern
    \sa Core::Internal::BinaryMatcher, Core::Internal::HeuristicTextMagicMatcher
    \sa Core::Internal::BaseMimeTypeParser, Core::Internal::MimeTypeParser
*/

class FileMatchContext
{
    Q_DISABLE_COPY(FileMatchContext)

public:
    // Max data to be read from a file
    enum { MaxData = 2048 };

    explicit FileMatchContext(const QFileInfo &fi);

    inline QString fileName() const { return m_fileName; }
    // Return (cached) first MaxData bytes of file
    QByteArray data();

private:
    enum State {
        // File cannot be read/does not exist
        NoDataAvailable,
        // Not read yet
        DataNotRead,
        // Available
        DataRead };
    const QFileInfo m_fileInfo;
    const QString m_fileName;
    State m_state;
    QByteArray m_data;
};

FileMatchContext::FileMatchContext(const QFileInfo &fi) :
    m_fileInfo(fi),
    m_fileName(fi.fileName()),
    m_state(fi.isFile() && fi.isReadable() && fi.size() > 0 ? DataNotRead : NoDataAvailable)
{
}

QByteArray FileMatchContext::data()
{
    // Do we need to read?
    if (m_state == DataNotRead) {
        const QString fullName = m_fileInfo.absoluteFilePath();
        QFile file(fullName);
        if (file.open(QIODevice::ReadOnly)) {
            m_data = file.read(MaxData);
            m_state = DataRead;
        } else {
            qWarning("%s failed to open %s: %s\n", Q_FUNC_INFO, fullName.toUtf8().constData(), file.errorString().toUtf8().constData());
            m_state = NoDataAvailable;
        }
    }
    return m_data;
}

/*!
    \class Core::Internal::BinaryMatcher
    \brief The binary fallback matcher for mime type "application/octet-stream".

    \sa Core::MimeType, Core::MimeDatabase, Core::IMagicMatcher, Core::MagicRuleMatcher, Core::MagicRule, Core::MagicStringRule, Core::MagicByteRule, Core::GlobPattern
    \sa Core::Internal::FileMatchContext, Core::Internal::HeuristicTextMagicMatcher
    \sa Core::Internal::BaseMimeTypeParser, Core::Internal::MimeTypeParser
*/

class BinaryMatcher : public IMagicMatcher
{
public:
    BinaryMatcher() {}
    virtual bool matches(const QByteArray & /*data*/) const { return true; }
    virtual int priority() const  { return BinaryMatchPriority; }
};

/*!
    \class Core::Internal::HeuristicTextMagicMatcher
    \brief Heuristic text file matcher for mime types.

    If the data do not contain any character below tab (9), detect as text.
    Additionally, check on UTF16 byte order markers.

    \sa Core::MimeType, Core::MimeDatabase, Core::IMagicMatcher, Core::MagicRuleMatcher, Core::MagicRule, Core::MagicStringRule, Core::MagicByteRule, Core::GlobPattern
    \sa Core::Internal::FileMatchContext, Core::Internal::BinaryMatcher
    \sa Core::Internal::BaseMimeTypeParser, Core::Internal::MimeTypeParser
*/

class HeuristicTextMagicMatcher : public IMagicMatcher
{
public:
    HeuristicTextMagicMatcher() {}
    virtual bool matches(const QByteArray &data) const;
    virtual int priority() const  { return TextMatchPriority; }

    static bool isTextFile(const QByteArray &data);
};

bool HeuristicTextMagicMatcher::isTextFile(const QByteArray &data)
{
    const int size = data.size();
    for (int i = 0; i < size; i++) {
        const char c = data.at(i);
        if (c >= 0x01 && c < 0x09) // Sure-fire binary
            return false;
        if (c == 0)             // Check for UTF16
            return data.startsWith(bigEndianByteOrderMarkC) || data.startsWith(littleEndianByteOrderMarkC);
    }
    return true;
}

bool HeuristicTextMagicMatcher::matches(const QByteArray &data) const
{
    const bool rc = isTextFile(data);
    if (debugMimeDB)
        qDebug() << Q_FUNC_INFO << " on " << data.size() << " returns " << rc;
    return rc;
}

} // namespace Internal

/*!
    \class Core::MagicRule
    \brief Base class for standard Magic match rules based on contents
    and offset specification.

    Stores the offset and provides conversion helpers.
    Base class for implementations for "string" and "byte".
    (Others like little16, big16, etc. can be created whenever there is a need.)

    \sa Core::MimeType, Core::MimeDatabase, Core::IMagicMatcher, Core::MagicRuleMatcher, Core::MagicStringRule, Core::MagicByteRule, Core::GlobPattern
    \sa Core::Internal::FileMatchContext, Core::Internal::BinaryMatcher, Core::Internal::HeuristicTextMagicMatcher
    \sa Core::Internal::BaseMimeTypeParser, Core::Internal::MimeTypeParser
 */

const QChar MagicRule::kColon(QLatin1Char(':'));

MagicRule::MagicRule(int startPos, int endPos) : m_startPos(startPos), m_endPos(endPos)
{
}

MagicRule::~MagicRule()
{
}

int MagicRule::startPos() const
{
    return m_startPos;
}

int MagicRule::endPos() const
{
    return m_endPos;
}

QString MagicRule::toOffset(const QPair<int, int> &startEnd)
{
    return QString(QLatin1String("%1:%2")).arg(startEnd.first).arg(startEnd.second);
}

QPair<int, int> MagicRule::fromOffset(const QString &offset)
{
    const QStringList &startEnd = offset.split(kColon);
    Q_ASSERT(startEnd.size() == 2);
    return qMakePair(startEnd.at(0).toInt(), startEnd.at(1).toInt());
}

/*!
    \class Core::MagicStringRule
    \brief Match on a string.

    \sa Core::MimeType, Core::MimeDatabase, Core::IMagicMatcher, Core::MagicRuleMatcher, Core::MagicRule, Core::MagicByteRule, Core::GlobPattern
    \sa Core::Internal::FileMatchContext, Core::Internal::BinaryMatcher, Core::Internal::HeuristicTextMagicMatcher
    \sa Core::Internal::BaseMimeTypeParser, Core::Internal::MimeTypeParser
 */

const QString MagicStringRule::kMatchType(QLatin1String("string"));

MagicStringRule::MagicStringRule(const QString &s, int startPos, int endPos) :
    MagicRule(startPos, endPos), m_pattern(s.toUtf8())
{
}

MagicStringRule::~MagicStringRule()
{
}

QString MagicStringRule::matchType() const
{
    return kMatchType;
}

QString MagicStringRule::matchValue() const
{
    return QLatin1String(m_pattern);
}

bool MagicStringRule::matches(const QByteArray &data) const
{
    // Quick check
    if ((startPos() + m_pattern.size()) > data.size())
        return false;
    // Most common: some string at position 0:
    if (startPos() == 0 && startPos() == endPos())
        return data.startsWith(m_pattern);
    // Range
    const int index = data.indexOf(m_pattern, startPos());
    const bool rc = index != -1 && index <= endPos();
    if (debugMimeDB)
        qDebug() << "Checking " << m_pattern << startPos() << endPos() << " returns " << rc;
    return rc;
}

/*!
    \class Core::MagicByteRule
    \brief Match on a sequence of binary data.

    Format:
    \code
    \0x7f\0x45\0x4c\0x46
    \endcode

    \sa Core::MimeType, Core::MimeDatabase, Core::IMagicMatcher, Core::MagicRuleMatcher, Core::MagicRule, Core::MagicStringRule, Core::MagicByteRule, Core::GlobPattern
    \sa Core::Internal::FileMatchContext, Core::Internal::BinaryMatcher, Core::Internal::HeuristicTextMagicMatcher
    \sa Core::Internal::BaseMimeTypeParser, Core::Internal::MimeTypeParser
 */

const QString MagicByteRule::kMatchType(QLatin1String("byte"));

MagicByteRule::MagicByteRule(const QString &s, int startPos, int endPos) :
    MagicRule(startPos, endPos), m_bytesSize(0)
{
    if (validateByteSequence(s, &m_bytes))
        m_bytesSize = m_bytes.size();
    else
        m_bytes.clear();
}

MagicByteRule::~MagicByteRule()
{
}

bool MagicByteRule::validateByteSequence(const QString &sequence, QList<int> *bytes)
{
    // Expect an hex format value like this: \0x7f\0x45\0x4c\0x46
    const QStringList &byteSequence = sequence.split(QLatin1Char('\\'), QString::SkipEmptyParts);
    foreach (const QString &byte, byteSequence) {
        bool ok;
        const int hex = byte.toInt(&ok, 16);
        if (ok) {
            if (bytes)
                bytes->push_back(hex);
        } else {
            return false;
        }
    }
    return true;
}

QString MagicByteRule::matchType() const
{
    return kMatchType;
}

QString MagicByteRule::matchValue() const
{
    QString value;
    foreach (int byte, m_bytes)
        value.append(QString(QLatin1String("\\0x%1")).arg(byte, 0, 16));
    return value;
}

bool MagicByteRule::matches(const QByteArray &data) const
{
    if (m_bytesSize == 0)
        return false;

    const int dataSize = data.size();
    for (int start = startPos(); start <= endPos(); ++start) {
        if ((start + m_bytesSize) > dataSize)
            return false;

        int matchAt = 0;
        while (matchAt < m_bytesSize) {
            if (data.at(start + matchAt) != m_bytes.at(matchAt))
                break;
            ++matchAt;
        }
        if (matchAt == m_bytesSize)
            return true;
    }

    return false;
}

/*!
    \class Core::MagicRuleMatcher

    \brief A Magic matcher that checks a number of rules based on operator "or".

    It is used for rules parsed from XML files.

    \sa Core::MimeType, Core::MimeDatabase, Core::IMagicMatcher, Core::MagicRule, Core::MagicStringRule, Core::MagicByteRule, Core::GlobPattern
    \sa Core::Internal::FileMatchContext, Core::Internal::BinaryMatcher, Core::Internal::HeuristicTextMagicMatcher
    \sa Core::Internal::BaseMimeTypeParser, Core::Internal::MimeTypeParser
*/

MagicRuleMatcher::MagicRuleMatcher() :
     m_priority(65535)
{
}

void MagicRuleMatcher::add(const MagicRuleSharedPointer &rule)
{
    m_list.append(rule);
}

void MagicRuleMatcher::add(const MagicRuleList &ruleList)
{
    m_list.append(ruleList);
}

MagicRuleMatcher::MagicRuleList MagicRuleMatcher::magicRules() const
{
    return m_list;
}

bool MagicRuleMatcher::matches(const QByteArray &data) const
{
    const MagicRuleList::const_iterator cend = m_list.constEnd();
    for (MagicRuleList::const_iterator it = m_list.constBegin(); it != cend; ++it)
        if ( (*it)->matches(data))
            return true;
    return false;
}

int MagicRuleMatcher::priority() const
{
    return m_priority;
}

void MagicRuleMatcher::setPriority(int p)
{
    m_priority = p;
}

IMagicMatcher::IMagicMatcherList MagicRuleMatcher::createMatchers(
    const QHash<int, MagicRuleList> &rulesByPriority)
{
    IMagicMatcher::IMagicMatcherList matchers;
    QHash<int, MagicRuleList>::const_iterator ruleIt = rulesByPriority.begin();
    for (; ruleIt != rulesByPriority.end(); ++ruleIt) {
        MagicRuleMatcher *magicRuleMatcher = new MagicRuleMatcher();
        magicRuleMatcher->setPriority(ruleIt.key());
        magicRuleMatcher->add(ruleIt.value());
        matchers.append(IMagicMatcher::IMagicMatcherSharedPointer(magicRuleMatcher));
    }
    return matchers;
}

/*!
    \class Core::GlobPattern
    \brief Glob pattern for file names for mime type matching.

    \sa Core::MimeType, Core::MimeDatabase, Core::IMagicMatcher, Core::MagicRuleMatcher, Core::MagicRule, Core::MagicStringRule, Core::MagicByteRule
    \sa Core::Internal::FileMatchContext, Core::Internal::BinaryMatcher, Core::Internal::HeuristicTextMagicMatcher
    \sa Core::Internal::BaseMimeTypeParser, Core::Internal::MimeTypeParser
*/

MimeGlobPattern::MimeGlobPattern(const QString &pattern, unsigned weight) :
    m_pattern(pattern), m_weight(weight)
{
    bool hasQuestionMark = pattern.contains(QLatin1Char('?'));
    bool hasStar = pattern.contains(QLatin1Char('*'));

    if (!hasQuestionMark && hasStar && pattern.lastIndexOf(QLatin1Char('*')) == 0) {
        m_type = Suffix;
    } else if (!hasQuestionMark && !hasStar) {
        m_type = Exact;
    } else {
        // This hopefully never happens as it is expensive.
        m_type = Glob;
        m_regexp.setPattern(pattern);
        m_regexp.setPatternSyntax(QRegExp::Wildcard);
        if (!m_regexp.isValid())
            qWarning("%s: Invalid wildcard '%s'.", Q_FUNC_INFO, pattern.toUtf8().constData());
    }
}

MimeGlobPattern::~MimeGlobPattern()
{
}

bool MimeGlobPattern::matches(const QString &fileName) const
{
    if (m_type == Exact)
        return fileName == m_pattern;
    if (m_type == Suffix)
        return fileName.endsWith(m_pattern.midRef(1));
    return m_regexp.exactMatch(fileName);
}


/*!
    \class Core::MimeType

    \brief Mime type data used in Qt Creator.

    Contains most information from standard mime type XML database files.

    Currently, magic of types "string", "bytes" is supported. In addition,
    C++ classes, derived from Core::IMagicMatcher can be added to check
    on contents.

    In addition, the class provides a list of suffixes and a concept of the
    'preferred suffix' (derived from glob patterns). This is used for example
    to be able to configure the suffix used for C++-files in Qt Creator.

    Mime XML looks like:
    \code
    <?xml version="1.0" encoding="UTF-8"?>
    <mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
    <!-- Mime types must match the desktop file associations -->
      <mime-type type="application/vnd.nokia.qt.qmakeprofile">
        <comment xml:lang="en">Qt qmake Profile</comment>
        <glob pattern="*.pro" weight="50"/>
      </mime-type>
    </mime-info>
    \endcode

    \sa Core::MimeDatabase, Core::IMagicMatcher, Core::MagicRuleMatcher, Core::MagicRule, Core::MagicStringRule, Core::MagicByteRule, Core::GlobPattern
    \sa Core::Internal::FileMatchContext, Core::Internal::BinaryMatcher, Core::Internal::HeuristicTextMagicMatcher
    \sa Core::Internal::BaseMimeTypeParser, Core::Internal::MimeTypeParser
*/

class MimeTypeData : public QSharedData {
public:
    typedef QHash<QString,QString> LocaleHash;

    MimeTypeData();

    void clear();
    void assignSuffix(const QString &pattern);
    void assignSuffixes(const QStringList &patterns);
    void debug(QTextStream &str, int indent = 0) const;

    QRegExp suffixPattern;

    QString type;
    QString comment;

    LocaleHash localeComments;
    QStringList aliases;
    QList<MimeGlobPattern> globPatterns;
    QStringList subClassesOf;
    QString preferredSuffix;
    QStringList suffixes;
    IMagicMatcher::IMagicMatcherList magicMatchers;
};

MimeTypeData::MimeTypeData()
    // RE to match a suffix glob pattern: "*.ext" (and not sth like "Makefile" or
    // "*.log[1-9]"
    : suffixPattern(QLatin1String("^\\*\\.[\\w+]+$"))
{
    QTC_CHECK(suffixPattern.isValid());
}

void MimeTypeData::clear()
{
    type.clear();
    comment.clear();
    aliases.clear();
    globPatterns.clear();
    subClassesOf.clear();
    preferredSuffix.clear();
    suffixes.clear();
    magicMatchers.clear();
}

void MimeTypeData::assignSuffix(const QString &pattern)
{
    if (suffixPattern.exactMatch(pattern)) {
        const QString suffix = pattern.right(pattern.size() - 2);
        suffixes.push_back(suffix);
        if (preferredSuffix.isEmpty())
            preferredSuffix = suffix;
    }
}

void MimeTypeData::assignSuffixes(const QStringList &patterns)
{
    foreach (const QString &pattern, patterns)
        assignSuffix(pattern);
}

void MimeTypeData::debug(QTextStream &str, int indent) const
{
    const QString indentS = QString(indent, QLatin1Char(' '));
    const QString comma = QString(1, QLatin1Char(','));
    str << indentS << "Type: " << type;
    if (!aliases.empty())
        str << " Aliases: " << aliases.join(comma);
    str << ", magic: " << magicMatchers.size() << '\n';
    str << indentS << "Comment: " << comment << '\n';
    if (!subClassesOf.empty())
        str << indentS << "SubClassesOf: " << subClassesOf.join(comma) << '\n';
    if (!globPatterns.empty()) {
        str << indentS << "Glob: ";
        foreach (const MimeGlobPattern &gp, globPatterns)
            str << gp.pattern() << '(' << gp.weight() << ')';
        str << '\n';
        if (!suffixes.empty()) {
            str <<  indentS << "Suffixes: " << suffixes.join(comma)
                << " preferred: " << preferredSuffix << '\n';
        }
    }
    str << '\n';
}

// ---------------- MimeType
MimeType::MimeType() :
    m_d(new MimeTypeData)
{
}

MimeType::MimeType(const MimeType &rhs) :
    m_d(rhs.m_d)
{
}

MimeType &MimeType::operator=(const MimeType &rhs)
{
    if (this != &rhs)
        m_d = rhs.m_d;
    return *this;
}

MimeType::MimeType(const MimeTypeData &d) :
    m_d(new MimeTypeData(d))
{
}

MimeType::~MimeType()
{
}

void MimeType::clear()
{
    m_d->clear();
}

bool MimeType::isNull() const
{
    return m_d->type.isEmpty();
}

MimeType::operator bool() const
{
    return !isNull();
}

bool MimeType::isTopLevel() const
{
    return m_d->subClassesOf.empty();
}

QString MimeType::type() const
{
    return m_d->type;
}

void MimeType::setType(const QString &type)
{
    m_d->type = type;
}

QString MimeType::comment() const
{
    return m_d->comment;
}

void MimeType::setComment(const QString &comment)
{
    m_d->comment = comment;
}

// Return "en", "de", etc. derived from "en_US", de_DE".
static inline QString systemLanguage()
{
    QString name = QLocale::system().name();
    const int underScorePos = name.indexOf(QLatin1Char('_'));
    if (underScorePos != -1)
        name.truncate(underScorePos);
    return name;
}

QString MimeType::localeComment(const QString &localeArg) const
{
    const QString locale = localeArg.isEmpty() ? systemLanguage() : localeArg;
    const MimeTypeData::LocaleHash::const_iterator it = m_d->localeComments.constFind(locale);
    if (it == m_d->localeComments.constEnd())
        return m_d->comment;
    return it.value();
}

void MimeType::setLocaleComment(const QString &locale, const QString &comment)
{
     m_d->localeComments[locale] = comment;
}

QStringList MimeType::aliases() const
{
    return m_d->aliases;
}

void MimeType::setAliases(const QStringList &a)
{
     m_d->aliases = a;
}

QList<MimeGlobPattern> MimeType::globPatterns() const
{
    return m_d->globPatterns;
}

void MimeType::setGlobPatterns(const QList<MimeGlobPattern> &g)
{
    m_d->globPatterns = g;

    QString oldPrefferedSuffix = m_d->preferredSuffix;
    m_d->suffixes.clear();
    m_d->preferredSuffix.clear();
    m_d->assignSuffixes(MimeDatabase::fromGlobPatterns(g));
    if (m_d->preferredSuffix != oldPrefferedSuffix && m_d->suffixes.contains(oldPrefferedSuffix))
        m_d->preferredSuffix = oldPrefferedSuffix;
}

QStringList MimeType::subClassesOf() const
{
    return m_d->subClassesOf;
}

void MimeType::setSubClassesOf(const QStringList &s)
{
    m_d->subClassesOf = s;
}

QString MimeType::preferredSuffix() const
{
    return m_d->preferredSuffix;
}

bool MimeType::setPreferredSuffix(const QString &s)
{
    if (!m_d->suffixes.contains(s)) {
        qWarning("%s: Attempt to set preferred suffix to '%s', which is not in the list of suffixes: %s.",
                 m_d->type.toUtf8().constData(),
                 s.toUtf8().constData(),
                 m_d->suffixes.join(QLatin1String(",")).toUtf8().constData());
        return false;
    }
    m_d->preferredSuffix = s;
    return true;
}

QString MimeType::formatFilterString(const QString &description, const QList<MimeGlobPattern> &globs)
{
    QString rc;
    if (globs.empty())  // Binary files
        return rc;
    {
        QTextStream str(&rc);
        str << description;
        if (!globs.empty()) {
            str << " (";
            const int size = globs.size();
            for (int i = 0; i < size; i++) {
                if (i)
                    str << ' ';
                str << globs.at(i).pattern();
            }
            str << ')';
        }
    }
    return rc;
}

QString MimeType::filterString() const
{
    // @todo: Use localeComment() once creator is shipped with translations
    return formatFilterString(comment(), m_d->globPatterns);
}

bool MimeType::matchesType(const QString &type) const
{
    return m_d->type == type || m_d->aliases.contains(type);
}

unsigned MimeType::matchesFile(const QFileInfo &file) const
{
    Internal::FileMatchContext context(file);
    const unsigned suffixPriority = matchesFileBySuffix(context);
    if (suffixPriority >= MimeGlobPattern::MaxWeight)
        return suffixPriority;
    return qMax(suffixPriority, matchesFileByContent(context));
}

unsigned MimeType::matchesFileBySuffix(Internal::FileMatchContext &c) const
{
    // check globs
    foreach (const MimeGlobPattern &gp, m_d->globPatterns) {
        if (gp.matches(c.fileName()))
            return gp.weight();
    }
    return 0;
}

unsigned MimeType::matchesFileByContent(Internal::FileMatchContext &c) const
{
    // Nope, try magic matchers on context data
    if (m_d->magicMatchers.isEmpty())
        return 0;

    return matchesData(c.data());
}

unsigned MimeType::matchesData(const QByteArray &data) const
{
    unsigned priority = 0;
    if (!data.isEmpty()) {
        foreach (const IMagicMatcher::IMagicMatcherSharedPointer &matcher, m_d->magicMatchers) {
            const unsigned magicPriority = matcher->priority();
            if (magicPriority > priority && matcher->matches(data))
                priority = magicPriority;
        }
    }
    return priority;
}

QStringList MimeType::suffixes() const
{
    return m_d->suffixes;
}

void MimeType::addMagicMatcher(const IMagicMatcherSharedPointer &matcher)
{
    m_d->magicMatchers.push_back(matcher);
}

const MimeType::IMagicMatcherList &MimeType::magicMatchers() const
{
    return m_d->magicMatchers;
}

void MimeType::setMagicMatchers(const IMagicMatcherList &matchers)
{
    m_d->magicMatchers = matchers;
}

namespace {
struct RemovePred : std::unary_function<MimeType::IMagicMatcherSharedPointer, bool>
{
    RemovePred(bool keepRuleBased) : m_keepRuleBase(keepRuleBased) {}
    bool m_keepRuleBase;

    bool operator()(const MimeType::IMagicMatcherSharedPointer &matcher) {
        if ((m_keepRuleBase && !dynamic_cast<MagicRuleMatcher *>(matcher.data()))
                || (!m_keepRuleBase && dynamic_cast<MagicRuleMatcher *>(matcher.data())))
            return true;
        return false;
    }
};
} // Anonymous

MimeType::IMagicMatcherList MimeType::magicRuleMatchers() const
{
    IMagicMatcherList ruleMatchers = m_d->magicMatchers;
    ruleMatchers.erase(std::remove_if(ruleMatchers.begin(), ruleMatchers.end(), RemovePred(true)),
                       ruleMatchers.end());
    return ruleMatchers;
}

void MimeType::setMagicRuleMatchers(const IMagicMatcherList &matchers)
{
    m_d->magicMatchers.erase(std::remove_if(m_d->magicMatchers.begin(), m_d->magicMatchers.end(),
                                            RemovePred(false)),
                             m_d->magicMatchers.end());
    m_d->magicMatchers.append(matchers);
}

QDebug operator<<(QDebug d, const MimeType &mt)
{
    QString s;
    {
        QTextStream str(&s);
        mt.m_d->debug(str);
    }
    d << s;
    return d;
}

namespace Internal {

/*!
    \class Core::Internal::BaseMimeTypeParser
    \brief Generic parser for a sequence of <mime-type>.

    Calls abstract handler function process for MimeType it finds.

    \sa Core::MimeDatabase, Core::IMagicMatcher, Core::MagicRuleMatcher, Core::MagicRule, Core::MagicStringRule, Core::MagicByteRule, Core::GlobPattern
    \sa Core::Internal::FileMatchContext, Core::Internal::BinaryMatcher, Core::Internal::HeuristicTextMagicMatcher
    \sa Core::Internal::MimeTypeParser
*/

class BaseMimeTypeParser
{
    Q_DISABLE_COPY(BaseMimeTypeParser)

public:
    BaseMimeTypeParser() {}
    virtual ~BaseMimeTypeParser() {}

    bool parse(QIODevice *dev, const QString &fileName, QString *errorMessage);

private:
    // Overwrite to process the sequence of parsed data
    virtual bool process(const MimeType &t, QString *errorMessage) = 0;

    void addGlobPattern(const QString &pattern, const QString &weight, MimeTypeData *d) const;

    enum ParseStage { ParseBeginning,
                  ParseMimeInfo,
                  ParseMimeType,
                  ParseComment,
                  ParseGlobPattern,
                  ParseSubClass,
                  ParseAlias,
                  ParseMagic,
                  ParseMagicMatchRule,
                  ParseOtherMimeTypeSubTag,
                  ParseError };

    static ParseStage nextStage(ParseStage currentStage, const QStringRef &startElement);
};

void BaseMimeTypeParser::addGlobPattern(const QString &pattern, const QString &weight, MimeTypeData *d) const
{
    if (pattern.isEmpty())
        return;
    // Collect patterns as a QRegExp list and filter out the plain
    // suffix ones for our suffix list. Use first one as preferred
    if (weight.isEmpty())
        d->globPatterns.push_back(MimeGlobPattern(pattern));
    else
        d->globPatterns.push_back(MimeGlobPattern(pattern, weight.toInt()));

    d->assignSuffix(pattern);
}

BaseMimeTypeParser::ParseStage BaseMimeTypeParser::nextStage(ParseStage currentStage, const QStringRef &startElement)
{
    switch (currentStage) {
    case ParseBeginning:
        if (startElement == QLatin1String(mimeInfoTagC))
            return ParseMimeInfo;
        if (startElement == QLatin1String(mimeTypeTagC))
            return ParseMimeType;
        return ParseError;
    case ParseMimeInfo:
        return startElement == QLatin1String(mimeTypeTagC) ? ParseMimeType : ParseError;
    case ParseMimeType:
    case ParseComment:
    case ParseGlobPattern:
    case ParseSubClass:
    case ParseAlias:
    case ParseOtherMimeTypeSubTag:
    case ParseMagicMatchRule:
        if (startElement == QLatin1String(mimeTypeTagC)) // Sequence of <mime-type>
            return ParseMimeType;
        if (startElement == QLatin1String(commentTagC ))
            return ParseComment;
        if (startElement == QLatin1String(globTagC))
            return ParseGlobPattern;
        if (startElement == QLatin1String(subClassTagC))
            return ParseSubClass;
        if (startElement == QLatin1String(aliasTagC))
            return ParseAlias;
        if (startElement == QLatin1String(magicTagC))
            return ParseMagic;
        if (startElement == QLatin1String(matchTagC))
            return ParseMagicMatchRule;
        return ParseOtherMimeTypeSubTag;
    case ParseMagic:
        if (startElement == QLatin1String(matchTagC))
            return ParseMagicMatchRule;
        break;
    case ParseError:
        break;
    }
    return ParseError;
}

// Parse int number from an (attribute) string)
static bool parseNumber(const QString &n, int *target, QString *errorMessage)
{
    bool ok;
    *target = n.toInt(&ok);
    if (!ok) {
        *errorMessage = QString::fromLatin1("Not a number '%1'.").arg(n);
        return false;
    }
    return true;
}

// Evaluate a magic match rule like
//  <match value="must be converted with BinHex" type="string" offset="11"/>
//  <match value="0x9501" type="big16" offset="0:64"/>
static bool addMagicMatchRule(const QXmlStreamAttributes &atts,
                              const MagicRuleMatcherPtr  &ruleMatcher,
                              QString *errorMessage)
{
    const QString type = atts.value(QLatin1String(matchTypeAttributeC)).toString();
    if (type != QLatin1String(matchStringTypeValueC) &&
        type != QLatin1String(matchByteTypeValueC)) {
        qWarning("%s: match type %s is not supported.", Q_FUNC_INFO, type.toUtf8().constData());
        return true;
    }
    const QString value = atts.value(QLatin1String(matchValueAttributeC)).toString();
    if (value.isEmpty()) {
        *errorMessage = QString::fromLatin1("Empty match value detected.");
        return false;
    }
    // Parse for offset as "1" or "1:10"
    int startPos, endPos;
    const QString offsetS = atts.value(QLatin1String(matchOffsetAttributeC)).toString();
    const int colonIndex = offsetS.indexOf(QLatin1Char(':'));
    const QString startPosS = colonIndex == -1 ? offsetS : offsetS.mid(0, colonIndex);
    const QString endPosS   = colonIndex == -1 ? offsetS : offsetS.mid(colonIndex + 1);
    if (!parseNumber(startPosS, &startPos, errorMessage) || !parseNumber(endPosS, &endPos, errorMessage))
        return false;
    if (debugMimeDB)
        qDebug() << Q_FUNC_INFO << value << startPos << endPos;

    if (type == QLatin1String(matchStringTypeValueC))
        ruleMatcher->add(QSharedPointer<MagicRule>(new MagicStringRule(value, startPos, endPos)));
    else
        ruleMatcher->add(QSharedPointer<MagicRule>(new MagicByteRule(value, startPos, endPos)));
    return true;
}

bool BaseMimeTypeParser::parse(QIODevice *dev, const QString &fileName, QString *errorMessage)
{
    MimeTypeData data;
    MagicRuleMatcherPtr ruleMatcher;
    QXmlStreamReader reader(dev);
    ParseStage ps = ParseBeginning;
    QXmlStreamAttributes atts;
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement:
            ps = nextStage(ps, reader.name());
            atts = reader.attributes();
            switch (ps) {
            case ParseMimeType: { // start parsing a type
                const QString type = atts.value(QLatin1String(mimeTypeAttributeC)).toString();
                if (type.isEmpty())
                    reader.raiseError(QString::fromLatin1("Missing 'type'-attribute"));
                else
                    data.type = type;
            }
                break;
            case ParseGlobPattern:
                addGlobPattern(atts.value(QLatin1String(patternAttributeC)).toString(),
                               atts.value(QLatin1String(weightAttributeC)).toString(), &data);
                break;
            case ParseSubClass: {
                const QString inheritsFrom = atts.value(QLatin1String(mimeTypeAttributeC)).toString();
                if (!inheritsFrom.isEmpty())
                    data.subClassesOf.push_back(inheritsFrom);
            }
                break;
            case ParseComment: {
                // comments have locale attributes. We want the default, English one
                QString locale = atts.value(QLatin1String(localeAttributeC)).toString();
                const QString comment = QCoreApplication::translate("MimeType", reader.readElementText().toLatin1());
                if (locale.isEmpty())
                    data.comment = comment;
                else
                    data.localeComments.insert(locale, comment);
            }
                break;
            case ParseAlias: {
                const QString alias = atts.value(QLatin1String(mimeTypeAttributeC)).toString();
                if (!alias.isEmpty())
                    data.aliases.push_back(alias);
            }
                break;
            case ParseMagic: {
                int priority = 0;
                const QString priorityS = atts.value(QLatin1String(priorityAttributeC)).toString();
                if (!priorityS.isEmpty()) {
                    if (!parseNumber(priorityS, &priority, errorMessage))
                        return false;

                }
                ruleMatcher = MagicRuleMatcherPtr(new MagicRuleMatcher);
                ruleMatcher->setPriority(priority);
            }
                break;
            case ParseMagicMatchRule:
                QTC_ASSERT(!ruleMatcher.isNull(), return false);
                if (!addMagicMatchRule(atts, ruleMatcher, errorMessage))
                    return false;
                break;
            case ParseError:
                reader.raiseError(QString::fromLatin1("Unexpected element <%1>").arg(reader.name().toString()));
                break;
            default:
                break;
            } // switch nextStage
            break;
        // continue switch QXmlStreamReader::Token...
        case QXmlStreamReader::EndElement: // Finished element
            if (reader.name() == QLatin1String(mimeTypeTagC)) {
                if (!process(MimeType(data), errorMessage))
                    return false;
                data.clear();
            } else {
                // Finished a match sequence
                if (reader.name() == QLatin1String(magicTagC)) {
                    QTC_ASSERT(!ruleMatcher.isNull(), return false);
                    data.magicMatchers.push_back(ruleMatcher);
                    ruleMatcher = MagicRuleMatcherPtr();
                }
            }
            break;

        default:
            break;
        } // switch reader.readNext()
    }

    if (reader.hasError()) {
        *errorMessage = QString::fromLatin1("An error has been encountered at line %1 of %2: %3:").arg(reader.lineNumber()).arg(fileName, reader.errorString());
        return false;
    }
    return true;
}

} // namespace Internal

// MimeMapEntry: Entry of a type map, consisting of type and level.

enum { Dangling = 32767 };

struct MimeMapEntry
{
    explicit MimeMapEntry(const MimeType &t = MimeType(), int aLevel = Dangling);
    MimeType type;
    int level; // hierachy level
};

MimeMapEntry::MimeMapEntry(const MimeType &t, int aLevel) :
    type(t),
    level(aLevel)
{
}

/*!
    \class Core::MimeDatabase
    \brief Mime data base to which the plugins can add the mime types they handle.

    The class is protected by a QMutex and can therefore be accessed by threads.

    A good testcase is to run it over \c '/usr/share/mime/<*>/<*>.xml' on Linux.

    When adding a "text/plain" to it, the mimetype will receive a magic matcher
    that checks for text files that do not match the globs by heuristics.

    \section1 Design Considerations

    Storage requirements:
    \list
    \li Must be robust in case of incomplete hierarchies, dangling entries
    \li Plugins will not load and register their mime types in order of inheritance.
    \li Multiple inheritance (several subClassesOf) can occur
    \li Provide quick lookup by name
    \li Provide quick lookup by file type.
    \endlist

    This basically rules out some pointer-based tree, so the structure chosen is:
    \list
    \li An alias map QString->QString for mapping aliases to types
    \li A Map QString->MimeMapEntry for the types (MimeMapEntry being a pair of
       MimeType and (hierarchy) level.
    \li A map  QString->QString representing parent->child relations (enabling
       recursing over children)
    \li Using strings avoids dangling pointers.
    \endlist

    The hierarchy level is used for mapping by file types. When findByFile()
    is first called after addMimeType() it recurses over the hierarchy and sets
    the hierarchy level of the entries accordingly (0 toplevel, 1 first
    order...). It then does several passes over the type map, checking the
    globs for maxLevel, maxLevel-1....until it finds a match (idea being to
    to check the most specific types first). Starting a recursion from the
    leaves is not suitable since it will hit parent nodes several times.

    \sa Core::MimeType, Core::IMagicMatcher, Core::MagicRuleMatcher, Core::MagicRule, Core::MagicStringRule, Core::MagicByteRule, Core::GlobPattern
    \sa Core::Internal::FileMatchContext, Core::Internal::BinaryMatcher, Core::Internal::HeuristicTextMagicMatcher
    \sa Core::Internal::BaseMimeTypeParser, Core::Internal::MimeTypeParser
*/

class MimeDatabasePrivate
{
    Q_DISABLE_COPY(MimeDatabasePrivate)

public:
    MimeDatabasePrivate();

    bool addMimeTypes(const QString &fileName, QString *errorMessage);
    bool addMimeTypes(QIODevice *device, QString *errorMessage);
    bool addMimeType(MimeType mt);

    MimeType findByType(const QString &type) const;
    MimeType findByFile(const QFileInfo &f) const;
    MimeType findByData(const QByteArray &data) const;

    QStringList filterStrings() const;

    QStringList suffixes() const;
    bool setPreferredSuffix(const QString &typeOrAlias, const QString &suffix);

    QList<MimeGlobPattern> globPatterns() const;
    void setGlobPatterns(const QString &typeOrAlias, const QList<MimeGlobPattern> &globPatterns);

    QList<QSharedPointer<IMagicMatcher> > magicMatchers() const;
    void setMagicMatchers(const QString &typeOrAlias,
                          const QList<QSharedPointer<IMagicMatcher> > &matchers);

    QList<MimeType> mimeTypes() const;

    void syncUserModifiedMimeTypes();
    static QList<MimeType> readUserModifiedMimeTypes();
    static void writeUserModifiedMimeTypes(const QList<MimeType> &mimeTypes);
    void clearUserModifiedMimeTypes();

    static QList<MimeGlobPattern> toGlobPatterns(const QStringList &patterns,
                                                 int weight = MimeGlobPattern::MaxWeight);
    static QStringList fromGlobPatterns(const QList<MimeGlobPattern> &globPatterns);

    void debug(QTextStream &str) const;

private:
    typedef QHash<QString, MimeMapEntry> TypeMimeTypeMap;
    typedef QHash<QString, QString> AliasMap;
    typedef QMultiHash<QString, QString> ParentChildrenMap;

    static const QChar kSemiColon;
    static const QString kModifiedMimeTypesFile;
    static QString kModifiedMimeTypesPath;


    bool addMimeTypes(QIODevice *device, const QString &fileName, QString *errorMessage);
    inline const QString &resolveAlias(const QString &name) const;
    MimeType findByFile(const QFileInfo &f, unsigned *priority) const;
    MimeType findByData(const QByteArray &data, unsigned *priority) const;
    void determineLevels();
    void raiseLevelRecursion(MimeMapEntry &e, int level);

    TypeMimeTypeMap m_typeMimeTypeMap;
    AliasMap m_aliasMap;
    ParentChildrenMap m_parentChildrenMap;
    int m_maxLevel;
};

const QChar MimeDatabasePrivate::kSemiColon(QLatin1Char(';'));
const QString MimeDatabasePrivate::kModifiedMimeTypesFile(QLatin1String("modifiedmimetypes.xml"));
QString MimeDatabasePrivate::kModifiedMimeTypesPath;

MimeDatabasePrivate::MimeDatabasePrivate() :
    m_maxLevel(-1)
{
    // Assign here to avoid non-local static data initialization issues.
    kModifiedMimeTypesPath = ICore::userResourcePath() + QLatin1String("/mimetypes/");
}

/*!
    \class Core::Internal::MimeTypeParser
    \brief Mime type parser

    Populates Core::MimeDataBase

    \sa Core::MimeDatabase, Core::IMagicMatcher, Core::MagicRuleMatcher, Core::MagicRule, Core::MagicStringRule, Core::MagicByteRule, Core::GlobPattern
    \sa Core::Internal::FileMatchContext, Core::Internal::BinaryMatcher, Core::Internal::HeuristicTextMagicMatcher
    \sa Core::Internal::MimeTypeParser
*/

namespace Internal {
    // Parser that builds MimeDB hierarchy by adding to MimeDatabasePrivate
    class MimeTypeParser : public BaseMimeTypeParser {
    public:
        explicit MimeTypeParser(MimeDatabasePrivate &db) : m_db(db) {}
    private:
        virtual bool process(const MimeType &t, QString *) { m_db.addMimeType(t); return true; }

        MimeDatabasePrivate &m_db;
    };
} // namespace Internal

bool MimeDatabasePrivate::addMimeTypes(QIODevice *device, const QString &fileName, QString *errorMessage)
{
    Internal::MimeTypeParser parser(*this);
    return parser.parse(device, fileName, errorMessage);
}

bool MimeDatabasePrivate::addMimeTypes(const QString &fileName, QString *errorMessage)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) {
        *errorMessage = QString::fromLatin1("Cannot open %1: %2").arg(fileName, file.errorString());
        return false;
    }
    return addMimeTypes(&file, fileName, errorMessage);
}

bool MimeDatabasePrivate::addMimeTypes(QIODevice *device, QString *errorMessage)
{
    return addMimeTypes(device, QLatin1String("<stream>"), errorMessage);
}

bool MimeDatabasePrivate::addMimeType(MimeType mt)
{
    if (!mt)
        return false;

    const QString type = mt.type();
    // Hack: Add a magic text matcher to "text/plain" and the fallback matcher to
    // binary types "application/octet-stream"
    if (type == QLatin1String(textTypeC)) {
        mt.addMagicMatcher(QSharedPointer<IMagicMatcher>(new Internal::HeuristicTextMagicMatcher));
    } else {
        if (type == QLatin1String(binaryTypeC))
            mt.addMagicMatcher(QSharedPointer<IMagicMatcher>(new Internal::BinaryMatcher));
    }
    // insert the type.
    m_typeMimeTypeMap.insert(type, MimeMapEntry(mt));
    // Register the children
    // Aliases will be resolved later once all mime types are known.
    const QStringList subClassesOf = mt.subClassesOf();
    if (!subClassesOf.empty()) {
        const QStringList::const_iterator socend = subClassesOf.constEnd();
        for (QStringList::const_iterator soit = subClassesOf.constBegin(); soit !=  socend; ++soit)
            m_parentChildrenMap.insert(*soit, type);
    }
    // register aliasses
    const QStringList aliases = mt.aliases();
    if (!aliases.empty()) {
        const QStringList::const_iterator cend = aliases.constEnd();
        for (QStringList::const_iterator it = aliases.constBegin(); it !=  cend; ++it)
            m_aliasMap.insert(*it, type);
    }
    m_maxLevel = -1; // Mark as dirty
    return true;
}

const QString &MimeDatabasePrivate::resolveAlias(const  QString &name) const
{
    const AliasMap::const_iterator aliasIt = m_aliasMap.constFind(name);
    return aliasIt == m_aliasMap.constEnd() ? name : aliasIt.value();
}

void MimeDatabasePrivate::raiseLevelRecursion(MimeMapEntry &e, int level)
{
    if (e.level == Dangling || e.level < level)
        e.level = level;
    if (m_maxLevel < level)
        m_maxLevel = level;
    // At all events recurse over children since nodes might have been
    // added.
    QStringList childTypes = m_parentChildrenMap.values(e.type.type());
    foreach (const QString &alias, e.type.aliases())
        childTypes.append(m_parentChildrenMap.values(alias));
    if (childTypes.empty())
        return;
    // look them up in the type->mime type map
    const int nextLevel = level + 1;
    const TypeMimeTypeMap::iterator tm_end = m_typeMimeTypeMap.end();
    const QStringList::const_iterator cend = childTypes.constEnd();
    for (QStringList::const_iterator it = childTypes.constBegin(); it !=  cend; ++it) {
        const TypeMimeTypeMap::iterator tm_it = m_typeMimeTypeMap.find(resolveAlias(*it));
        if (tm_it == tm_end) {
            qWarning("%s: Inconsistent mime hierarchy detected, child %s of %s cannot be found.",
                     Q_FUNC_INFO, it->toUtf8().constData(), e.type.type().toUtf8().constData());
        } else {
            raiseLevelRecursion(*tm_it, nextLevel);
        }
    }
}

void MimeDatabasePrivate::determineLevels()
{
    // Loop over toplevels and recurse down their hierarchies.
    // Determine top levels by subtracting the children from the parent
    // set. Note that a toplevel at this point might have 'subclassesOf'
    // set to some class that is not in the DB, so, checking for an empty
    // 'subclassesOf' set is not sufficient to find the toplevels.
    // First, take the parent->child entries  whose parent exists and build
    // sets of parents/children
    QSet<QString> parentSet, childrenSet;
    const ParentChildrenMap::const_iterator pcend = m_parentChildrenMap.constEnd();
    for (ParentChildrenMap::const_iterator it =  m_parentChildrenMap.constBegin(); it !=  pcend; ++it)
        if (m_typeMimeTypeMap.contains(it.key())) {
            parentSet.insert(it.key());
            childrenSet.insert(it.value());
        }
    const QSet<QString> topLevels = parentSet.subtract(childrenSet);
    if (debugMimeDB)
        qDebug() << Q_FUNC_INFO << "top levels" << topLevels;
    const TypeMimeTypeMap::iterator tm_end = m_typeMimeTypeMap.end();
    const QSet<QString>::const_iterator tl_cend = topLevels.constEnd();
    for (QSet<QString>::const_iterator tl_it =  topLevels.constBegin(); tl_it !=  tl_cend; ++tl_it) {
        const TypeMimeTypeMap::iterator tm_it = m_typeMimeTypeMap.find(resolveAlias(*tl_it));
        if (tm_it == tm_end) {
            qWarning("%s: Inconsistent mime hierarchy detected, top level element %s cannot be found.",
                     Q_FUNC_INFO, tl_it->toUtf8().constData());
        } else {
            raiseLevelRecursion(tm_it.value(), 0);
        }
    }
}

bool MimeDatabasePrivate::setPreferredSuffix(const QString &typeOrAlias, const QString &suffix)
{
    TypeMimeTypeMap::iterator tit =  m_typeMimeTypeMap.find(resolveAlias(typeOrAlias));
    if (tit != m_typeMimeTypeMap.end())
        return tit.value().type.setPreferredSuffix(suffix);
    return false;
}

// Returns a mime type or Null one if none found
MimeType MimeDatabasePrivate::findByType(const QString &typeOrAlias) const
{
    const TypeMimeTypeMap::const_iterator tit =  m_typeMimeTypeMap.constFind(resolveAlias(typeOrAlias));
    if (tit != m_typeMimeTypeMap.constEnd())
        return tit.value().type;
    return MimeType();
}

// Debugging wrapper around findByFile()
MimeType MimeDatabasePrivate::findByFile(const QFileInfo &f) const
{
    unsigned priority = 0;
    if (debugMimeDB)
        qDebug() << '>' << Q_FUNC_INFO << f.absoluteFilePath();
    const MimeType rc = findByFile(f, &priority);
    if (debugMimeDB) {
        if (rc)
            qDebug() << "<MimeDatabase::findByFile: match prio=" << priority << rc.type();
        else
            qDebug() << "<MimeDatabase::findByFile: no match";
    }
    return rc;
}

// Returns a mime type or Null one if none found
MimeType MimeDatabasePrivate::findByFile(const QFileInfo &f, unsigned *priorityPtr) const
{
    // Is the hierarchy set up in case we find several matches?
    if (m_maxLevel < 0) {
        MimeDatabasePrivate *db = const_cast<MimeDatabasePrivate *>(this);
        db->determineLevels();
    }

    // First, glob patterns are evaluated. If there is a match with max weight,
    // this one is selected and we are done. Otherwise, the file contents are
    // evaluated and the match with the highest value (either a magic priority or
    // a glob pattern weight) is selected. Matching starts from max level (most
    // specific) in both cases, even when there is already a suffix matching candidate.
    *priorityPtr = 0;
    MimeType candidate;
    Internal::FileMatchContext context(f);

    // Pass 1) Try to match on suffix
    const TypeMimeTypeMap::const_iterator cend = m_typeMimeTypeMap.constEnd();
    for (int level = m_maxLevel; level >= 0; level--) {
        for (TypeMimeTypeMap::const_iterator it = m_typeMimeTypeMap.constBegin(); it != cend; ++it) {
            if (it.value().level == level) {
                const unsigned suffixPriority = it.value().type.matchesFileBySuffix(context);
                if (suffixPriority && suffixPriority > *priorityPtr) {
                    *priorityPtr = suffixPriority;
                    candidate = it.value().type;
                    if (suffixPriority >= MimeGlobPattern::MaxWeight)
                        return candidate;
                }
            }
        }
    }

    // Pass 2) Match on content
    if (!f.isReadable())
        return candidate;
    for (int level = m_maxLevel; level >= 0; level--) {
        for (TypeMimeTypeMap::const_iterator it = m_typeMimeTypeMap.constBegin(); it != cend; ++it) {
            if (it.value().level == level) {
                const unsigned contentPriority = it.value().type.matchesFileByContent(context);
                if (contentPriority && contentPriority > *priorityPtr) {
                    *priorityPtr = contentPriority;
                    candidate = it.value().type;
                }
            }
        }
    }

    return candidate;
}

// Debugging wrapper around findByData()
MimeType MimeDatabasePrivate::findByData(const QByteArray &data) const
{
    unsigned priority = 0;
    if (debugMimeDB)
        qDebug() << '>' << Q_FUNC_INFO << data.left(20).toHex();
    const MimeType rc = findByData(data, &priority);
    if (debugMimeDB) {
        if (rc)
            qDebug() << "<MimeDatabase::findByData: match prio=" << priority << rc.type();
        else
            qDebug() << "<MimeDatabase::findByData: no match";
    }
    return rc;
}

// Returns a mime type or Null one if none found
MimeType MimeDatabasePrivate::findByData(const QByteArray &data, unsigned *priorityPtr) const
{
    // Is the hierarchy set up in case we find several matches?
    if (m_maxLevel < 0) {
        MimeDatabasePrivate *db = const_cast<MimeDatabasePrivate *>(this);
        db->determineLevels();
    }

    *priorityPtr = 0;
    MimeType candidate;

    const TypeMimeTypeMap::const_iterator cend = m_typeMimeTypeMap.constEnd();
    for (int level = m_maxLevel; level >= 0; level--)
        for (TypeMimeTypeMap::const_iterator it = m_typeMimeTypeMap.constBegin(); it != cend; ++it)
            if (it.value().level == level) {
                const unsigned contentPriority = it.value().type.matchesData(data);
                if (contentPriority && contentPriority > *priorityPtr) {
                    *priorityPtr = contentPriority;
                    candidate = it.value().type;
                }
            }

    return candidate;
}

// Return all known suffixes
QStringList MimeDatabasePrivate::suffixes() const
{
    QStringList rc;
    const TypeMimeTypeMap::const_iterator cend = m_typeMimeTypeMap.constEnd();
    for (TypeMimeTypeMap::const_iterator it = m_typeMimeTypeMap.constBegin(); it != cend; ++it)
        rc += it.value().type.suffixes();
    return rc;
}

QStringList MimeDatabasePrivate::filterStrings() const
{
    QStringList rc;
    const TypeMimeTypeMap::const_iterator cend = m_typeMimeTypeMap.constEnd();
    for (TypeMimeTypeMap::const_iterator it = m_typeMimeTypeMap.constBegin(); it != cend; ++it) {
        const QString filterString = it.value().type.filterString();
        if (!filterString.isEmpty())
            rc += filterString;
    }
    return rc;
}

QList<MimeGlobPattern> MimeDatabasePrivate::globPatterns() const
{
    QList<MimeGlobPattern> globPatterns;
    const TypeMimeTypeMap::const_iterator cend = m_typeMimeTypeMap.constEnd();
    for (TypeMimeTypeMap::const_iterator it = m_typeMimeTypeMap.constBegin(); it != cend; ++it)
        globPatterns.append(it.value().type.globPatterns());
    return globPatterns;
}

void MimeDatabasePrivate::setGlobPatterns(const QString &typeOrAlias,
                                          const QList<MimeGlobPattern> &globPatterns)
{
    TypeMimeTypeMap::iterator tit =  m_typeMimeTypeMap.find(resolveAlias(typeOrAlias));
    if (tit != m_typeMimeTypeMap.end())
        tit.value().type.setGlobPatterns(globPatterns);
}

QList<QSharedPointer<IMagicMatcher> > MimeDatabasePrivate::magicMatchers() const
{
    QList<QSharedPointer<IMagicMatcher> > magicMatchers;
    const TypeMimeTypeMap::const_iterator cend = m_typeMimeTypeMap.constEnd();
    for (TypeMimeTypeMap::const_iterator it = m_typeMimeTypeMap.constBegin(); it != cend; ++it)
        magicMatchers.append(it.value().type.magicMatchers());
    return magicMatchers;
}

void MimeDatabasePrivate::setMagicMatchers(const QString &typeOrAlias,
                                           const QList<QSharedPointer<IMagicMatcher> > &matchers)
{
    TypeMimeTypeMap::iterator tit =  m_typeMimeTypeMap.find(resolveAlias(typeOrAlias));
    if (tit != m_typeMimeTypeMap.end())
        tit.value().type.setMagicMatchers(matchers);
}

QList<MimeType> MimeDatabasePrivate::mimeTypes() const
{
    QList<MimeType> mimeTypes;
    const TypeMimeTypeMap::const_iterator cend = m_typeMimeTypeMap.constEnd();
    for (TypeMimeTypeMap::const_iterator it = m_typeMimeTypeMap.constBegin(); it != cend; ++it)
        mimeTypes.append(it.value().type);
    return mimeTypes;
}

void MimeDatabasePrivate::syncUserModifiedMimeTypes()
{
    QHash<QString, MimeType> userModified;
    const QList<MimeType> &userMimeTypes = readUserModifiedMimeTypes();
    foreach (const MimeType &userMimeType, userMimeTypes)
        userModified.insert(userMimeType.type(), userMimeType);

    TypeMimeTypeMap::iterator end = m_typeMimeTypeMap.end();
    QHash<QString, MimeType>::const_iterator userMimeEnd = userModified.end();
    for (TypeMimeTypeMap::iterator it = m_typeMimeTypeMap.begin(); it != end; ++it) {
        QHash<QString, MimeType>::const_iterator userMimeIt =
            userModified.find(it.value().type.type());
        if (userMimeIt != userMimeEnd) {
            it.value().type.setGlobPatterns(userMimeIt.value().globPatterns());
            it.value().type.setMagicRuleMatchers(userMimeIt.value().magicRuleMatchers());
        }
    }
}

QList<MimeType> MimeDatabasePrivate::readUserModifiedMimeTypes()
{
    typedef MagicRuleMatcher::MagicRuleList MagicRuleList;
    typedef MagicRuleMatcher::MagicRuleSharedPointer MagicRuleSharedPointer;

    QList<MimeType> mimeTypes;
    QFile file(kModifiedMimeTypesPath + kModifiedMimeTypesFile);
    if (file.open(QFile::ReadOnly)) {
        MimeType mimeType;
        QHash<int, MagicRuleList> rules;
        QXmlStreamReader reader(&file);
        QXmlStreamAttributes atts;
        const QString mimeTypeAttribute = QLatin1String(mimeTypeAttributeC);
        const QString patternAttribute = QLatin1String(patternAttributeC);
        const QString matchValueAttribute = QLatin1String(matchValueAttributeC);
        const QString matchTypeAttribute = QLatin1String(matchTypeAttributeC);
        const QString matchOffsetAttribute = QLatin1String(matchOffsetAttributeC);
        const QString priorityAttribute = QLatin1String(priorityAttributeC);
        while (!reader.atEnd()) {
            switch (reader.readNext()) {
            case QXmlStreamReader::StartElement:
                atts = reader.attributes();
                if (reader.name() == QLatin1String(mimeTypeTagC)) {
                    mimeType.setType(atts.value(mimeTypeAttribute).toString());
                    const QString &patterns = atts.value(patternAttribute).toString();
                    mimeType.setGlobPatterns(toGlobPatterns(patterns.split(kSemiColon)));
                } else if (reader.name() == QLatin1String(matchTagC)) {
                    const QString &value = atts.value(matchValueAttribute).toString();
                    const QString &type = atts.value(matchTypeAttribute).toString();
                    const QString &offset = atts.value(matchOffsetAttribute).toString();
                    QPair<int, int> range = MagicRule::fromOffset(offset);
                    const int priority = atts.value(priorityAttribute).toString().toInt();

                    MagicRule *magicRule;
                    if (type == MagicStringRule::kMatchType)
                        magicRule = new MagicStringRule(value, range.first, range.second);
                    else
                        magicRule = new MagicByteRule(value, range.first, range.second);
                    rules[priority].append(MagicRuleSharedPointer(magicRule));
                }
                break;
            case QXmlStreamReader::EndElement:
                if (reader.name() == QLatin1String(mimeTypeTagC)) {
                    mimeType.setMagicRuleMatchers(MagicRuleMatcher::createMatchers(rules));
                    mimeTypes.append(mimeType);
                    mimeType.clear();
                    rules.clear();
                }
                break;
            default:
                break;
            }
        }
        if (reader.hasError())
            qWarning() << kModifiedMimeTypesFile << reader.errorString() << reader.lineNumber()
                       << reader.columnNumber();
        file.close();
    }
    return mimeTypes;
}

void MimeDatabasePrivate::writeUserModifiedMimeTypes(const QList<MimeType> &mimeTypes)
{
    // Keep mime types modified which are already on file, unless they are part of the current set.
    QSet<QString> currentMimeTypes;
    foreach (const MimeType &mimeType, mimeTypes)
        currentMimeTypes.insert(mimeType.type());
    const QList<MimeType> &inFileMimeTypes = readUserModifiedMimeTypes();
    QList<MimeType> allModifiedMimeTypes = mimeTypes;
    foreach (const MimeType &mimeType, inFileMimeTypes)
        if (!currentMimeTypes.contains(mimeType.type()))
            allModifiedMimeTypes.append(mimeType);

    if (QFile::exists(kModifiedMimeTypesPath) || QDir().mkpath(kModifiedMimeTypesPath)) {
        QFile file(kModifiedMimeTypesPath + kModifiedMimeTypesFile);
        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
            // Notice this file only represents user modifications. It is writen in a
            // convienient way for synchronization, which is similar to but not exactly the
            // same format we use for the embedded mime type files.
            QXmlStreamWriter writer(&file);
            writer.setAutoFormatting(true);
            writer.writeStartDocument();
            writer.writeStartElement(QLatin1String(mimeInfoTagC));
            const QString mimeTypeTag = QLatin1String(mimeTypeTagC);
            const QString matchTag = QLatin1String(matchTagC);
            const QString mimeTypeAttribute = QLatin1String(mimeTypeAttributeC);
            const QString patternAttribute = QLatin1String(patternAttributeC);
            const QString matchValueAttribute = QLatin1String(matchValueAttributeC);
            const QString matchTypeAttribute = QLatin1String(matchTypeAttributeC);
            const QString matchOffsetAttribute = QLatin1String(matchOffsetAttributeC);
            const QString priorityAttribute = QLatin1String(priorityAttributeC);

            foreach (const MimeType &mimeType, allModifiedMimeTypes) {
                writer.writeStartElement(mimeTypeTag);
                writer.writeAttribute(mimeTypeAttribute, mimeType.type());
                writer.writeAttribute(patternAttribute,
                                      fromGlobPatterns(mimeType.globPatterns()).join(kSemiColon));
                const QList<QSharedPointer<IMagicMatcher> > &matchers = mimeType.magicMatchers();
                foreach (const QSharedPointer<IMagicMatcher> &matcher, matchers) {
                    // Only care about rule-based matchers.
                    if (MagicRuleMatcher *ruleMatcher =
                        dynamic_cast<MagicRuleMatcher *>(matcher.data())) {
                        const MagicRuleMatcher::MagicRuleList &rules = ruleMatcher->magicRules();
                        foreach (const MagicRuleMatcher::MagicRuleSharedPointer &rule, rules) {
                            writer.writeStartElement(matchTag);
                            writer.writeAttribute(matchValueAttribute, rule->matchValue());
                            writer.writeAttribute(matchTypeAttribute, rule->matchType());
                            writer.writeAttribute(matchOffsetAttribute,
                                                  MagicRule::toOffset(
                                                      qMakePair(rule->startPos(), rule->endPos())));
                            writer.writeAttribute(priorityAttribute,
                                                  QString::number(ruleMatcher->priority()));
                            writer.writeEndElement();
                        }
                    }
                }
                writer.writeEndElement();
            }
            writer.writeEndElement();
            writer.writeEndDocument();
            file.close();
        }
    }
}

void MimeDatabasePrivate::clearUserModifiedMimeTypes()
{
    // This removes the user's file. However, the operation will actually take place the next time
    // Creator starts, since we currently don't support removing stuff from the mime database.
    QFile::remove(kModifiedMimeTypesPath + kModifiedMimeTypesFile);
}

QList<MimeGlobPattern> MimeDatabasePrivate::toGlobPatterns(const QStringList &patterns, int weight)
{
    QList<MimeGlobPattern> globPatterns;
    foreach (const QString &pattern, patterns)
        globPatterns.append(Core::MimeGlobPattern(pattern, weight));
    return globPatterns;
}

QStringList MimeDatabasePrivate::fromGlobPatterns(const QList<MimeGlobPattern> &globPatterns)
{
    QStringList patterns;
    foreach (const MimeGlobPattern &globPattern, globPatterns)
        patterns.append(globPattern.pattern());
    return patterns;
}

void MimeDatabasePrivate::debug(QTextStream &str) const
{
    str << ">MimeDatabase\n";
    const TypeMimeTypeMap::const_iterator cend = m_typeMimeTypeMap.constEnd();
    for (TypeMimeTypeMap::const_iterator it = m_typeMimeTypeMap.constBegin(); it != cend; ++it) {
        str << "Entry level " << it.value().level << '\n';
        it.value().type.m_d->debug(str);
    }
    str << "<MimeDatabase\n";
}

MimeDatabase::MimeDatabase() :
    d(new MimeDatabasePrivate)
{
}

MimeDatabase::~MimeDatabase()
{
    delete d;
}

MimeType MimeDatabase::findByType(const QString &typeOrAlias) const
{
    m_mutex.lock();
    const MimeType rc = d->findByType(typeOrAlias);
    m_mutex.unlock();
    return rc;
}

MimeType MimeDatabase::findByFileUnlocked(const QFileInfo &f) const
{
    return d->findByFile(f);
}

MimeType MimeDatabase::findByFile(const QFileInfo &f) const
{
    m_mutex.lock();
    const MimeType rc = findByFileUnlocked(f);
    m_mutex.unlock();
    return rc;
}

MimeType MimeDatabase::findByData(const QByteArray &data) const
{
    m_mutex.lock();
    const MimeType rc = d->findByData(data);
    m_mutex.unlock();
    return rc;
}

bool MimeDatabase::addMimeType(const  MimeType &mt)
{
    m_mutex.lock();
    const bool rc = d->addMimeType(mt);
    m_mutex.unlock();
    return rc;
}

bool MimeDatabase::addMimeTypes(const QString &fileName, QString *errorMessage)
{
    m_mutex.lock();
    const bool rc = d->addMimeTypes(fileName, errorMessage);
    m_mutex.unlock();
    return rc;
}

bool MimeDatabase::addMimeTypes(QIODevice *device, QString *errorMessage)
{
    m_mutex.lock();
    const bool rc = d->addMimeTypes(device, errorMessage);
    m_mutex.unlock();
    return rc;
}

QStringList MimeDatabase::suffixes() const
{
    m_mutex.lock();
    const QStringList rc = d->suffixes();
    m_mutex.unlock();
    return rc;
}

QStringList MimeDatabase::filterStrings() const
{
    m_mutex.lock();
    const QStringList rc = d->filterStrings();
    m_mutex.unlock();
    return rc;
}
QString MimeDatabase::allFiltersString(QString *allFilesFilter) const
{
    if (allFilesFilter)
        allFilesFilter->clear();

    // Compile list of filter strings, sort, and remove duplicates (different mime types might
    // generate the same filter).
    QStringList filters = filterStrings();
    if (filters.empty())
        return QString();
    filters.sort();
    filters.erase(std::unique(filters.begin(), filters.end()), filters.end());

    static const QString allFiles =
        QCoreApplication::translate("Core", Constants::ALL_FILES_FILTER);
    if (allFilesFilter)
        *allFilesFilter = allFiles;

    // Prepend all files filter (instead of appending to work around a bug in Qt/Mac).
    filters.prepend(allFiles);

    return filters.join(QLatin1String(";;"));
}

QList<MimeGlobPattern> MimeDatabase::globPatterns() const
{
    m_mutex.lock();
    const QList<MimeGlobPattern> rc = d->globPatterns();
    m_mutex.unlock();
    return rc;
}

void MimeDatabase::setGlobPatterns(const QString &typeOrAlias,
                                   const QList<MimeGlobPattern> &globPatterns)
{
    m_mutex.lock();
    d->setGlobPatterns(typeOrAlias, globPatterns);
    m_mutex.unlock();
}

MimeDatabase::IMagicMatcherList MimeDatabase::magicMatchers() const
{
    m_mutex.lock();
    const IMagicMatcherList rc = d->magicMatchers();
    m_mutex.unlock();
    return rc;
}

void MimeDatabase::setMagicMatchers(const QString &typeOrAlias,
                                    const IMagicMatcherList &matchers)
{
    m_mutex.lock();
    d->setMagicMatchers(typeOrAlias, matchers);
    m_mutex.unlock();
}

QList<MimeType> MimeDatabase::mimeTypes() const
{
    m_mutex.lock();
    const QList<MimeType> &mimeTypes = d->mimeTypes();
    m_mutex.unlock();
    return mimeTypes;
}

void MimeDatabase::syncUserModifiedMimeTypes()
{
    m_mutex.lock();
    d->syncUserModifiedMimeTypes();
    m_mutex.unlock();
}

void MimeDatabase::clearUserModifiedMimeTypes()
{
    m_mutex.lock();
    d->clearUserModifiedMimeTypes();
    m_mutex.unlock();
}

QList<MimeType> MimeDatabase::readUserModifiedMimeTypes()
{
    return MimeDatabasePrivate::readUserModifiedMimeTypes();
}

void MimeDatabase::writeUserModifiedMimeTypes(const QList<MimeType> &mimeTypes)
{
    MimeDatabasePrivate::writeUserModifiedMimeTypes(mimeTypes);
}

QString MimeDatabase::preferredSuffixByType(const QString &type) const
{
    if (const MimeType mt = findByType(type))
        return mt.preferredSuffix(); // already does Mutex locking
    return QString();
}

QString MimeDatabase::preferredSuffixByFile(const QFileInfo &f) const
{
    if (const MimeType mt = findByFile(f))
        return mt.preferredSuffix(); // already does Mutex locking
    return QString();
}

bool MimeDatabase::setPreferredSuffix(const QString &typeOrAlias, const QString &suffix)
{
    m_mutex.lock();
    const bool rc = d->setPreferredSuffix(typeOrAlias, suffix);
    m_mutex.unlock();
    return rc;
}

QList<MimeGlobPattern> MimeDatabase::toGlobPatterns(const QStringList &patterns, int weight)
{
    return MimeDatabasePrivate::toGlobPatterns(patterns, weight);
}

QStringList MimeDatabase::fromGlobPatterns(const QList<MimeGlobPattern> &globPatterns)
{
    return MimeDatabasePrivate::fromGlobPatterns(globPatterns);
}

QDebug operator<<(QDebug d, const MimeDatabase &mt)
{
    QString s;
    {
        QTextStream str(&s);
        mt.d->debug(str);
    }
    d << s;
    return d;
}

} // namespace Core

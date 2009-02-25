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

#include "mimedatabase.h"

#include <utils/qtcassert.h>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QMap>
#include <QtCore/QMultiHash>
#include <QtCore/QRegExp>
#include <QtCore/QSharedData>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>

#include <QtXml/QXmlStreamReader>

enum { debugMimeDB = 0 };

// XML tags in mime files
static const char *mimeInfoTagC = "mime-info";
static const char *mimeTypeTagC = "mime-type";
static const char *mimeTypeAttributeC = "type";
static const char *subClassTagC = "sub-class-of";
static const char *commentTagC = "comment";
static const char *globTagC = "glob";
static const char *aliasTagC = "alias";
static const char *patternAttributeC = "pattern";
static const char *localeAttributeC = "xml:lang";

static const char *magicTagC = "magic";
static const char *priorityAttributeC = "priority";
static const char *matchTagC = "match";
static const char *matchValueAttributeC = "value";
static const char *matchTypeAttributeC = "type";
static const char *matchStringTypeValueC = "string";
static const char *matchOffsetAttributeC = "offset";

// Types
static const char *textTypeC = "text/plain";
static const char *binaryTypeC = "application/octet-stream";

// UTF16 byte order marks
static const char bigEndianByteOrderMarkC[] = "\xFE\xFF";
static const char littleEndianByteOrderMarkC[] = "\xFF\xFE";

// Fallback priorities, must be low.
enum { BinaryMatchPriority = 1, TextMatchPriority = 2};

/* Parse sth like (<mime-info> being optional):
 *\code
?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
  <!-- Mime types must match the desktop file associations -->
  <mime-type type="application/vnd.nokia.qt.qmakeprofile">
    <comment xml:lang="en">Qt QMake Profile</comment>
    <glob pattern="*.pro"/>
  </mime-type>
</mime-info>
 *\endcode
*/

namespace Core {
namespace Internal {

// FileMatchContext: Passed on to the mimetypes from the database
// when looking for a file match. It exists to enable reading the file
// contents "on demand" (as opposed to each mime type trying to open
// and read while checking).

class FileMatchContext {
    Q_DISABLE_COPY(FileMatchContext);
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

// The binary fallback matcher for "application/octet-stream".
class BinaryMatcher : public IMagicMatcher {
    Q_DISABLE_COPY(BinaryMatcher);
public:
    BinaryMatcher() {}
    virtual bool matches(const QByteArray & /*data*/) const { return true; }
    virtual int priority() const  { return BinaryMatchPriority; }
};

// A heuristic text file matcher: If the data do not contain any character
// below tab (9), detect as text.
class HeuristicTextMagicMatcher : public IMagicMatcher {
    Q_DISABLE_COPY(HeuristicTextMagicMatcher);
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

// MagicRule
MagicRule::MagicRule(const QByteArray &pattern, int startPos, int endPos) :
    m_pattern(pattern),
    m_startPos(startPos),
    m_endPos(endPos)
{
}

bool MagicRule::matches(const QByteArray &data) const
{
    // Quick check
    const int dataSize = data.size();
    if ((m_startPos + m_pattern.size()) >= dataSize)
        return false;
    // Most common: some string at position 0:
    if (m_startPos == 0 && m_startPos == m_endPos)
        return data.startsWith(m_pattern);
    // Range
    const int index = data.indexOf(m_pattern, m_startPos);
    return index != -1 && index < m_endPos;
}

MagicRule *MagicRule::createStringRule(const QString &c, int startPos, int endPos)
{
    return new MagicRule(c.toUtf8(), startPos, endPos);
}

// List matcher
MagicRuleMatcher::MagicRuleMatcher() :
     m_priority(65535)
{
}

void MagicRuleMatcher::add(const MagicRuleSharedPointer &rule)
{
    m_list.push_back(rule);
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

// ---------- MimeTypeData
class MimeTypeData : public QSharedData {
public:
    typedef QHash<QString,QString> LocaleHash;
    void clear();
    void debug(QTextStream &str, int indent = 0) const;

    QString type;
    QString comment;

    LocaleHash localeComments;
    QStringList aliases;
    QList<QRegExp> globPatterns;
    QStringList subClassesOf;
    QString preferredSuffix;
    QStringList suffixes;

    typedef QSharedPointer<IMagicMatcher> IMagicMatcherSharedPointer;
    typedef QList<IMagicMatcherSharedPointer> IMagicMatcherList;
    IMagicMatcherList magicMatchers;
};

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
        foreach (const QRegExp &r, globPatterns)
            str << r.pattern() << ' ';
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

QList<QRegExp> MimeType::globPatterns() const
{
    return m_d->globPatterns;
}

void MimeType::setGlobPatterns(const QList<QRegExp> &g)
{
    m_d->globPatterns = g;
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

static QString formatFilterString(const QString &description, const QList<QRegExp> &globs)
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
    return matchesFile(context);
}

unsigned MimeType::matchesFile(Internal::FileMatchContext &c) const
{
    // check globs
    const QList<QRegExp>::const_iterator cend = m_d->globPatterns.constEnd();
    for (QList<QRegExp>::const_iterator it = m_d->globPatterns.constBegin(); it != cend; ++it)
        if (it->exactMatch(c.fileName()))
            return GlobMatchPriority;
    // Nope, try magic matchers on context data
    if (m_d->magicMatchers.empty())
        return 0;
    const QByteArray data = c.data();
    if (!data.isEmpty()) {
        const MimeTypeData::IMagicMatcherList::const_iterator cend = m_d->magicMatchers.constEnd();
        for (MimeTypeData::IMagicMatcherList::const_iterator it = m_d->magicMatchers.constBegin(); it != cend; ++it)
            if ((*it)->matches(data))
                return (*it)->priority();
    }
    return 0;
}

QStringList MimeType::suffixes() const
{
    return m_d->suffixes;
}

void MimeType::setSuffixes(const QStringList &s)
{
    m_d->suffixes = s;
}

void MimeType::addMagicMatcher(const QSharedPointer<IMagicMatcher> &matcher)
{
    m_d->magicMatchers.push_back(matcher);
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

//  MimeDatabase helpers: Generic parser for a sequence of <mime-type>.
//  Calls abstract handler function process for MimeType it finds.
class BaseMimeTypeParser {
    Q_DISABLE_COPY(BaseMimeTypeParser);
public:
    BaseMimeTypeParser();
    virtual ~BaseMimeTypeParser() {}

    bool parse(QIODevice *dev, const QString &fileName, QString *errorMessage);

private:
    // Overwrite to process the sequence of parsed data
    virtual bool process(const MimeType &t, QString *errorMessage) = 0;

    void addGlobPattern(const QString &pattern, MimeTypeData *d) const;

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

    const QRegExp m_suffixPattern;
};

BaseMimeTypeParser:: BaseMimeTypeParser() :
    // RE to match a suffix glob pattern: "*.ext" (and not sth like "Makefile" or
    // "*.log[1-9]"
    m_suffixPattern(QLatin1String("^\\*\\.[\\w]+$"))
{
    QTC_ASSERT(m_suffixPattern.isValid(), /**/);
}

void BaseMimeTypeParser::addGlobPattern(const QString &pattern, MimeTypeData *d) const
{
    if (pattern.isEmpty())
        return;
    // Collect patterns as a QRegExp list and filter out the plain
    // suffix ones for our suffix list. Use first one as preferred
    const QRegExp wildCard(pattern, Qt::CaseSensitive, QRegExp::Wildcard);
    if (!wildCard.isValid()) {
        qWarning("%s: Invalid wildcard '%s'.",
                 Q_FUNC_INFO, pattern.toUtf8().constData());
        return;
    }

    d->globPatterns.push_back(wildCard);
    if (m_suffixPattern.exactMatch(pattern)) {
        const QString suffix = pattern.right(pattern.size() - 2);
        d->suffixes.push_back(suffix);
        if (d->preferredSuffix.isEmpty())
            d->preferredSuffix = suffix;
    }
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
        *errorMessage = QCoreApplication::translate("MimeDatabase", "Not a number '%1'.").arg(n);
        return false;
    }
    return true;
}

// Evaluate a magic match rule like
//  <match value="must be converted with BinHex" type="string" offset="11"/>
//  <match value="0x9501" type="big16" offset="0:64"/>
static bool addMagicMatchRule(const QXmlStreamAttributes &atts,
                              MagicRuleMatcher *ruleMatcher,
                              QString *errorMessage)
{
    const QString type = atts.value(QLatin1String(matchTypeAttributeC)).toString();
    if (type != QLatin1String(matchStringTypeValueC)) {
        qWarning("%s: match type %s is not supported.", Q_FUNC_INFO, type.toUtf8().constData());
        return true;
    }
    const QString value = atts.value(QLatin1String(matchValueAttributeC)).toString();
    if (value.isEmpty()) {
        *errorMessage = QCoreApplication::translate("MimeDatabase", "Empty match value detected.");
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
    ruleMatcher->add(QSharedPointer<MagicRule>(MagicRule::createStringRule(value, startPos, endPos)));
    return true;
}

bool BaseMimeTypeParser::parse(QIODevice *dev, const QString &fileName, QString *errorMessage)
{
    MimeTypeData data;
    MagicRuleMatcher *ruleMatcher = 0;

    QXmlStreamReader reader(dev);
    ParseStage ps = ParseBeginning;
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement:
            ps = nextStage(ps, reader.name());
            switch (ps) {
            case ParseMimeType: { // start parsing a type
                const QString type = reader.attributes().value(QLatin1String(mimeTypeAttributeC)).toString();
                if (type.isEmpty()) {
                    reader.raiseError(QCoreApplication::translate("MimeDatabase", "Missing 'type'-attribute"));
                } else {
                    data.type = type;
                }
            }
                break;
            case ParseGlobPattern:
                addGlobPattern(reader.attributes().value(QLatin1String(patternAttributeC)).toString(), &data);
                break;
            case ParseSubClass: {
                const QString inheritsFrom = reader.attributes().value(QLatin1String(mimeTypeAttributeC)).toString();
                if (!inheritsFrom.isEmpty())
                    data.subClassesOf.push_back(inheritsFrom);
            }
                break;
            case ParseComment: {
                // comments have locale attributes. We want the default, English one
                QString locale = reader.attributes().value(QLatin1String(localeAttributeC)).toString();
                const QString comment = reader.readElementText();
                if (locale.isEmpty()) {
                    data.comment = comment;
                } else {
                    data.localeComments.insert(locale, comment);
                }
            }
                break;
            case ParseAlias: {
                const QString alias = reader.attributes().value(QLatin1String(mimeTypeAttributeC)).toString();
                if (!alias.isEmpty())
                    data.aliases.push_back(alias);
            }
                break;
            case ParseMagic: {
                int priority = 0;
                const QString priorityS = reader.attributes().value(QLatin1String(priorityAttributeC)).toString();
                if (!priorityS.isEmpty()) {
                    if (!parseNumber(priorityS, &priority, errorMessage))
                        return false;

                }
                ruleMatcher = new MagicRuleMatcher;
                ruleMatcher->setPriority(priority);
            }
                break;
            case ParseMagicMatchRule:
                if (!addMagicMatchRule(reader.attributes(), ruleMatcher, errorMessage))
                    return false;
                break;
            case ParseError:
                reader.raiseError(QCoreApplication::translate("MimeDatabase", "Unexpected element <%1>").arg(reader.name().toString()));
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
                if (reader.name() == QLatin1String(QLatin1String(magicTagC))) {
                    data.magicMatchers.push_back(QSharedPointer<IMagicMatcher>(ruleMatcher));
                    ruleMatcher = 0;
                }
            }
            break;

        default:
            break;
        } // switch reader.readNext()
    }

    if (reader.hasError()) {
        *errorMessage = QCoreApplication::translate("MimeDatabase", "An error has been encountered at line %1 of %2: %3:").arg(reader.lineNumber()).arg(fileName, reader.errorString());
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

/* MimeDatabasePrivate: Requirements for storage:
 * - Must be robust in case of incomplete hierachies, dangling entries
 * - Plugins will not load and register their mime types in order
 *   of inheritance.
 * - Multiple inheritance (several subClassesOf) can occur
 * - Provide quick lookup by name
 * - Provide quick lookup by file type.
 * This basically rules out some pointer-based tree, so the structure choosen
 * is:
 * - An alias map <QString->QString> for mapping aliases to types
 * - A Map <QString-MimeMapEntry> for the types (MimeMapEntry being a pair of
 *   MimeType and (hierarchy) level.
 * - A map  <QString->QString> representing parent->child relations (enabling
 *   recursing over children)
 * Using strings avoids dangling pointers.
 * The hierarchy level is used for mapping by file types. When findByFile()
 * is first called after addMimeType() it recurses over the hierarchy and sets
 * the hierarchy level of the entries accordingly (0 toplevel, 1 first
 * order...). It then does several passes over the type map, checking the
 * globs for maxLevel, maxLevel-1....until it finds a match (idea being to
 * to check the most specific types first). Starting a recursion from the
 * leaves is not suitable since it will hit parent nodes several times. */

class MimeDatabasePrivate
{
    Q_DISABLE_COPY(MimeDatabasePrivate)
public:
    MimeDatabasePrivate();

    bool addMimeTypes(const QString &fileName, QString *errorMessage);
    bool addMimeTypes(QIODevice *device, QString *errorMessage);
    bool addMimeType(MimeType mt);

    // Returns a mime type or Null one if none found
    MimeType findByType(const QString &type) const;
    // Returns a mime type or Null one if none found
    MimeType findByFile(const QFileInfo &f) const;

    bool setPreferredSuffix(const QString &typeOrAlias, const QString &suffix);

    // Return all known suffixes
    QStringList suffixes() const;
    QStringList filterStrings() const;

    void debug(QTextStream &str) const;

private:
    typedef QHash<QString, MimeMapEntry> TypeMimeTypeMap;
    typedef QHash<QString, QString> AliasMap;
    typedef QMultiHash<QString, QString> ParentChildrenMap;

    bool addMimeTypes(QIODevice *device, const QString &fileName, QString *errorMessage);
    inline const QString &resolveAlias(const QString &name) const;
    MimeType findByFile(const QFileInfo &f, unsigned *priority) const;
    void determineLevels();
    void raiseLevelRecursion(MimeMapEntry &e, int level);

    TypeMimeTypeMap m_typeMimeTypeMap;
    AliasMap m_aliasMap;
    ParentChildrenMap m_parentChildrenMap;
    int m_maxLevel;
};

MimeDatabasePrivate::MimeDatabasePrivate() :
    m_maxLevel(-1)
{
}

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
        *errorMessage = QCoreApplication::translate("MimeDatabase", "Cannot open %1: %2").arg(fileName, file.errorString());
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
    // Register the children, resolved via alias map. Note that it is still
    // possible that aliases end up in the map if the parent classes are not inserted
    // at this point (thus their aliases not known).
    const QStringList subClassesOf = mt.subClassesOf();
    if (!subClassesOf.empty()) {
        const QStringList::const_iterator socend = subClassesOf.constEnd();
        for (QStringList::const_iterator soit = subClassesOf.constBegin(); soit !=  socend; ++soit)
            m_parentChildrenMap.insert(resolveAlias(*soit), type);
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
    const QStringList childTypes = m_parentChildrenMap.values(e.type.type());
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
        qDebug() << '>' << Q_FUNC_INFO << f.fileName();
    const MimeType rc = findByFile(f, &priority);
    if (debugMimeDB) {
        if (rc) {
            qDebug() << "<MimeDatabase::findByFile: match prio=" << priority << rc.type();
        } else {
            qDebug() << "<MimeDatabase::findByFile: no match";
        }
    }
    return rc;
}

// Returns a mime type or Null one if none found
MimeType MimeDatabasePrivate::findByFile(const QFileInfo &f, unsigned *priorityPtr) const
{
    typedef QList<MimeMapEntry> MimeMapEntryList;

    // Is the hierarchy set up in case we find several matches?
    if (m_maxLevel < 0) {
        MimeDatabasePrivate *db = const_cast<MimeDatabasePrivate *>(this);
        db->determineLevels();
    }
    // Starting from max level (most specific): Try to find a match of
    // best (max) priority. Return if a glob match triggers.
    *priorityPtr = 0;
    unsigned maxPriority = 0;
    MimeType rc;
    Internal::FileMatchContext context(f);
    const TypeMimeTypeMap::const_iterator cend = m_typeMimeTypeMap.constEnd();
    for (int level = m_maxLevel; level >= 0; level--)
        for (TypeMimeTypeMap::const_iterator it = m_typeMimeTypeMap.constBegin(); it != cend; ++it)
            if (it.value().level == level) {
                const unsigned priority = it.value().type.matchesFile(context);
                if (debugMimeDB > 1)
                    qDebug() <<  "pass" << level << it.value().type.type() << " matches " << priority;
                if (priority)
                    if (priority > maxPriority) {
                        rc = it.value().type;
                        maxPriority = priority;
                        // Glob (exact) match?! We are done
                        if (maxPriority == MimeType::GlobMatchPriority) {
                            *priorityPtr = priority;
                            return rc;
                        }
                    }
            }
    return rc;
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
    for (TypeMimeTypeMap::const_iterator it = m_typeMimeTypeMap.constBegin(); it != cend; ++it)
        rc += it.value().type.filterString();
    return rc;
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

// --------------- MimeDatabase
MimeDatabase::MimeDatabase() :
    m_d(new MimeDatabasePrivate)
{
}

MimeDatabase::~MimeDatabase()
{
    delete m_d;
}

MimeType MimeDatabase::findByType(const QString &typeOrAlias) const
{
    return m_d->findByType(typeOrAlias);
}

MimeType MimeDatabase::findByFile(const QFileInfo &f) const
{
    return m_d->findByFile(f);
}

bool MimeDatabase::addMimeType(const  MimeType &mt)
{
    return m_d->addMimeType(mt);
}

bool MimeDatabase::addMimeTypes(const QString &fileName, QString *errorMessage)
{
    return m_d->addMimeTypes(fileName, errorMessage);
}

bool MimeDatabase::addMimeTypes(QIODevice *device, QString *errorMessage)
{
    return m_d->addMimeTypes(device, errorMessage);
}

QStringList MimeDatabase::suffixes() const
{
    return m_d->suffixes();
}

QStringList MimeDatabase::filterStrings() const
{
    return m_d->filterStrings();
}

QString MimeDatabase::preferredSuffixByType(const QString &type) const
{
    if (const MimeType mt = findByType(type))
        return mt.preferredSuffix();
    return QString();
}

QString MimeDatabase::preferredSuffixByFile(const QFileInfo &f) const
{
    if (const MimeType mt = findByFile(f))
        return mt.preferredSuffix();
    return QString();
}

bool MimeDatabase::setPreferredSuffix(const QString &typeOrAlias, const QString &suffix)
{
    return m_d->setPreferredSuffix(typeOrAlias, suffix);
}

QDebug operator<<(QDebug d, const MimeDatabase &mt)
{
    QString s;
    {
        QTextStream str(&s);
        mt.m_d->debug(str);
    }
    d << s;
    return d;
}

} // namespace Core

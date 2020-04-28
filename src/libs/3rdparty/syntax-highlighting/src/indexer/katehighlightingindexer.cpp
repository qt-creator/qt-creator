/*
    Copyright (C) 2014 Christoph Cullmann <cullmann@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QVariant>
#include <QXmlStreamReader>

#ifdef QT_XMLPATTERNS_LIB
#include <QXmlSchema>
#include <QXmlSchemaValidator>
#endif

namespace
{
QStringList readListing(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return QStringList();
    }

    QXmlStreamReader xml(&file);
    QStringList listing;
    while (!xml.atEnd()) {
        xml.readNext();

        // add only .xml files, no .json or stuff
        if (xml.isCharacters() && xml.text().toString().contains(QLatin1String(".xml"))) {
            listing.append(xml.text().toString());
        }
    }

    if (xml.hasError()) {
        qWarning() << "XML error while reading" << fileName << " - " << qPrintable(xml.errorString()) << "@ offset" << xml.characterOffset();
        listing.clear();
    }

    return listing;
}

/**
 * check if the "extensions" attribute have valid wildcards
 * @param extensions extensions string to check
 * @return valid?
 */
bool checkExtensions(const QString &extensions)
{
    // get list of extensions
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    const QStringList extensionParts = extensions.split(QLatin1Char(';'), QString::SkipEmptyParts);
#else
    const QStringList extensionParts = extensions.split(QLatin1Char(';'), Qt::SkipEmptyParts);
#endif

    // ok if empty
    if (extensionParts.isEmpty()) {
        return true;
    }

    // check that only valid wildcard things are inside the parts
    for (const auto &extension : extensionParts) {
        for (const auto c : extension) {
            // eat normal things
            if (c.isDigit() || c.isLetter()) {
                continue;
            }

            // allow some special characters
            if (c == QLatin1Char('.') || c == QLatin1Char('-') || c == QLatin1Char('_') || c == QLatin1Char('+')) {
                continue;
            }

            // only allowed wildcard things: '?' and '*'
            if (c == QLatin1Char('?') || c == QLatin1Char('*')) {
                continue;
            }

            qWarning() << "invalid character" << c << " seen in extensions wildcard";
            return false;
        }
    }

    // all checks passed
    return true;
}

//! Check that a regular expression in a RegExpr rule:
//! - is not empty
//! - isValid()
//! - character ranges such as [A-Z] are valid and not accidentally e.g. [A-z].
//! - dynamic=true but no place holder used?
bool checkRegularExpression(const QString &hlFilename, QXmlStreamReader &xml)
{
    if (xml.name() == QLatin1String("RegExpr") || xml.name() == QLatin1String("emptyLine")) {
        // get right attribute
        const QString string(xml.attributes().value((xml.name() == QLatin1String("RegExpr")) ? QLatin1String("String") : QLatin1String("regexpr")).toString());

        // validate regexp
        const QRegularExpression regexp(string);
        if (!regexp.isValid()) {
            qWarning() << hlFilename << "line" << xml.lineNumber() << "broken regex:" << string << "problem:" << regexp.errorString() << "at offset" << regexp.patternErrorOffset();
            return false;
        } else if (string.isEmpty()) {
            qWarning() << hlFilename << "line" << xml.lineNumber() << "empty regex not allowed.";
            return false;
        }

        // catch possible case typos: [A-z] or [a-Z]
        const int azOffset = std::max(string.indexOf(QStringLiteral("A-z")), string.indexOf(QStringLiteral("a-Z")));
        if (azOffset >= 0) {
            qWarning() << hlFilename << "line" << xml.lineNumber() << "broken regex:" << string << "problem: [a-Z] or [A-z] at offset" << azOffset;
            return false;
        }

        // dynamic == true and no place holder?
        if (xml.name() == QLatin1String("RegExpr") && xml.attributes().value(QStringLiteral("dynamic")) == QStringLiteral("true")) {
            static const QRegularExpression placeHolder(QStringLiteral("%\\d+"));
            if (!string.contains(placeHolder)) {
                qWarning() << hlFilename << "line" << xml.lineNumber() << "broken regex:" << string << "problem: dynamic=true but no %\\d+ placeholder";
                return false;
            }
        }
    }

    return true;
}

//! Check that keyword list items do not have trailing or leading spaces,
//! e.g.: <item> keyword </item>
bool checkItemsTrimmed(const QString &hlFilename, QXmlStreamReader &xml)
{
    if (xml.name() == QLatin1String("item")) {
        const QString keyword = xml.readElementText();
        if (keyword != keyword.trimmed()) {
            qWarning() << hlFilename << "line" << xml.lineNumber() << "keyword with leading/trailing spaces:" << keyword;
            return false;
        }
    }

    return true;
}

//! Checks that DetectChar and Detect2Chars really only have one char
//! in the attributes 'char' and 'char1'.
bool checkSingleChars(const QString &hlFilename, QXmlStreamReader &xml)
{
    const bool testChar1 = xml.name() == QLatin1String("Detect2Chars");
    const bool testChar = testChar1 || xml.name() == QLatin1String("DetectChar");

    if (testChar) {
        const QString c = xml.attributes().value(QLatin1String("char")).toString();
        if (c.size() != 1) {
            qWarning() << hlFilename << "line" << xml.lineNumber() << "'char' must contain exactly one char:" << c;
            return false;
        }
    }

    if (testChar1) {
        const QString c = xml.attributes().value(QLatin1String("char1")).toString();
        if (c.size() != 1) {
            qWarning() << hlFilename << "line" << xml.lineNumber() << "'char1' must contain exactly one char:" << c;
            return false;
        }
    }

    return true;
}

//! Search for rules with lookAhead="true" and context="#stay".
//! This would cause an infinite loop.
bool checkLookAhead(const QString &hlFilename, QXmlStreamReader &xml)
{
    if (xml.attributes().hasAttribute(QStringLiteral("lookAhead"))) {
        auto lookAhead = xml.attributes().value(QStringLiteral("lookAhead"));
        if (lookAhead == QStringLiteral("true")) {
            auto context = xml.attributes().value(QStringLiteral("context"));
            if (context == QStringLiteral("#stay")) {
                qWarning() << hlFilename << "line" << xml.lineNumber() << "Infinite loop: lookAhead with context #stay";
                return false;
            }
        }
    }
    return true;
}

/**
 * Helper class to search for non-existing keyword include.
 */
class KeywordIncludeChecker
{
public:
    void processElement(const QString &hlFilename, const QString &hlName, QXmlStreamReader &xml)
    {
        if (xml.name() == QLatin1String("list")) {
            auto &keywords = m_keywordMap[hlName];
            keywords.filename = hlFilename;
            auto name = xml.attributes().value(QLatin1String("name")).toString();
            m_currentIncludes = &keywords.includes[name];
        } else if (xml.name() == QLatin1String("include")) {
            if (!m_currentIncludes) {
                qWarning() << hlFilename << "line" << xml.lineNumber() << "<include> tag ouside <list>";
                m_success = false;
            } else {
                m_currentIncludes->push_back({xml.lineNumber(), xml.readElementText()});
            }
        }
    }

    bool check() const
    {
        bool success = m_success;
        for (auto &keywords : m_keywordMap) {
            QMapIterator<QString, QVector<Keywords::Include>> includes(keywords.includes);
            while (includes.hasNext()) {
                includes.next();
                for (auto &include : includes.value()) {
                    bool containsKeywordName = true;
                    int const idx = include.name.indexOf(QStringLiteral("##"));
                    if (idx == -1) {
                        auto &keywordName = includes.key();
                        containsKeywordName = keywords.includes.contains(keywordName);
                    } else {
                        auto defName = include.name.mid(idx + 2);
                        auto listName = include.name.left(idx);
                        auto it = m_keywordMap.find(defName);
                        if (it == m_keywordMap.end()) {
                            qWarning() << keywords.filename << "line" << include.line << "unknown definition in" << include.name;
                            success = false;
                        } else {
                            containsKeywordName = it->includes.contains(listName);
                        }
                    }

                    if (!containsKeywordName) {
                        qWarning() << keywords.filename << "line" << include.line << "unknown keyword name in" << include.name;
                        success = false;
                    }
                }
            }
        }
        return success;
    }

private:
    struct Keywords {
        QString filename;
        struct Include {
            qint64 line;
            QString name;
        };
        QMap<QString, QVector<Include>> includes;
    };
    QHash<QString, Keywords> m_keywordMap;
    QVector<Keywords::Include> *m_currentIncludes = nullptr;
    bool m_success = true;
};

/**
 * Helper class to search for non-existing or unreferenced keyword lists.
 */
class KeywordChecker
{
public:
    KeywordChecker(const QString &filename)
        : m_filename(filename)
    {
    }

    void processElement(QXmlStreamReader &xml)
    {
        if (xml.name() == QLatin1String("list")) {
            const QString name = xml.attributes().value(QLatin1String("name")).toString();
            if (m_existingNames.contains(name)) {
                qWarning() << m_filename << "list duplicate:" << name;
                m_success = false;
            }
            m_existingNames.insert(name);
        } else if (xml.name() == QLatin1String("keyword")) {
            const QString context = xml.attributes().value(QLatin1String("String")).toString();
            if (!context.isEmpty())
                m_usedNames.insert(context);
        }
    }

    bool check() const
    {
        bool success = m_success;
        const auto invalidNames = m_usedNames - m_existingNames;
        if (!invalidNames.isEmpty()) {
            qWarning() << m_filename << "Reference of non-existing keyword list:" << invalidNames;
            success = false;
        }

        const auto unusedNames = m_existingNames - m_usedNames;
        if (!unusedNames.isEmpty()) {
            qWarning() << m_filename << "Unused keyword lists:" << unusedNames;
            success = false;
        }

        return success;
    }

private:
    QString m_filename;
    QSet<QString> m_usedNames;
    QSet<QString> m_existingNames;
    bool m_success = true;
};

/**
 * Helper class to search for non-existing contexts and invalid version
 */
class ContextChecker
{
public:
    void setKateVersion(const QStringRef &verStr, const QString &hlFilename, const QString &hlName)
    {
        const auto idx = verStr.indexOf(QLatin1Char('.'));
        if (idx <= 0) {
            qWarning() << hlFilename << "invalid kateversion" << verStr;
            m_success = false;
        } else {
            auto &language = m_contextMap[hlName];
            language.version = {verStr.left(idx).toInt(), verStr.mid(idx + 1).toInt()};
        }
    }

    void processElement(const QString &hlFilename, const QString &hlName, QXmlStreamReader &xml)
    {
        if (xml.name() == QLatin1String("context")) {
            auto &language = m_contextMap[hlName];
            language.hlFilename = hlFilename;
            const QString name = xml.attributes().value(QLatin1String("name")).toString();
            if (language.isFirstContext) {
                language.isFirstContext = false;
                language.usedContextNames.insert(name);
            }

            if (language.existingContextNames.contains(name)) {
                qWarning() << hlFilename << "Duplicate context:" << name;
                m_success = false;
            } else {
                language.existingContextNames.insert(name);
            }

            if (xml.attributes().value(QLatin1String("fallthroughContext")).toString() == QLatin1String("#stay")) {
                qWarning() << hlFilename << "possible infinite loop due to fallthroughContext=\"#stay\" in context " << name;
                m_success = false;
            }

            processContext(hlName, xml.attributes().value(QLatin1String("lineEndContext")).toString());
            processContext(hlName, xml.attributes().value(QLatin1String("lineEmptyContext")).toString());
            processContext(hlName, xml.attributes().value(QLatin1String("fallthroughContext")).toString());
        } else if (xml.name() == QLatin1String("include")) {
            // <include> tag inside <list>
            processVersion(hlFilename, hlName, xml, {5, 53}, QLatin1String("<include>"));
        } else {
            if (xml.attributes().hasAttribute(QLatin1String("context"))) {
                const QString context = xml.attributes().value(QLatin1String("context")).toString();
                if (context.isEmpty()) {
                    qWarning() << hlFilename << "Missing context name in line" << xml.lineNumber();
                    m_success = false;
                } else {
                    processContext(hlName, context);
                }
            }
        }
    }

    bool check() const
    {
        bool success = m_success;

        // recursive search for the required miximal version
        struct GetRequiredVersion {
            QHash<const Language *, Version> versionMap;

            Version operator()(const QHash<QString, Language> &contextMap, const Language &language)
            {
                auto &version = versionMap[&language];
                if (version < language.version) {
                    version = language.version;
                    for (auto &languageName : language.usedLanguageName) {
                        auto it = contextMap.find(languageName);
                        if (it != contextMap.end()) {
                            version = std::max(operator()(contextMap, *it), version);
                        }
                    }
                }
                return version;
            };
        };
        GetRequiredVersion getRequiredVersion;

        for (auto &language : m_contextMap) {
            const auto invalidContextNames = language.usedContextNames - language.existingContextNames;
            if (!invalidContextNames.isEmpty()) {
                qWarning() << language.hlFilename << "Reference of non-existing contexts:" << invalidContextNames;
                success = false;
            }

            const auto unusedNames = language.existingContextNames - language.usedContextNames;
            if (!unusedNames.isEmpty()) {
                qWarning() << language.hlFilename << "Unused contexts:" << unusedNames;
                success = false;
            }

            auto requiredVersion = getRequiredVersion(m_contextMap, language);
            if (language.version < requiredVersion) {
                qWarning().nospace() << language.hlFilename << " depends on a language in version " << requiredVersion.majorRevision << "." << requiredVersion.minorRevision << ". Please, increase kateversion.";
                success = false;
            }
        }

        return success;
    }

private:
    //! Extract the referenced context name and language.
    //! Some input / output examples are:
    //! - "#stay"         -> ""
    //! - "#pop"          -> ""
    //! - "Comment"       -> "Comment"
    //! - "#pop!Comment"  -> "Comment"
    //! - "##ISO C++"     -> ""
    //! - "Comment##ISO C++"-> "Comment" in ISO C++
    void processContext(const QString &language, QString context)
    {
        if (context.isEmpty())
            return;

        // filter out #stay and #pop
        static QRegularExpression stayPop(QStringLiteral("^(#stay|#pop)+"));
        context.remove(stayPop);

        // handle cross-language context references
        if (context.contains(QStringLiteral("##"))) {
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
            const QStringList list = context.split(QStringLiteral("##"), QString::SkipEmptyParts);
#else
            const QStringList list = context.split(QStringLiteral("##"), Qt::SkipEmptyParts);
#endif
            if (list.size() == 1) {
                // nothing to do, other language is included: e.g. ##Doxygen
            } else if (list.size() == 2) {
                // specific context of other language, e.g. Comment##ISO C++
                m_contextMap[list[1]].usedContextNames.insert(list[0]);
                m_contextMap[language].usedLanguageName.insert(list[1]);
            }
            return;
        }

        // handle #pop!context" (#pop was already removed above)
        if (context.startsWith(QLatin1Char('!')))
            context.remove(0, 1);

        if (!context.isEmpty())
            m_contextMap[language].usedContextNames.insert(context);
    }

private:
    struct Version {
        int majorRevision;
        int minorRevision;

        Version(int majorRevision = 0, int minorRevision = 0)
            : majorRevision(majorRevision)
            , minorRevision(minorRevision)
        {
        }

        bool operator<(const Version &version) const
        {
            return majorRevision < version.majorRevision || (majorRevision == version.majorRevision && minorRevision < version.minorRevision);
        }
    };

    void processVersion(const QString &hlFilename, const QString &hlName, QXmlStreamReader &xml, Version const &requiredVersion, QLatin1String item)
    {
        auto &language = m_contextMap[hlName];

        if (language.version < requiredVersion) {
            qWarning().nospace() << hlFilename << " " << item << " in line " << xml.lineNumber() << " is only available since version " << requiredVersion.majorRevision << "." << requiredVersion.minorRevision
                                 << ". Please, increase kateversion.";
            // update the version to cancel future warnings
            language.version = requiredVersion;
            m_success = false;
        }
    }

    class Language
    {
    public:
        // filename on disk or in Qt resource
        QString hlFilename;

        // Is true, if the first context in xml file is encountered, and
        // false in all other cases. This is required, since the first context
        // is typically not referenced explicitly. So we will simply add the
        // first context to the usedContextNames list.
        bool isFirstContext = true;

        // holds all contexts that were referenced
        QSet<QString> usedContextNames;

        // holds all existing context names
        QSet<QString> existingContextNames;

        // holds all existing language names
        QSet<QString> usedLanguageName;

        // kateversion language attribute
        Version version;
    };

    /**
     * "Language name" to "Language" map.
     * Example key: "Doxygen"
     */
    QHash<QString, Language> m_contextMap;
    bool m_success = true;
};

/**
 * Helper class to search for non-existing itemDatas.
 */
class AttributeChecker
{
public:
    AttributeChecker(const QString &filename)
        : m_filename(filename)
    {
    }

    void processElement(QXmlStreamReader &xml)
    {
        if (xml.name() == QLatin1String("itemData")) {
            const QString name = xml.attributes().value(QLatin1String("name")).toString();
            if (!name.isEmpty()) {
                if (m_existingAttributeNames.contains(name)) {
                    qWarning() << m_filename << "itemData duplicate:" << name;
                    m_success = false;
                } else {
                    m_existingAttributeNames.insert(name);
                }
            }
        } else if (xml.attributes().hasAttribute(QLatin1String("attribute"))) {
            const QString name = xml.attributes().value(QLatin1String("attribute")).toString();
            if (name.isEmpty()) {
                qWarning() << m_filename << "specified attribute is empty:" << xml.name();
                m_success = false;
            } else {
                m_usedAttributeNames.insert(name);
            }
        }
    }

    bool check() const
    {
        bool success = m_success;
        const auto invalidNames = m_usedAttributeNames - m_existingAttributeNames;
        if (!invalidNames.isEmpty()) {
            qWarning() << m_filename << "Reference of non-existing itemData attributes:" << invalidNames;
            success = false;
        }

        auto unusedNames = m_existingAttributeNames - m_usedAttributeNames;
        if (!unusedNames.isEmpty()) {
            qWarning() << m_filename << "Unused itemData:" << unusedNames;
            success = false;
        }

        return success;
    }

private:
    QString m_filename;
    QSet<QString> m_usedAttributeNames;
    QSet<QString> m_existingAttributeNames;
    bool m_success = true;
};

}

int main(int argc, char *argv[])
{
    // get app instance
    QCoreApplication app(argc, argv);

    // ensure enough arguments are passed
    if (app.arguments().size() < 3)
        return 1;

#ifdef QT_XMLPATTERNS_LIB
    // open schema
    QXmlSchema schema;
    if (!schema.load(QUrl::fromLocalFile(app.arguments().at(2))))
        return 2;
#endif

    const QString hlFilenamesListing = app.arguments().value(3);
    if (hlFilenamesListing.isEmpty()) {
        return 1;
    }

    QStringList hlFilenames = readListing(hlFilenamesListing);
    if (hlFilenames.isEmpty()) {
        qWarning("Failed to read %s", qPrintable(hlFilenamesListing));
        return 3;
    }

    // text attributes
    const QStringList textAttributes = QStringList() << QStringLiteral("name") << QStringLiteral("section") << QStringLiteral("mimetype") << QStringLiteral("extensions") << QStringLiteral("style") << QStringLiteral("author")
                                                     << QStringLiteral("license") << QStringLiteral("indenter");

    // index all given highlightings
    ContextChecker contextChecker;
    KeywordIncludeChecker keywordIncludeChecker;
    QVariantMap hls;
    int anyError = 0;
    for (const QString &hlFilename : qAsConst(hlFilenames)) {
        QFile hlFile(hlFilename);
        if (!hlFile.open(QIODevice::ReadOnly)) {
            qWarning("Failed to open %s", qPrintable(hlFilename));
            anyError = 3;
            continue;
        }

#ifdef QT_XMLPATTERNS_LIB
        // validate against schema
        QXmlSchemaValidator validator(schema);
        if (!validator.validate(&hlFile, QUrl::fromLocalFile(hlFile.fileName()))) {
            anyError = 4;
            continue;
        }
#endif

        // read the needed attributes from toplevel language tag
        hlFile.reset();
        QXmlStreamReader xml(&hlFile);
        if (xml.readNextStartElement()) {
            if (xml.name() != QLatin1String("language")) {
                anyError = 5;
                continue;
            }
        } else {
            anyError = 6;
            continue;
        }

        // map to store hl info
        QVariantMap hl;

        // transfer text attributes
        for (const QString &attribute : qAsConst(textAttributes)) {
            hl[attribute] = xml.attributes().value(attribute).toString();
        }

        // check if extensions have the right format
        if (!checkExtensions(hl[QStringLiteral("extensions")].toString())) {
            qWarning() << hlFilename << "'extensions' wildcards invalid:" << hl[QStringLiteral("extensions")].toString();
            anyError = 23;
        }

        // numerical attributes
        hl[QStringLiteral("version")] = xml.attributes().value(QLatin1String("version")).toInt();
        hl[QStringLiteral("priority")] = xml.attributes().value(QLatin1String("priority")).toInt();

        // add boolean one
        const QString hidden = xml.attributes().value(QLatin1String("hidden")).toString();
        hl[QStringLiteral("hidden")] = (hidden == QLatin1String("true") || hidden == QLatin1String("1"));

        // remember hl
        hls[QFileInfo(hlFile).fileName()] = hl;

        AttributeChecker attributeChecker(hlFilename);
        KeywordChecker keywordChecker(hlFilename);

        const QString hlName = hl[QStringLiteral("name")].toString();

        contextChecker.setKateVersion(xml.attributes().value(QStringLiteral("kateversion")), hlFilename, hlName);

        // scan for broken regex or keywords with spaces
        while (!xml.atEnd()) {
            xml.readNext();
            if (!xml.isStartElement()) {
                continue;
            }

            // search for used/existing contexts if applicable
            contextChecker.processElement(hlFilename, hlName, xml);

            // search for existing keyword includes
            keywordIncludeChecker.processElement(hlFilename, hlName, xml);

            // search for used/existing attributes if applicable
            attributeChecker.processElement(xml);

            // search for used/existing keyword lists if applicable
            keywordChecker.processElement(xml);

            // scan for bad regex
            if (!checkRegularExpression(hlFilename, xml)) {
                anyError = 7;
                continue;
            }

            // scan for bogus <item>     lala    </item> spaces
            if (!checkItemsTrimmed(hlFilename, xml)) {
                anyError = 8;
                continue;
            }

            // check single chars in DetectChar and Detect2Chars
            if (!checkSingleChars(hlFilename, xml)) {
                anyError = 8;
                continue;
            }

            // scan for lookAhead="true" with context="#stay"
            if (!checkLookAhead(hlFilename, xml)) {
                anyError = 7;
                continue;
            }
        }

        if (!attributeChecker.check()) {
            anyError = 7;
        }

        if (!keywordChecker.check()) {
            anyError = 7;
        }
    }

    if (!contextChecker.check())
        anyError = 7;

    if (!keywordIncludeChecker.check())
        anyError = 7;

    // bail out if any problem was seen
    if (anyError)
        return anyError;

    // create outfile, after all has worked!
    QFile outFile(app.arguments().at(1));
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return 9;

    // write out json
    outFile.write(QJsonDocument::fromVariant(QVariant(hls)).toBinaryData());

    // be done
    return 0;
}

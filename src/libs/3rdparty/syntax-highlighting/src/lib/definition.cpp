/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2018 Dominik Haumann <dhaumann@kde.org>
    SPDX-FileCopyrightText: 2018 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "definition.h"
#include "definition_p.h"
#include "definitionref_p.h"

#include "context_p.h"
#include "format.h"
#include "format_p.h"
#include "ksyntaxhighlighting_logging.h"
#include "ksyntaxhighlighting_version.h"
#include "repository.h"
#include "repository_p.h"
#include "rule_p.h"
#include "worddelimiters_p.h"
#include "xml_p.h"

#include <QCborMap>
#include <QCoreApplication>
#include <QFile>
#include <QHash>
#include <QStringList>
#include <QXmlStreamReader>

#include <algorithm>

using namespace KSyntaxHighlighting;

DefinitionData::DefinitionData()
    : wordDelimiters()
    , wordWrapDelimiters(wordDelimiters)
{
}

DefinitionData::~DefinitionData()
{
    qDeleteAll(contexts);
}

DefinitionData *DefinitionData::get(const Definition &def)
{
    return def.d.get();
}

Definition::Definition()
    : d(new DefinitionData)
{
}

Definition::Definition(const Definition &other)
    : d(other.d)
{
    d->q = *this;
}

Definition::Definition(std::shared_ptr<DefinitionData> &&dd)
    : d(std::move(dd))
{
}

Definition::~Definition()
{
}

Definition &Definition::operator=(const Definition &rhs)
{
    d = rhs.d;
    return *this;
}

bool Definition::operator==(const Definition &other) const
{
    return d->fileName == other.d->fileName;
}

bool Definition::operator!=(const Definition &other) const
{
    return d->fileName != other.d->fileName;
}

bool Definition::isValid() const
{
    return d->repo && !d->fileName.isEmpty() && !d->name.isEmpty();
}

QString Definition::filePath() const
{
    return d->fileName;
}

QString Definition::name() const
{
    return d->name;
}

QString Definition::translatedName() const
{
    return QCoreApplication::instance()->translate("Language", d->name.toUtf8().constData());
}

QString Definition::section() const
{
    return d->section;
}

QString Definition::translatedSection() const
{
    return QCoreApplication::instance()->translate("Language Section", d->section.toUtf8().constData());
}

QVector<QString> Definition::mimeTypes() const
{
    return d->mimetypes;
}

QVector<QString> Definition::extensions() const
{
    return d->extensions;
}

int Definition::version() const
{
    return d->version;
}

int Definition::priority() const
{
    return d->priority;
}

bool Definition::isHidden() const
{
    return d->hidden;
}

QString Definition::style() const
{
    return d->style;
}

QString Definition::indenter() const
{
    return d->indenter;
}

QString Definition::author() const
{
    return d->author;
}

QString Definition::license() const
{
    return d->license;
}

bool Definition::isWordDelimiter(QChar c) const
{
    d->load();
    return d->wordDelimiters.contains(c);
}

bool Definition::isWordWrapDelimiter(QChar c) const
{
    d->load();
    return d->wordWrapDelimiters.contains(c);
}

bool Definition::foldingEnabled() const
{
    d->load();
    if (d->hasFoldingRegions || indentationBasedFoldingEnabled()) {
        return true;
    }

    // check included definitions
    const auto defs = includedDefinitions();
    for (const auto &def : defs) {
        if (def.foldingEnabled()) {
            d->hasFoldingRegions = true;
            break;
        }
    }

    return d->hasFoldingRegions;
}

bool Definition::indentationBasedFoldingEnabled() const
{
    d->load();
    return d->indentationBasedFolding;
}

QStringList Definition::foldingIgnoreList() const
{
    d->load();
    return d->foldingIgnoreList;
}

QStringList Definition::keywordLists() const
{
    d->load(DefinitionData::OnlyKeywords(true));
    return d->keywordLists.keys();
}

QStringList Definition::keywordList(const QString &name) const
{
    d->load(DefinitionData::OnlyKeywords(true));
    const auto list = d->keywordList(name);
    return list ? list->keywords() : QStringList();
}

bool Definition::setKeywordList(const QString &name, const QStringList &content)
{
    d->load(DefinitionData::OnlyKeywords(true));
    KeywordList *list = d->keywordList(name);
    if (list) {
        list->setKeywordList(content);
        return true;
    } else
        return false;
}

QVector<Format> Definition::formats() const
{
    d->load();

    // sort formats so that the order matches the order of the itemDatas in the xml files.
    auto formatList = QVector<Format>::fromList(d->formats.values());
    std::sort(formatList.begin(), formatList.end(), [](const KSyntaxHighlighting::Format &lhs, const KSyntaxHighlighting::Format &rhs) {
        return lhs.id() < rhs.id();
    });

    return formatList;
}

QVector<Definition> Definition::includedDefinitions() const
{
    d->load();

    // init worklist and result used as guard with this definition
    QVector<Definition> queue{*this};
    QVector<Definition> definitions{*this};
    while (!queue.isEmpty()) {
        // Iterate all context rules to find associated Definitions. This will
        // automatically catch other Definitions referenced with IncludeRuldes or ContextSwitch.
        const auto definition = queue.takeLast();
        for (const auto &context : qAsConst(definition.d->contexts)) {
            // handle context switch attributes of this context itself
            for (const auto switchContext :
                 {context->lineEndContext().context(), context->lineEmptyContext().context(), context->fallthroughContext().context()}) {
                if (switchContext) {
                    if (!definitions.contains(switchContext->definition())) {
                        queue.push_back(switchContext->definition());
                        definitions.push_back(switchContext->definition());
                    }
                }
            }

            // handle the embedded rules
            for (const auto &rule : context->rules()) {
                // handle include rules like inclusion
                if (!definitions.contains(rule->definition())) {
                    queue.push_back(rule->definition());
                    definitions.push_back(rule->definition());
                }

                // handle context switch context inclusion
                if (auto switchContext = rule->context().context()) {
                    if (!definitions.contains(switchContext->definition())) {
                        queue.push_back(switchContext->definition());
                        definitions.push_back(switchContext->definition());
                    }
                }
            }
        }
    }

    // remove the 1st entry, since it is this Definition
    definitions.pop_front();

    return definitions;
}

QString Definition::singleLineCommentMarker() const
{
    d->load();
    return d->singleLineCommentMarker;
}

CommentPosition Definition::singleLineCommentPosition() const
{
    d->load();
    return d->singleLineCommentPosition;
}

QPair<QString, QString> Definition::multiLineCommentMarker() const
{
    d->load();
    return {d->multiLineCommentStartMarker, d->multiLineCommentEndMarker};
}

QVector<QPair<QChar, QString>> Definition::characterEncodings() const
{
    d->load();
    return d->characterEncodings;
}

Context *DefinitionData::initialContext() const
{
    Q_ASSERT(!contexts.isEmpty());
    return contexts.first();
}

Context *DefinitionData::contextByName(const QString &wantedName) const
{
    for (const auto context : contexts) {
        if (context->name() == wantedName)
            return context;
    }
    return nullptr;
}

KeywordList *DefinitionData::keywordList(const QString &wantedName)
{
    auto it = keywordLists.find(wantedName);
    return (it == keywordLists.end()) ? nullptr : &it.value();
}

Format DefinitionData::formatByName(const QString &wantedName) const
{
    const auto it = formats.constFind(wantedName);
    if (it != formats.constEnd())
        return it.value();

    return Format();
}

bool DefinitionData::isLoaded() const
{
    return !contexts.isEmpty();
}

bool DefinitionData::load(OnlyKeywords onlyKeywords)
{
    if (fileName.isEmpty())
        return false;

    if (isLoaded())
        return true;

    if (bool(onlyKeywords) && keywordIsLoaded)
        return true;

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
        return false;

    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        const auto token = reader.readNext();
        if (token != QXmlStreamReader::StartElement)
            continue;

        if (reader.name() == QLatin1String("highlighting")) {
            loadHighlighting(reader, onlyKeywords);
            if (bool(onlyKeywords)) {
                return true;
            }
        }

        else if (reader.name() == QLatin1String("general"))
            loadGeneral(reader);
    }

    for (auto it = keywordLists.begin(); it != keywordLists.end(); ++it) {
        it->setCaseSensitivity(caseSensitive);
    }

    for (const auto context : qAsConst(contexts)) {
        context->resolveContexts();
        context->resolveIncludes();
        context->resolveAttributeFormat();
    }

    for (const auto context : qAsConst(contexts)) {
        for (const auto &rule : context->rules()) {
            rule->resolvePostProcessing();
        }
    }

    return true;
}

void DefinitionData::clear()
{
    // keep only name and repo, so we can re-lookup to make references persist over repo reloads
    keywordLists.clear();
    qDeleteAll(contexts);
    contexts.clear();
    formats.clear();

    fileName.clear();
    section.clear();
    style.clear();
    indenter.clear();
    author.clear();
    license.clear();
    mimetypes.clear();
    extensions.clear();
    wordDelimiters = WordDelimiters();
    wordWrapDelimiters = wordDelimiters;
    caseSensitive = Qt::CaseSensitive;
    version = 0.0f;
    priority = 0;
    hidden = false;
}

bool DefinitionData::loadMetaData(const QString &definitionFileName)
{
    fileName = definitionFileName;

    QFile file(definitionFileName);
    if (!file.open(QFile::ReadOnly))
        return false;

    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        const auto token = reader.readNext();
        if (token != QXmlStreamReader::StartElement)
            continue;
        if (reader.name() == QLatin1String("language")) {
            return loadLanguage(reader);
        }
    }

    return false;
}

bool DefinitionData::loadMetaData(const QString &file, const QCborMap &obj)
{
    name = obj.value(QLatin1String("name")).toString();
    section = obj.value(QLatin1String("section")).toString();
    version = obj.value(QLatin1String("version")).toInteger();
    priority = obj.value(QLatin1String("priority")).toInteger();
    style = obj.value(QLatin1String("style")).toString();
    author = obj.value(QLatin1String("author")).toString();
    license = obj.value(QLatin1String("license")).toString();
    indenter = obj.value(QLatin1String("indenter")).toString();
    hidden = obj.value(QLatin1String("hidden")).toBool();
    fileName = file;

    const auto exts = obj.value(QLatin1String("extensions")).toString();
    for (const auto &ext : exts.split(QLatin1Char(';'), Qt::SkipEmptyParts))
        extensions.push_back(ext);
    const auto mts = obj.value(QLatin1String("mimetype")).toString();
    for (const auto &mt : mts.split(QLatin1Char(';'), Qt::SkipEmptyParts))
        mimetypes.push_back(mt);

    return true;
}

bool DefinitionData::loadLanguage(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("language"));
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);

    if (!checkKateVersion(reader.attributes().value(QLatin1String("kateversion"))))
        return false;

    name = reader.attributes().value(QLatin1String("name")).toString();
    section = reader.attributes().value(QLatin1String("section")).toString();
    // toFloat instead of toInt for backward compatibility with old Kate files
    version = reader.attributes().value(QLatin1String("version")).toFloat();
    priority = reader.attributes().value(QLatin1String("priority")).toInt();
    hidden = Xml::attrToBool(reader.attributes().value(QLatin1String("hidden")));
    style = reader.attributes().value(QLatin1String("style")).toString();
    indenter = reader.attributes().value(QLatin1String("indenter")).toString();
    author = reader.attributes().value(QLatin1String("author")).toString();
    license = reader.attributes().value(QLatin1String("license")).toString();
    const auto exts = reader.attributes().value(QLatin1String("extensions")).toString();
    for (const auto &ext : exts.split(QLatin1Char(';'), Qt::SkipEmptyParts))
        extensions.push_back(ext);
    const auto mts = reader.attributes().value(QLatin1String("mimetype")).toString();
    for (const auto &mt : mts.split(QLatin1Char(';'), Qt::SkipEmptyParts))
        mimetypes.push_back(mt);
    if (reader.attributes().hasAttribute(QLatin1String("casesensitive")))
        caseSensitive = Xml::attrToBool(reader.attributes().value(QLatin1String("casesensitive"))) ? Qt::CaseSensitive : Qt::CaseInsensitive;
    return true;
}

void DefinitionData::loadHighlighting(QXmlStreamReader &reader, OnlyKeywords onlyKeywords)
{
    Q_ASSERT(reader.name() == QLatin1String("highlighting"));
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);

    // skip highlighting
    reader.readNext();

    while (!reader.atEnd()) {
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (reader.name() == QLatin1String("list")) {
                if (!keywordIsLoaded) {
                    KeywordList keywords;
                    keywords.load(reader);
                    keywordLists.insert(keywords.name(), keywords);
                } else {
                    reader.skipCurrentElement();
                    reader.readNext(); // Skip </list>
                }
            } else if (bool(onlyKeywords)) {
                resolveIncludeKeywords();
                return;
            } else if (reader.name() == QLatin1String("contexts")) {
                resolveIncludeKeywords();
                loadContexts(reader);
                reader.readNext();
            } else if (reader.name() == QLatin1String("itemDatas")) {
                loadItemData(reader);
            } else {
                reader.readNext();
            }
            break;
        case QXmlStreamReader::EndElement:
            return;
        default:
            reader.readNext();
            break;
        }
    }
}

void DefinitionData::resolveIncludeKeywords()
{
    if (keywordIsLoaded) {
        return;
    }

    keywordIsLoaded = true;

    for (auto it = keywordLists.begin(); it != keywordLists.end(); ++it) {
        it->resolveIncludeKeywords(*this);
    }
}

void DefinitionData::loadContexts(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("contexts"));
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);

    while (!reader.atEnd()) {
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (reader.name() == QLatin1String("context")) {
                auto context = new Context;
                context->setDefinition(q);
                context->load(reader);
                contexts.push_back(context);
            }
            reader.readNext();
            break;
        case QXmlStreamReader::EndElement:
            return;
        default:
            reader.readNext();
            break;
        }
    }
}

void DefinitionData::loadItemData(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("itemDatas"));
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);

    while (!reader.atEnd()) {
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (reader.name() == QLatin1String("itemData")) {
                Format f;
                auto formatData = FormatPrivate::detachAndGet(f);
                formatData->definition = q;
                formatData->load(reader);
                formatData->id = RepositoryPrivate::get(repo)->nextFormatId();
                formats.insert(f.name(), f);
                reader.readNext();
            }
            reader.readNext();
            break;
        case QXmlStreamReader::EndElement:
            return;
        default:
            reader.readNext();
            break;
        }
    }
}

void DefinitionData::loadGeneral(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("general"));
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);
    reader.readNext();

    // reference counter to count XML child elements, to not return too early
    int elementRefCounter = 1;

    while (!reader.atEnd()) {
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            ++elementRefCounter;

            if (reader.name() == QLatin1String("keywords")) {
                if (reader.attributes().hasAttribute(QLatin1String("casesensitive")))
                    caseSensitive = Xml::attrToBool(reader.attributes().value(QLatin1String("casesensitive"))) ? Qt::CaseSensitive : Qt::CaseInsensitive;

                // adapt wordDelimiters
                wordDelimiters.append(reader.attributes().value(QLatin1String("additionalDeliminator")));
                wordDelimiters.remove(reader.attributes().value(QLatin1String("weakDeliminator")));

                // adapt WordWrapDelimiters
                auto wordWrapDeliminatorAttr = reader.attributes().value(
                    QLatin1String("wordWrapDeliminator"));
                if (wordWrapDeliminatorAttr.isEmpty())
                    wordWrapDelimiters = wordDelimiters;
                else {
                    wordWrapDelimiters.append(wordWrapDeliminatorAttr);
                }
            } else if (reader.name() == QLatin1String("folding")) {
                if (reader.attributes().hasAttribute(QLatin1String("indentationsensitive")))
                    indentationBasedFolding = Xml::attrToBool(reader.attributes().value(QLatin1String("indentationsensitive")));
            } else if (reader.name() == QLatin1String("emptyLines")) {
                loadFoldingIgnoreList(reader);
            } else if (reader.name() == QLatin1String("comments")) {
                loadComments(reader);
            } else if (reader.name() == QLatin1String("spellchecking")) {
                loadSpellchecking(reader);
            } else {
                reader.skipCurrentElement();
            }
            reader.readNext();
            break;
        case QXmlStreamReader::EndElement:
            --elementRefCounter;
            if (elementRefCounter == 0)
                return;
            reader.readNext();
            break;
        default:
            reader.readNext();
            break;
        }
    }
}

void DefinitionData::loadComments(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("comments"));
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);
    reader.readNext();

    // reference counter to count XML child elements, to not return too early
    int elementRefCounter = 1;

    while (!reader.atEnd()) {
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            ++elementRefCounter;
            if (reader.name() == QLatin1String("comment")) {
                const bool isSingleLine = reader.attributes().value(QLatin1String("name")) == QLatin1String("singleLine");
                if (isSingleLine) {
                    singleLineCommentMarker = reader.attributes().value(QLatin1String("start")).toString();
                    const bool afterWhiteSpace = reader.attributes().value(QLatin1String("position")).toString() == QLatin1String("afterwhitespace");
                    singleLineCommentPosition = afterWhiteSpace ? CommentPosition::AfterWhitespace : CommentPosition::StartOfLine;
                } else {
                    multiLineCommentStartMarker = reader.attributes().value(QLatin1String("start")).toString();
                    multiLineCommentEndMarker = reader.attributes().value(QLatin1String("end")).toString();
                }
            }
            reader.readNext();
            break;
        case QXmlStreamReader::EndElement:
            --elementRefCounter;
            if (elementRefCounter == 0)
                return;
            reader.readNext();
            break;
        default:
            reader.readNext();
            break;
        }
    }
}

void DefinitionData::loadFoldingIgnoreList(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("emptyLines"));
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);
    reader.readNext();

    // reference counter to count XML child elements, to not return too early
    int elementRefCounter = 1;

    while (!reader.atEnd()) {
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            ++elementRefCounter;
            if (reader.name() == QLatin1String("emptyLine")) {
                foldingIgnoreList << reader.attributes().value(QLatin1String("regexpr")).toString();
            }
            reader.readNext();
            break;
        case QXmlStreamReader::EndElement:
            --elementRefCounter;
            if (elementRefCounter == 0)
                return;
            reader.readNext();
            break;
        default:
            reader.readNext();
            break;
        }
    }
}

void DefinitionData::loadSpellchecking(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("spellchecking"));
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);
    reader.readNext();

    // reference counter to count XML child elements, to not return too early
    int elementRefCounter = 1;

    while (!reader.atEnd()) {
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            ++elementRefCounter;
            if (reader.name() == QLatin1String("encoding")) {
                const auto charRef = reader.attributes().value(QLatin1String("char"));
                if (!charRef.isEmpty()) {
                    const auto str = reader.attributes().value(QLatin1String("string")).toString();
                    characterEncodings.push_back({charRef[0], str});
                }
            }
            reader.readNext();
            break;
        case QXmlStreamReader::EndElement:
            --elementRefCounter;
            if (elementRefCounter == 0)
                return;
            reader.readNext();
            break;
        default:
            reader.readNext();
            break;
        }
    }
}

bool DefinitionData::checkKateVersion(QStringView verStr)
{
    const auto idx = verStr.indexOf(QLatin1Char('.'));
    if (idx <= 0) {
        qCWarning(Log) << "Skipping" << fileName << "due to having no valid kateversion attribute:" << verStr;
        return false;
    }
    const auto major = verStr.left(idx).toString().toInt();
    const auto minor = verStr.mid(idx + 1).toString().toInt();

    if (major > SyntaxHighlighting_VERSION_MAJOR || (major == SyntaxHighlighting_VERSION_MAJOR && minor > SyntaxHighlighting_VERSION_MINOR)) {
        qCWarning(Log) << "Skipping" << fileName << "due to being too new, version:" << verStr;
        return false;
    }

    return true;
}

quint16 DefinitionData::foldingRegionId(const QString &foldName)
{
    hasFoldingRegions = true;
    return RepositoryPrivate::get(repo)->foldingRegionId(name, foldName);
}

DefinitionRef::DefinitionRef()
{
}

DefinitionRef::DefinitionRef(const Definition &def)
    : d(def.d)
{
}

DefinitionRef::~DefinitionRef()
{
}

DefinitionRef &DefinitionRef::operator=(const Definition &def)
{
    d = def.d;
    return *this;
}

Definition DefinitionRef::definition() const
{
    if (!d.expired())
        return Definition(d.lock());
    return Definition();
}

bool DefinitionRef::operator==(const DefinitionRef &other) const
{
    if (d.expired() != other.d.expired()) {
        return false;
    }

    return d.expired() || d.lock().get() == other.d.lock().get();
}

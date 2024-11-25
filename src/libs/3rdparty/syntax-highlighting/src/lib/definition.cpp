/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2018 Dominik Haumann <dhaumann@kde.org>
    SPDX-FileCopyrightText: 2018 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "definition.h"
#include "definition_p.h"

#include "context_p.h"
#include "format.h"
#include "format_p.h"
#include "highlightingdata_p.hpp"
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
#include <QStringTokenizer>
#include <QXmlStreamReader>

#include <algorithm>
#include <atomic>

using namespace KSyntaxHighlighting;

DefinitionData::DefinitionData()
    : wordDelimiters()
    , wordWrapDelimiters(wordDelimiters)
{
}

DefinitionData::~DefinitionData() = default;

Definition::Definition()
    : d(std::make_shared<DefinitionData>())
{
    d->q = d;
}

Definition::Definition(Definition &&other) noexcept = default;
Definition::Definition(const Definition &) = default;
Definition::~Definition() = default;
Definition &Definition::operator=(Definition &&other) noexcept = default;
Definition &Definition::operator=(const Definition &) = default;

Definition::Definition(const DefinitionData &defData)
    : d(defData.q.lock())
{
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

QStringList Definition::alternativeNames() const
{
    return d->alternativeNames;
}

QString Definition::translatedName() const
{
    if (d->translatedName.isEmpty()) {
        d->translatedName = QCoreApplication::instance()->translate("Language", d->nameUtf8.isEmpty() ? d->name.toUtf8().constData() : d->nameUtf8.constData());
    }
    return d->translatedName;
}

QString Definition::section() const
{
    return d->section;
}

QString Definition::translatedSection() const
{
    if (d->translatedSection.isEmpty()) {
        d->translatedSection = QCoreApplication::instance()->translate("Language Section",
                                                                       d->sectionUtf8.isEmpty() ? d->section.toUtf8().constData() : d->sectionUtf8.constData());
    }
    return d->translatedSection;
}

QList<QString> Definition::mimeTypes() const
{
    return d->mimetypes;
}

QList<QString> Definition::extensions() const
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
    if (d->foldingRegionsState == DefinitionData::FoldingRegionsState::Undetermined) {
        d->load();
    }

    return d->foldingRegionsState == DefinitionData::FoldingRegionsState::ContainsFoldingRegions || d->indentationBasedFolding;
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
    } else {
        return false;
    }
}

QList<Format> Definition::formats() const
{
    d->load();

    // sort formats so that the order matches the order of the itemDatas in the xml files.
    auto formatList = d->formats.values();
    std::sort(formatList.begin(), formatList.end(), [](const KSyntaxHighlighting::Format &lhs, const KSyntaxHighlighting::Format &rhs) {
        return lhs.id() < rhs.id();
    });

    return formatList;
}

QList<Definition> Definition::includedDefinitions() const
{
    QList<Definition> definitions;

    if (isValid()) {
        d->load();

        // init worklist and result used as guard with this definition
        QVarLengthArray<const DefinitionData *, 4> queue{d.get()};
        definitions.push_back(*this);
        while (!queue.empty()) {
            const auto *def = queue.back();
            queue.pop_back();
            for (const auto *defData : def->immediateIncludedDefinitions) {
                auto pred = [defData](const Definition &def) {
                    return DefinitionData::get(def) == defData;
                };
                if (std::find_if(definitions.begin(), definitions.end(), pred) == definitions.end()) {
                    definitions.push_back(Definition(*defData));
                    queue.push_back(defData);
                }
            }
        }

        // remove the 1st entry, since it is this Definition
        definitions.front() = std::move(definitions.back());
        definitions.pop_back();
    }

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

QList<QPair<QChar, QString>> Definition::characterEncodings() const
{
    d->load();
    return d->characterEncodings;
}

Context *DefinitionData::initialContext()
{
    Q_ASSERT(!contexts.empty());
    return &contexts.front();
}

Context *DefinitionData::contextByName(QStringView wantedName)
{
    for (auto &context : contexts) {
        if (context.name() == wantedName) {
            return &context;
        }
    }
    return nullptr;
}

KeywordList *DefinitionData::keywordList(const QString &wantedName)
{
    auto it = keywordLists.find(wantedName);
    return (it == keywordLists.end()) ? nullptr : &it.value();
}

Format DefinitionData::formatByName(QStringView wantedName) const
{
    const auto it = formats.constFind(wantedName);
    if (it != formats.constEnd()) {
        return it.value();
    }

    return Format();
}

bool DefinitionData::isLoaded() const
{
    return !contexts.empty();
}

namespace
{
std::atomic<uint64_t> definitionId{1};
}

bool DefinitionData::load(OnlyKeywords onlyKeywords)
{
    if (isLoaded()) {
        return true;
    }

    if (fileName.isEmpty()) {
        return false;
    }

    if (bool(onlyKeywords) && keywordIsLoaded) {
        return true;
    }

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        return false;
    }

    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        const auto token = reader.readNext();
        if (token != QXmlStreamReader::StartElement) {
            continue;
        }

        if (reader.name() == QLatin1String("highlighting")) {
            loadHighlighting(reader, onlyKeywords);
            if (bool(onlyKeywords)) {
                return true;
            }
        }

        else if (reader.name() == QLatin1String("general")) {
            loadGeneral(reader);
        }
    }

    for (auto &kw : keywordLists) {
        kw.setCaseSensitivity(caseSensitive);
    }

    resolveContexts();

    id = definitionId.fetch_add(1, std::memory_order_relaxed);

    return true;
}

void DefinitionData::clear()
{
    // keep only name and repo, so we can re-lookup to make references persist over repo reloads
    // see AbstractHighlighterPrivate::ensureDefinitionLoaded()
    id = 0;
    keywordLists.clear();
    contexts.clear();
    formats.clear();
    contextDatas.clear();
    immediateIncludedDefinitions.clear();
    wordDelimiters = WordDelimiters();
    wordWrapDelimiters = wordDelimiters;
    keywordIsLoaded = false;
    foldingRegionsState = FoldingRegionsState::Undetermined;
    indentationBasedFolding = false;
    foldingIgnoreList.clear();
    singleLineCommentMarker.clear();
    singleLineCommentPosition = CommentPosition::StartOfLine;
    multiLineCommentStartMarker.clear();
    multiLineCommentEndMarker.clear();
    characterEncodings.clear();

    fileName.clear();
    nameUtf8.clear();
    translatedName.clear();
    section.clear();
    sectionUtf8.clear();
    translatedSection.clear();
    style.clear();
    indenter.clear();
    author.clear();
    license.clear();
    mimetypes.clear();
    extensions.clear();
    caseSensitive = Qt::CaseSensitive;
    version = 0.0f;
    priority = 0;
    hidden = false;

    // purge our cache that is used to unify states
    unify.clear();
}

bool DefinitionData::loadMetaData(const QString &definitionFileName)
{
    fileName = definitionFileName;

    QFile file(definitionFileName);
    if (!file.open(QFile::ReadOnly)) {
        return false;
    }

    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        const auto token = reader.readNext();
        if (token != QXmlStreamReader::StartElement) {
            continue;
        }
        if (reader.name() == QLatin1String("language")) {
            return loadLanguage(reader);
        }
    }

    return false;
}

bool DefinitionData::loadMetaData(const QString &file, const QCborMap &obj)
{
    name = obj.value(QLatin1String("name")).toString();
    if (name.isEmpty()) {
        return false;
    }
    nameUtf8 = obj.value(QLatin1String("name")).toByteArray();
    section = obj.value(QLatin1String("section")).toString();
    sectionUtf8 = obj.value(QLatin1String("section")).toByteArray();
    version = obj.value(QLatin1String("version")).toInteger();
    priority = obj.value(QLatin1String("priority")).toInteger();
    style = obj.value(QLatin1String("style")).toString();
    author = obj.value(QLatin1String("author")).toString();
    license = obj.value(QLatin1String("license")).toString();
    indenter = obj.value(QLatin1String("indenter")).toString();
    hidden = obj.value(QLatin1String("hidden")).toBool();
    fileName = file;

    const auto names = obj.value(QLatin1String("alternativeNames")).toString();
    alternativeNames = names.split(QLatin1Char(';'), Qt::SkipEmptyParts);

    const auto exts = obj.value(QLatin1String("extensions")).toString();
    extensions = exts.split(QLatin1Char(';'), Qt::SkipEmptyParts);

    const auto mts = obj.value(QLatin1String("mimetype")).toString();
    mimetypes = mts.split(QLatin1Char(';'), Qt::SkipEmptyParts);

    return true;
}

bool DefinitionData::loadLanguage(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("language"));
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);

    if (!checkKateVersion(reader.attributes().value(QLatin1String("kateversion")))) {
        return false;
    }

    name = reader.attributes().value(QLatin1String("name")).toString();
    if (name.isEmpty()) {
        return false;
    }
    const auto names = reader.attributes().value(QLatin1String("alternativeNames"));
    for (const auto &n : QStringTokenizer(names, u';', Qt::SkipEmptyParts)) {
        alternativeNames.push_back(n.toString());
    }
    section = reader.attributes().value(QLatin1String("section")).toString();
    // toFloat instead of toInt for backward compatibility with old Kate files
    version = reader.attributes().value(QLatin1String("version")).toFloat();
    priority = reader.attributes().value(QLatin1String("priority")).toInt();
    hidden = Xml::attrToBool(reader.attributes().value(QLatin1String("hidden")));
    style = reader.attributes().value(QLatin1String("style")).toString();
    indenter = reader.attributes().value(QLatin1String("indenter")).toString();
    author = reader.attributes().value(QLatin1String("author")).toString();
    license = reader.attributes().value(QLatin1String("license")).toString();
    const auto exts = reader.attributes().value(QLatin1String("extensions"));
    for (const auto &ext : QStringTokenizer(exts, u';', Qt::SkipEmptyParts)) {
        extensions.push_back(ext.toString());
    }
    const auto mts = reader.attributes().value(QLatin1String("mimetype"));
    for (const auto &mt : QStringTokenizer(mts, u';', Qt::SkipEmptyParts)) {
        mimetypes.push_back(mt.toString());
    }
    if (reader.attributes().hasAttribute(QLatin1String("casesensitive"))) {
        caseSensitive = Xml::attrToBool(reader.attributes().value(QLatin1String("casesensitive"))) ? Qt::CaseSensitive : Qt::CaseInsensitive;
    }
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

    for (auto &kw : keywordLists) {
        kw.resolveIncludeKeywords(*this);
    }
}

void DefinitionData::loadContexts(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("contexts"));
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);

    contextDatas.reserve(32);

    while (!reader.atEnd()) {
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (reader.name() == QLatin1String("context")) {
                contextDatas.emplace_back().load(name, reader);
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

void DefinitionData::resolveContexts()
{
    contexts.reserve(contextDatas.size());

    /**
     * Transform all HighlightingContextData to Context.
     * This is necessary so that Context::resolveContexts() can find the referenced contexts.
     */
    for (const auto &contextData : std::as_const(contextDatas)) {
        contexts.emplace_back(*this, contextData);
    }

    /**
     * Resolves contexts and rules.
     */
    auto ctxIt = contexts.begin();
    for (const auto &contextData : std::as_const(contextDatas)) {
        ctxIt->resolveContexts(*this, contextData);
        ++ctxIt;
    }

    /**
     * To free the memory, constDatas is emptied because it is no longer used.
     */
    contextDatas.clear();
    contextDatas.shrink_to_fit();

    /**
     * Resolved includeRules.
     */
    for (auto &context : contexts) {
        context.resolveIncludes(*this);
    }

    // when a context includes a folding region this value is Yes, otherwise it remains undetermined
    if (foldingRegionsState == FoldingRegionsState::Undetermined) {
        foldingRegionsState = FoldingRegionsState::NoFoldingRegions;
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
                formatData->definitionName = name;
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
                if (reader.attributes().hasAttribute(QLatin1String("casesensitive"))) {
                    caseSensitive = Xml::attrToBool(reader.attributes().value(QLatin1String("casesensitive"))) ? Qt::CaseSensitive : Qt::CaseInsensitive;
                }

                // adapt wordDelimiters
                wordDelimiters.append(reader.attributes().value(QLatin1String("additionalDeliminator")));
                wordDelimiters.remove(reader.attributes().value(QLatin1String("weakDeliminator")));

                // adapt WordWrapDelimiters
                auto wordWrapDeliminatorAttr = reader.attributes().value(QLatin1String("wordWrapDeliminator"));
                if (wordWrapDeliminatorAttr.isEmpty()) {
                    wordWrapDelimiters = wordDelimiters;
                } else {
                    wordWrapDelimiters.append(wordWrapDeliminatorAttr);
                }
            } else if (reader.name() == QLatin1String("folding")) {
                if (reader.attributes().hasAttribute(QLatin1String("indentationsensitive"))) {
                    indentationBasedFolding = Xml::attrToBool(reader.attributes().value(QLatin1String("indentationsensitive")));
                }
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
            if (elementRefCounter == 0) {
                return;
            }
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
                    const bool afterWhiteSpace = reader.attributes().value(QLatin1String("position")) == QLatin1String("afterwhitespace");
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
            if (elementRefCounter == 0) {
                return;
            }
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
            if (elementRefCounter == 0) {
                return;
            }
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
                    const auto str = reader.attributes().value(QLatin1String("string"));
                    characterEncodings.push_back({charRef[0], str.toString()});
                }
            }
            reader.readNext();
            break;
        case QXmlStreamReader::EndElement:
            --elementRefCounter;
            if (elementRefCounter == 0) {
                return;
            }
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
    const auto major = verStr.sliced(0, idx).toInt();
    const auto minor = verStr.sliced(idx + 1).toInt();

    if (major > KSYNTAXHIGHLIGHTING_VERSION_MAJOR || (major == KSYNTAXHIGHLIGHTING_VERSION_MAJOR && minor > KSYNTAXHIGHLIGHTING_VERSION_MINOR)) {
        qCWarning(Log) << "Skipping" << fileName << "due to being too new, version:" << verStr;
        return false;
    }

    return true;
}

quint16 DefinitionData::foldingRegionId(const QString &foldName)
{
    foldingRegionsState = FoldingRegionsState::ContainsFoldingRegions;
    return RepositoryPrivate::get(repo)->foldingRegionId(name, foldName);
}

DefinitionData::ResolvedContext DefinitionData::resolveIncludedContext(QStringView defName, QStringView contextName)
{
    if (defName.isEmpty()) {
        return {this, contextByName(contextName)};
    }

    auto d = repo->definitionForName(defName.toString());
    if (d.isValid()) {
        auto *resolvedDef = get(d);
        if (resolvedDef != this) {
            if (std::find(immediateIncludedDefinitions.begin(), immediateIncludedDefinitions.end(), resolvedDef) == immediateIncludedDefinitions.end()) {
                immediateIncludedDefinitions.push_back(resolvedDef);
                resolvedDef->load();
                if (resolvedDef->foldingRegionsState == FoldingRegionsState::ContainsFoldingRegions) {
                    foldingRegionsState = FoldingRegionsState::ContainsFoldingRegions;
                }
            }
        }
        if (contextName.isEmpty()) {
            return {resolvedDef, resolvedDef->initialContext()};
        } else {
            return {resolvedDef, resolvedDef->contextByName(contextName)};
        }
    }

    return {nullptr, nullptr};
}

#include "moc_definition.cpp"

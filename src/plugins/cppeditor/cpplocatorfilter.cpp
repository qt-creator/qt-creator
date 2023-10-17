// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpplocatorfilter.h"

#include "cppeditorconstants.h"
#include "cppeditorplugin.h"
#include "cppeditortr.h"
#include "cpplocatordata.h"
#include "cppmodelmanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/fuzzymatcher.h>

#include <QRegularExpression>

using namespace Core;
using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor {

using EntryFromIndex = std::function<LocatorFilterEntry(const IndexItem::Ptr &)>;

void matchesFor(QPromise<void> &promise, const LocatorStorage &storage,
                IndexItem::ItemType wantedType, const EntryFromIndex &converter)
{
    const QString input = storage.input();
    LocatorFilterEntries entries[int(ILocatorFilter::MatchLevel::Count)];
    const Qt::CaseSensitivity caseSensitivityForPrefix = ILocatorFilter::caseSensitivity(input);
    const QRegularExpression regexp = ILocatorFilter::createRegExp(input);
    if (!regexp.isValid())
        return;

    const bool hasColonColon = input.contains("::");
    const QRegularExpression shortRegexp = hasColonColon
            ? ILocatorFilter::createRegExp(input.mid(input.lastIndexOf("::") + 2)) : regexp;
    CppLocatorData *locatorData = CppModelManager::locatorData();
    locatorData->filterAllFiles([&](const IndexItem::Ptr &info) {
        if (promise.isCanceled())
            return IndexItem::Break;
        const IndexItem::ItemType type = info->type();
        if (type & wantedType) {
            const QString symbolName = info->symbolName();
            QString matchString = hasColonColon ? info->scopedSymbolName() : symbolName;
            int matchOffset = hasColonColon ? matchString.size() - symbolName.size() : 0;
            QRegularExpressionMatch match = regexp.match(matchString);
            bool matchInParameterList = false;
            if (!match.hasMatch() && (type == IndexItem::Function)) {
                matchString += info->symbolType();
                match = regexp.match(matchString);
                matchInParameterList = true;
            }

            if (match.hasMatch()) {
                LocatorFilterEntry filterEntry = converter(info);

                // Highlight the matched characters, therefore it may be necessary
                // to update the match if the displayName is different from matchString
                if (QStringView(matchString).mid(matchOffset) != filterEntry.displayName) {
                    match = shortRegexp.match(filterEntry.displayName);
                    matchOffset = 0;
                }
                filterEntry.highlightInfo = ILocatorFilter::highlightInfo(match);
                if (matchInParameterList && filterEntry.highlightInfo.startsDisplay.isEmpty()) {
                    match = regexp.match(filterEntry.extraInfo);
                    filterEntry.highlightInfo = ILocatorFilter::highlightInfo(
                        match, LocatorFilterEntry::HighlightInfo::ExtraInfo);
                } else if (matchOffset > 0) {
                    for (int &start : filterEntry.highlightInfo.startsDisplay)
                        start -= matchOffset;
                }

                if (matchInParameterList)
                    entries[int(ILocatorFilter::MatchLevel::Normal)].append(filterEntry);
                else if (filterEntry.displayName.startsWith(input, caseSensitivityForPrefix))
                    entries[int(ILocatorFilter::MatchLevel::Best)].append(filterEntry);
                else if (filterEntry.displayName.contains(input, caseSensitivityForPrefix))
                    entries[int(ILocatorFilter::MatchLevel::Better)].append(filterEntry);
                else
                    entries[int(ILocatorFilter::MatchLevel::Good)].append(filterEntry);
            }
        }

        if (info->type() & IndexItem::Enum)
            return IndexItem::Continue;
        return IndexItem::Recurse;
    });

    for (auto &entry : entries) {
        if (entry.size() < 1000)
            Utils::sort(entry, LocatorFilterEntry::compareLexigraphically);
    }

    storage.reportOutput(std::accumulate(std::begin(entries), std::end(entries),
                                      LocatorFilterEntries()));
}

LocatorMatcherTask locatorMatcher(IndexItem::ItemType type, const EntryFromIndex &converter)
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;

    const auto onSetup = [=](Async<void> &async) {
        async.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        async.setConcurrentCallData(matchesFor, *storage, type, converter);
    };
    return {AsyncTask<void>(onSetup), storage};
}

LocatorMatcherTask allSymbolsMatcher()
{
    const auto converter = [](const IndexItem::Ptr &info) {
        LocatorFilterEntry filterEntry;
        filterEntry.displayName = info->symbolName();
        filterEntry.displayIcon = info->icon();
        filterEntry.linkForEditor = {info->filePath(), info->line(), info->column()};
        if (!info->symbolScope().isEmpty())
            filterEntry.extraInfo = info->symbolScope();
        else if (info->type() == IndexItem::Class || info->type() == IndexItem::Enum)
            filterEntry.extraInfo = info->shortNativeFilePath();
        else
            filterEntry.extraInfo = info->symbolType();
        return filterEntry;
    };
    return locatorMatcher(IndexItem::All, converter);
}

LocatorMatcherTask classMatcher()
{
    const auto converter = [](const IndexItem::Ptr &info) {
        LocatorFilterEntry filterEntry;
        filterEntry.displayName = info->symbolName();
        filterEntry.displayIcon = info->icon();
        filterEntry.linkForEditor = {info->filePath(), info->line(), info->column()};
        filterEntry.extraInfo = info->symbolScope().isEmpty()
                                    ? info->shortNativeFilePath()
                                    : info->symbolScope();
        filterEntry.filePath = info->filePath();
        return filterEntry;
    };
    return locatorMatcher(IndexItem::Class, converter);
}

LocatorMatcherTask functionMatcher()
{
    const auto converter = [](const IndexItem::Ptr &info) {
        QString name = info->symbolName();
        QString extraInfo = info->symbolScope();
        info->unqualifiedNameAndScope(name, &name, &extraInfo);
        if (extraInfo.isEmpty())
            extraInfo = info->shortNativeFilePath();
        else
            extraInfo.append(" (" + info->filePath().fileName() + ')');
        LocatorFilterEntry filterEntry;
        filterEntry.displayName = name + info->symbolType();
        filterEntry.displayIcon = info->icon();
        filterEntry.linkForEditor = {info->filePath(), info->line(), info->column()};
        filterEntry.extraInfo = extraInfo;

        return filterEntry;
    };
    return locatorMatcher(IndexItem::Function, converter);
}

QList<IndexItem::Ptr> itemsOfCurrentDocument(const FilePath &currentFileName)
{
    if (currentFileName.isEmpty())
        return {};

    QList<IndexItem::Ptr> results;
    const Snapshot snapshot = CppModelManager::snapshot();
    if (const Document::Ptr thisDocument = snapshot.document(currentFileName)) {
        SearchSymbols search;
        search.setSymbolsToSearchFor(SymbolSearcher::Declarations |
                                     SymbolSearcher::Enums |
                                     SymbolSearcher::Functions |
                                     SymbolSearcher::Classes);
        IndexItem::Ptr rootNode = search(thisDocument);
        rootNode->visitAllChildren([&](const IndexItem::Ptr &info) {
            results.append(info);
            return IndexItem::Recurse;
        });
    }
    return results;
}

LocatorFilterEntry::HighlightInfo highlightInfo(const QRegularExpressionMatch &match,
                                  LocatorFilterEntry::HighlightInfo::DataType dataType)
{
    const FuzzyMatcher::HighlightingPositions positions =
        FuzzyMatcher::highlightingPositions(match);

    return LocatorFilterEntry::HighlightInfo(positions.starts, positions.lengths, dataType);
}

void matchesForCurrentDocument(QPromise<void> &promise, const LocatorStorage &storage,
                               const FilePath &currentFileName)
{
    const QString input = storage.input();
    const QRegularExpression regexp = ILocatorFilter::createRegExp(input);
    if (!regexp.isValid())
        return;

    struct Entry
    {
        LocatorFilterEntry entry;
        IndexItem::Ptr info;
    };
    QList<Entry> goodEntries;
    QList<Entry> betterEntries;
    const QList<IndexItem::Ptr> items = itemsOfCurrentDocument(currentFileName);
    for (const IndexItem::Ptr &info : items) {
        if (promise.isCanceled())
            break;

        QString matchString = info->symbolName();
        if (info->type() == IndexItem::Declaration)
            matchString = info->representDeclaration();
        else if (info->type() == IndexItem::Function)
            matchString += info->symbolType();

        QRegularExpressionMatch match = regexp.match(matchString);
        if (match.hasMatch()) {
            const bool betterMatch = match.capturedStart() == 0;
            QString name = matchString;
            QString extraInfo = info->symbolScope();
            if (info->type() == IndexItem::Function) {
                if (info->unqualifiedNameAndScope(matchString, &name, &extraInfo)) {
                    name += info->symbolType();
                    match = regexp.match(name);
                }
            }

            LocatorFilterEntry filterEntry;
            filterEntry.displayName = name;
            filterEntry.displayIcon = info->icon();
            filterEntry.linkForEditor = {info->filePath(), info->line(), info->column()};
            filterEntry.extraInfo = extraInfo;
            if (match.hasMatch()) {
                filterEntry.highlightInfo = highlightInfo(match,
                            LocatorFilterEntry::HighlightInfo::DisplayName);
            } else {
                match = regexp.match(extraInfo);
                filterEntry.highlightInfo =
                    highlightInfo(match, LocatorFilterEntry::HighlightInfo::ExtraInfo);
            }

            if (betterMatch)
                betterEntries.append({filterEntry, info});
            else
                goodEntries.append({filterEntry, info});
        }
    }

    // entries are unsorted by design!
    betterEntries += goodEntries;

    QHash<QString, QList<Entry>> possibleDuplicates;
    for (const Entry &e : std::as_const(betterEntries))
        possibleDuplicates[e.info->scopedSymbolName() + e.info->symbolType()] << e;
    for (auto it = possibleDuplicates.cbegin(); it != possibleDuplicates.cend(); ++it) {
        const QList<Entry> &duplicates = it.value();
        if (duplicates.size() == 1)
            continue;
        QList<Entry> declarations;
        QList<Entry> definitions;
        for (const Entry &candidate : duplicates) {
            const IndexItem::Ptr info = candidate.info;
            if (info->type() != IndexItem::Function)
                break;
            if (info->isFunctionDefinition())
                definitions << candidate;
            else
                declarations << candidate;
        }
        if (definitions.size() == 1
            && declarations.size() + definitions.size() == duplicates.size()) {
            for (const Entry &decl : std::as_const(declarations)) {
                Utils::erase(betterEntries, [&decl](const Entry &e) {
                    return e.info == decl.info;
                });
            }
        }
    }
    storage.reportOutput(Utils::transform(betterEntries,
                                       [](const Entry &entry) { return entry.entry; }));
}

FilePath currentFileName()
{
    IEditor *currentEditor = EditorManager::currentEditor();
    return currentEditor ? currentEditor->document()->filePath() : FilePath();
}

LocatorMatcherTask currentDocumentMatcher()
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;

    const auto onSetup = [=](Async<void> &async) {
        async.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        async.setConcurrentCallData(matchesForCurrentDocument, *storage, currentFileName());
    };
    return {AsyncTask<void>(onSetup), storage};
}

using MatcherCreator = std::function<Core::LocatorMatcherTask()>;

static MatcherCreator creatorForType(MatcherType type)
{
    switch (type) {
    case MatcherType::AllSymbols: return &allSymbolsMatcher;
    case MatcherType::Classes: return &classMatcher;
    case MatcherType::Functions: return &functionMatcher;
    case MatcherType::CurrentDocumentSymbols: return &currentDocumentMatcher;
    }
    return {};
}

LocatorMatcherTasks cppMatchers(MatcherType type)
{
    const MatcherCreator creator = creatorForType(type);
    if (!creator)
        return {};
    return {creator()};
}

CppAllSymbolsFilter::CppAllSymbolsFilter()
{
    setId(Constants::LOCATOR_FILTER_ID);
    setDisplayName(Tr::tr(Constants::LOCATOR_FILTER_DISPLAY_NAME));
    setDescription(Tr::tr(Constants::LOCATOR_FILTER_DESCRIPTION));
    setDefaultShortcutString(":");
}

LocatorMatcherTasks CppAllSymbolsFilter::matchers()
{
    return {allSymbolsMatcher()};
}


CppClassesFilter::CppClassesFilter()
{
    setId(Constants::CLASSES_FILTER_ID);
    setDisplayName(Tr::tr(Constants::CLASSES_FILTER_DISPLAY_NAME));
    setDescription(Tr::tr(Constants::CLASSES_FILTER_DESCRIPTION));
    setDefaultShortcutString("c");
}

LocatorMatcherTasks CppClassesFilter::matchers()
{
    return {classMatcher()};
}

CppFunctionsFilter::CppFunctionsFilter()
{
    setId(Constants::FUNCTIONS_FILTER_ID);
    setDisplayName(Tr::tr(Constants::FUNCTIONS_FILTER_DISPLAY_NAME));
    setDescription(Tr::tr(Constants::FUNCTIONS_FILTER_DESCRIPTION));
    setDefaultShortcutString("m");
}

LocatorMatcherTasks CppFunctionsFilter::matchers()
{
    return {functionMatcher()};
}

CppCurrentDocumentFilter::CppCurrentDocumentFilter()
{
    setId(Constants::CURRENT_DOCUMENT_FILTER_ID);
    setDisplayName(Tr::tr(Constants::CURRENT_DOCUMENT_FILTER_DISPLAY_NAME));
    setDescription(Tr::tr(Constants::CURRENT_DOCUMENT_FILTER_DESCRIPTION));
    setDefaultShortcutString(".");
    setPriority(High);
}

LocatorMatcherTasks CppCurrentDocumentFilter::matchers()
{
    return {currentDocumentMatcher()};
}

} // namespace CppEditor

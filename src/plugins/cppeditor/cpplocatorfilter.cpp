// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpplocatorfilter.h"

#include "cppeditorconstants.h"
#include "cppeditorplugin.h"
#include "cppeditortr.h"
#include "cpplocatordata.h"

#include <utils/algorithm.h>
#include <utils/asynctask.h>

#include <QRegularExpression>

using namespace Core;
using namespace Utils;

namespace CppEditor {

using EntryFromIndex = std::function<LocatorFilterEntry(const IndexItem::Ptr &)>;

void matchesFor(QPromise<LocatorMatcherTask::OutputData> &promise, const QString &entry,
                IndexItem::ItemType wantedType, const EntryFromIndex &converter)
{
    QList<LocatorFilterEntry> entries[int(ILocatorFilter::MatchLevel::Count)];
    const Qt::CaseSensitivity caseSensitivityForPrefix = ILocatorFilter::caseSensitivity(entry);
    const QRegularExpression regexp = ILocatorFilter::createRegExp(entry);
    if (!regexp.isValid())
        return;

    const bool hasColonColon = entry.contains("::");
    const QRegularExpression shortRegexp = hasColonColon
            ? ILocatorFilter::createRegExp(entry.mid(entry.lastIndexOf("::") + 2)) : regexp;
    CppLocatorData *locatorData = CppModelManager::instance()->locatorData();
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
                else if (filterEntry.displayName.startsWith(entry, caseSensitivityForPrefix))
                    entries[int(ILocatorFilter::MatchLevel::Best)].append(filterEntry);
                else if (filterEntry.displayName.contains(entry, caseSensitivityForPrefix))
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

    promise.addResult(std::accumulate(std::begin(entries), std::end(entries),
                                      QList<LocatorFilterEntry>()));
}

LocatorMatcherTask locatorMatcher(IndexItem::ItemType type, const EntryFromIndex &converter)
{
    using namespace Tasking;

    TreeStorage<LocatorMatcherTask::Storage> storage;

    const auto onSetup = [=](AsyncTask<LocatorMatcherTask::OutputData> &async) {
        async.setFutureSynchronizer(Internal::CppEditorPlugin::futureSynchronizer());
        async.setConcurrentCallData(matchesFor, storage->input, type, converter);
    };
    const auto onDone = [storage](const AsyncTask<LocatorMatcherTask::OutputData> &async) {
        if (async.isResultAvailable())
            storage->output = async.result();
    };
    return {Async<LocatorMatcherTask::OutputData>(onSetup, onDone, onDone), storage};
}

LocatorMatcherTask cppAllSymbolsMatcher()
{
    const auto converter = [](const IndexItem::Ptr &info) {
        // TODO: Passing nullptr for filter -> accept won't work now. Replace with accept function.
        LocatorFilterEntry filterEntry(nullptr, info->scopedSymbolName());
        filterEntry.displayIcon = info->icon();
        filterEntry.linkForEditor = {info->filePath(), info->line(), info->column()};
        if (info->type() == IndexItem::Class || info->type() == IndexItem::Enum)
            filterEntry.extraInfo = info->shortNativeFilePath();
        else
            filterEntry.extraInfo = info->symbolType();
        return filterEntry;
    };
    return locatorMatcher(IndexItem::All, converter);
}

LocatorMatcherTask cppClassMatcher()
{
    const auto converter = [](const IndexItem::Ptr &info) {
        // TODO: Passing nullptr for filter -> accept won't work now. Replace with accept function.
        LocatorFilterEntry filterEntry(nullptr, info->symbolName());
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

LocatorMatcherTask cppFunctionMatcher()
{
    const auto converter = [](const IndexItem::Ptr &info) {
        QString name = info->symbolName();
        QString extraInfo = info->symbolScope();
        info->unqualifiedNameAndScope(name, &name, &extraInfo);
        if (extraInfo.isEmpty())
            extraInfo = info->shortNativeFilePath();
        else
            extraInfo.append(" (" + info->filePath().fileName() + ')');
        // TODO: Passing nullptr for filter -> accept won't work now. Replace with accept function.
        LocatorFilterEntry filterEntry(nullptr, name + info->symbolType());
        filterEntry.displayIcon = info->icon();
        filterEntry.linkForEditor = {info->filePath(), info->line(), info->column()};
        filterEntry.extraInfo = extraInfo;

        return filterEntry;
    };
    return locatorMatcher(IndexItem::Function, converter);
}

CppLocatorFilter::CppLocatorFilter()
{
    setId(Constants::LOCATOR_FILTER_ID);
    setDisplayName(Tr::tr(Constants::LOCATOR_FILTER_DISPLAY_NAME));
    setDescription(Tr::tr(Constants::LOCATOR_FILTER_DESCRIPTION));
    setDefaultShortcutString(":");
    setDefaultIncludedByDefault(false);
}

LocatorFilterEntry CppLocatorFilter::filterEntryFromIndexItem(IndexItem::Ptr info)
{
    LocatorFilterEntry filterEntry(this, info->scopedSymbolName());
    filterEntry.displayIcon = info->icon();
    filterEntry.linkForEditor = {info->filePath(), info->line(), info->column()};
    if (info->type() == IndexItem::Class || info->type() == IndexItem::Enum)
        filterEntry.extraInfo = info->shortNativeFilePath();
    else
        filterEntry.extraInfo = info->symbolType();

    return filterEntry;
}

QList<LocatorFilterEntry> CppLocatorFilter::matchesFor(
        QFutureInterface<LocatorFilterEntry> &future, const QString &entry)
{
    QList<LocatorFilterEntry> entries[int(MatchLevel::Count)];
    const Qt::CaseSensitivity caseSensitivityForPrefix = caseSensitivity(entry);
    const IndexItem::ItemType wanted = matchTypes();

    const QRegularExpression regexp = createRegExp(entry);
    if (!regexp.isValid())
        return {};
    const bool hasColonColon = entry.contains("::");
    const QRegularExpression shortRegexp =
            hasColonColon ? createRegExp(entry.mid(entry.lastIndexOf("::") + 2)) : regexp;

    CppLocatorData *locatorData = CppModelManager::instance()->locatorData();
    locatorData->filterAllFiles([&](const IndexItem::Ptr &info) -> IndexItem::VisitorResult {
        if (future.isCanceled())
            return IndexItem::Break;
        const IndexItem::ItemType type = info->type();
        if (type & wanted) {
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
                LocatorFilterEntry filterEntry = filterEntryFromIndexItem(info);

                // Highlight the matched characters, therefore it may be necessary
                // to update the match if the displayName is different from matchString
                if (QStringView(matchString).mid(matchOffset) != filterEntry.displayName) {
                    match = shortRegexp.match(filterEntry.displayName);
                    matchOffset = 0;
                }
                filterEntry.highlightInfo = highlightInfo(match);
                if (matchInParameterList && filterEntry.highlightInfo.startsDisplay.isEmpty()) {
                    match = regexp.match(filterEntry.extraInfo);
                    filterEntry.highlightInfo
                        = highlightInfo(match, LocatorFilterEntry::HighlightInfo::ExtraInfo);
                } else if (matchOffset > 0) {
                    for (int &start : filterEntry.highlightInfo.startsDisplay)
                        start -= matchOffset;
                }

                if (matchInParameterList)
                    entries[int(MatchLevel::Normal)].append(filterEntry);
                else if (filterEntry.displayName.startsWith(entry, caseSensitivityForPrefix))
                    entries[int(MatchLevel::Best)].append(filterEntry);
                else if (filterEntry.displayName.contains(entry, caseSensitivityForPrefix))
                    entries[int(MatchLevel::Better)].append(filterEntry);
                else
                    entries[int(MatchLevel::Good)].append(filterEntry);
            }
        }

        if (info->type() & IndexItem::Enum)
            return IndexItem::Continue;
        else
            return IndexItem::Recurse;
    });

    for (auto &entry : entries) {
        if (entry.size() < 1000)
            Utils::sort(entry, LocatorFilterEntry::compareLexigraphically);
    }

    return std::accumulate(std::begin(entries), std::end(entries), QList<LocatorFilterEntry>());
}

CppClassesFilter::CppClassesFilter()
{
    setId(Constants::CLASSES_FILTER_ID);
    setDisplayName(Tr::tr(Constants::CLASSES_FILTER_DISPLAY_NAME));
    setDescription(Tr::tr(Constants::CLASSES_FILTER_DESCRIPTION));
    setDefaultShortcutString("c");
    setDefaultIncludedByDefault(false);
}

LocatorFilterEntry CppClassesFilter::filterEntryFromIndexItem(IndexItem::Ptr info)
{
    LocatorFilterEntry filterEntry(this, info->symbolName());
    filterEntry.displayIcon = info->icon();
    filterEntry.linkForEditor = {info->filePath(), info->line(), info->column()};
    filterEntry.extraInfo = info->symbolScope().isEmpty()
        ? info->shortNativeFilePath()
        : info->symbolScope();
    filterEntry.filePath = info->filePath();
    return filterEntry;
}

CppFunctionsFilter::CppFunctionsFilter()
{
    setId(Constants::FUNCTIONS_FILTER_ID);
    setDisplayName(Tr::tr(Constants::FUNCTIONS_FILTER_DISPLAY_NAME));
    setDescription(Tr::tr(Constants::FUNCTIONS_FILTER_DESCRIPTION));
    setDefaultShortcutString("m");
    setDefaultIncludedByDefault(false);
}

LocatorFilterEntry CppFunctionsFilter::filterEntryFromIndexItem(IndexItem::Ptr info)
{
    QString name = info->symbolName();
    QString extraInfo = info->symbolScope();
    info->unqualifiedNameAndScope(name, &name, &extraInfo);
    if (extraInfo.isEmpty()) {
        extraInfo = info->shortNativeFilePath();
    } else {
        extraInfo.append(" (" + info->filePath().fileName() + ')');
    }

    LocatorFilterEntry filterEntry(this, name + info->symbolType());
    filterEntry.displayIcon = info->icon();
    filterEntry.linkForEditor = {info->filePath(), info->line(), info->column()};
    filterEntry.extraInfo = extraInfo;

    return filterEntry;
}

} // namespace CppEditor

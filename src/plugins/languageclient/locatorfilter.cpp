// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "locatorfilter.h"

#include "clientrequest.h"
#include "currentdocumentsymbolsrequest.h"
#include "languageclientmanager.h"
#include "languageclienttr.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/async.h>
#include <utils/fuzzymatcher.h>

#include <QRegularExpression>

using namespace Core;
using namespace LanguageServerProtocol;
using namespace Utils;

namespace LanguageClient {

void filterResults(QPromise<void> &promise, const LocatorStorage &storage, Client *client,
                   const QList<SymbolInformation> &results, const QList<SymbolKind> &filter)
{
    const auto doFilter = [&](const SymbolInformation &info) {
        return filter.contains(SymbolKind(info.kind()));
    };
    if (promise.isCanceled())
        return;
    const QList<SymbolInformation> filteredResults = filter.isEmpty() ? results
        : Utils::filtered(results, doFilter);
    const auto generateEntry = [client](const SymbolInformation &info) {
        LocatorFilterEntry entry;
        entry.displayName = info.name();
        if (std::optional<QString> container = info.containerName())
            entry.extraInfo = container.value_or(QString());
        entry.displayIcon = symbolIcon(info.kind());
        entry.linkForEditor = info.location().toLink(client->hostPathMapper());
        return entry;
    };
    storage.reportOutput(Utils::transform(filteredResults, generateEntry));
}

LocatorMatcherTask locatorMatcher(Client *client, int maxResultCount,
                                  const QList<SymbolKind> &filter)
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;
    TreeStorage<QList<SymbolInformation>> resultStorage;

    const auto onQuerySetup = [storage, client, maxResultCount](ClientWorkspaceSymbolRequest &request) {
        request.setClient(client);
        WorkspaceSymbolParams params;
        params.setQuery(storage->input());
        if (maxResultCount > 0)
            params.setLimit(maxResultCount);
        request.setParams(params);
    };
    const auto onQueryDone = [resultStorage](const ClientWorkspaceSymbolRequest &request) {
        const std::optional<LanguageClientArray<SymbolInformation>> result
            = request.response().result();
        if (result.has_value())
            *resultStorage = result->toList();
    };

    const auto onFilterSetup = [storage, resultStorage, client, filter](Async<void> &async) {
        const QList<SymbolInformation> results = *resultStorage;
        if (results.isEmpty())
            return SetupResult::StopWithDone;
        async.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        async.setConcurrentCallData(filterResults, *storage, client, results, filter);
        return SetupResult::Continue;
    };

    const Group root {
        Tasking::Storage(resultStorage),
        ClientWorkspaceSymbolRequestTask(onQuerySetup, onQueryDone),
        AsyncTask<void>(onFilterSetup)
    };
    return {root, storage};
}

LocatorMatcherTask allSymbolsMatcher(Client *client, int maxResultCount)
{
    return locatorMatcher(client, maxResultCount, {});
}

LocatorMatcherTask classMatcher(Client *client, int maxResultCount)
{
    return locatorMatcher(client, maxResultCount, {SymbolKind::Class, SymbolKind::Struct});
}

LocatorMatcherTask functionMatcher(Client *client, int maxResultCount)
{
    return locatorMatcher(client, maxResultCount,
                          {SymbolKind::Method, SymbolKind::Function, SymbolKind::Constructor});
}

static void filterCurrentResults(QPromise<void> &promise, const LocatorStorage &storage,
                                 const CurrentDocumentSymbolsData &currentSymbolsData)
{
    Q_UNUSED(promise)
    const auto docSymbolModifier = [](LocatorFilterEntry &entry, const DocumentSymbol &info,
                                      const LocatorFilterEntry &parent) {
        Q_UNUSED(parent)
        entry.displayName = info.name();
        if (std::optional<QString> detail = info.detail())
            entry.extraInfo = *detail;
    };
    // TODO: Pass promise into currentSymbols
    storage.reportOutput(LanguageClient::currentDocumentSymbols(storage.input(), currentSymbolsData,
                                                                docSymbolModifier));
}

LocatorMatcherTask currentDocumentMatcher()
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;
    TreeStorage<CurrentDocumentSymbolsData> resultStorage;

    const auto onQuerySetup = [](CurrentDocumentSymbolsRequest &request) {
        Q_UNUSED(request)
    };
    const auto onQueryDone = [resultStorage](const CurrentDocumentSymbolsRequest &request) {
        *resultStorage = request.currentDocumentSymbolsData();
    };

    const auto onFilterSetup = [storage, resultStorage](Async<void> &async) {
        async.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        async.setConcurrentCallData(filterCurrentResults, *storage, *resultStorage);
    };

    const Group root {
        Tasking::Storage(resultStorage),
        CurrentDocumentSymbolsRequestTask(onQuerySetup, onQueryDone),
        AsyncTask<void>(onFilterSetup)
    };
    return {root, storage};
}

using MatcherCreator = std::function<Core::LocatorMatcherTask(Client *, int)>;

static MatcherCreator creatorForType(MatcherType type)
{
    switch (type) {
    case MatcherType::AllSymbols: return &allSymbolsMatcher;
    case MatcherType::Classes: return &classMatcher;
    case MatcherType::Functions: return &functionMatcher;
    case MatcherType::CurrentDocumentSymbols: QTC_CHECK(false); return {};
    }
    return {};
}

LocatorMatcherTasks languageClientMatchers(MatcherType type, const QList<Client *> &clients,
                                           int maxResultCount)
{
    if (type == MatcherType::CurrentDocumentSymbols)
        return {currentDocumentMatcher()};
    const MatcherCreator creator = creatorForType(type);
    if (!creator)
        return {};
    LocatorMatcherTasks matchers;
    for (Client *client : clients)
        matchers << creator(client, maxResultCount);
    return matchers;
}

LanguageCurrentDocumentFilter::LanguageCurrentDocumentFilter()
{
    setId(Constants::LANGUAGECLIENT_DOCUMENT_FILTER_ID);
    setDisplayName(Tr::tr(Constants::LANGUAGECLIENT_DOCUMENT_FILTER_DISPLAY_NAME));
    setDescription(Tr::tr(Constants::LANGUAGECLIENT_DOCUMENT_FILTER_DESCRIPTION));
    setDefaultShortcutString(".");
    setPriority(ILocatorFilter::Low);
}

LocatorMatcherTasks LanguageCurrentDocumentFilter::matchers()
{
    return {currentDocumentMatcher()};
}

static LocatorFilterEntry entryForSymbolInfo(const SymbolInformation &info,
                                             const DocumentUri::PathMapper &pathMapper)
{
    LocatorFilterEntry entry;
    entry.displayName = info.name();
    if (std::optional<QString> container = info.containerName())
        entry.extraInfo = container.value_or(QString());
    entry.displayIcon = symbolIcon(info.kind());
    entry.linkForEditor = info.location().toLink(pathMapper);
    return entry;
}

LocatorFilterEntries entriesForSymbolsInfo(const QList<SymbolInformation> &infoList,
    const QRegularExpression &regexp, const DocumentUri::PathMapper &pathMapper)
{
    QTC_ASSERT(pathMapper, return {});
    LocatorFilterEntries entries;
    for (const SymbolInformation &info : infoList) {
        if (regexp.match(info.name()).hasMatch())
            entries << LanguageClient::entryForSymbolInfo(info, pathMapper);
    }
    return entries;
}

LocatorFilterEntries entriesForDocSymbols(const QList<DocumentSymbol> &infoList,
    const QRegularExpression &regexp, const FilePath &filePath,
    const DocSymbolModifier &docSymbolModifier, const LocatorFilterEntry &parent = {})
{
    LocatorFilterEntries entries;
    for (const DocumentSymbol &info : infoList) {
        const QList<DocumentSymbol> children = info.children().value_or(QList<DocumentSymbol>());
        const bool hasMatch = regexp.match(info.name()).hasMatch();
        LocatorFilterEntry entry;
        if (hasMatch) {
            entry.displayIcon = LanguageClient::symbolIcon(info.kind());
            const Position &pos = info.range().start();
            entry.linkForEditor = {filePath, pos.line() + 1, pos.character()};
            docSymbolModifier(entry, info, parent);
            entries << entry;
        } else {
            entry = parent;
        }
        entries << entriesForDocSymbols(children, regexp, filePath, docSymbolModifier, entry);
    }
    return entries;
}

Core::LocatorFilterEntries currentDocumentSymbols(const QString &input,
                                                  const CurrentDocumentSymbolsData &currentSymbolsData,
                                                  const DocSymbolModifier &docSymbolModifier)
{
    const Qt::CaseSensitivity caseSensitivity = ILocatorFilter::caseSensitivity(input);
    const QRegularExpression regExp = ILocatorFilter::createRegExp(input, caseSensitivity);
    if (!regExp.isValid())
        return {};

    if (auto list = std::get_if<QList<DocumentSymbol>>(&currentSymbolsData.m_symbols))
        return entriesForDocSymbols(*list, regExp, currentSymbolsData.m_filePath, docSymbolModifier);
    else if (auto list = std::get_if<QList<SymbolInformation>>(&currentSymbolsData.m_symbols))
        return entriesForSymbolsInfo(*list, regExp, currentSymbolsData.m_pathMapper);
    return {};
}

LanguageAllSymbolsFilter::LanguageAllSymbolsFilter()
{
    setId(Constants::LANGUAGECLIENT_WORKSPACE_FILTER_ID);
    setDisplayName(Tr::tr(Constants::LANGUAGECLIENT_WORKSPACE_FILTER_DISPLAY_NAME));
    setDescription(Tr::tr(Constants::LANGUAGECLIENT_WORKSPACE_FILTER_DESCRIPTION));
    setDefaultShortcutString(":");
    setPriority(ILocatorFilter::Low);
}

LocatorMatcherTasks LanguageAllSymbolsFilter::matchers()
{
    return languageClientMatchers(MatcherType::AllSymbols,
        Utils::filtered(LanguageClientManager::clients(), &Client::locatorsEnabled));
}

LanguageClassesFilter::LanguageClassesFilter()
{
    setId(Constants::LANGUAGECLIENT_WORKSPACE_CLASS_FILTER_ID);
    setDisplayName(Tr::tr(Constants::LANGUAGECLIENT_WORKSPACE_CLASS_FILTER_DISPLAY_NAME));
    setDescription(Tr::tr(Constants::LANGUAGECLIENT_WORKSPACE_CLASS_FILTER_DESCRIPTION));
    setDefaultShortcutString("c");
}

LocatorMatcherTasks LanguageClassesFilter::matchers()
{
    return languageClientMatchers(MatcherType::Classes,
        Utils::filtered(LanguageClientManager::clients(), &Client::locatorsEnabled));
}

LanguageFunctionsFilter::LanguageFunctionsFilter()
{
    setId(Constants::LANGUAGECLIENT_WORKSPACE_METHOD_FILTER_ID);
    setDisplayName(Tr::tr(Constants::LANGUAGECLIENT_WORKSPACE_METHOD_FILTER_DISPLAY_NAME));
    setDescription(Tr::tr(Constants::LANGUAGECLIENT_WORKSPACE_METHOD_FILTER_DESCRIPTION));
    setDefaultShortcutString("m");
}

LocatorMatcherTasks LanguageFunctionsFilter::matchers()
{
    return languageClientMatchers(MatcherType::Functions,
        Utils::filtered(LanguageClientManager::clients(), &Client::locatorsEnabled));
}

} // namespace LanguageClient

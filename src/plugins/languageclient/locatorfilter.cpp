// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "locatorfilter.h"

#include "documentsymbolcache.h"
#include "languageclient_global.h"
#include "languageclientmanager.h"
#include "languageclientutils.h"

#include <coreplugin/editormanager/editormanager.h>

#include <languageserverprotocol/lsptypes.h>
#include <languageserverprotocol/servercapabilities.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/fuzzymatcher.h>
#include <utils/linecolumn.h>

#include <QFutureWatcher>
#include <QRegularExpression>

using namespace LanguageServerProtocol;

namespace LanguageClient {

DocumentLocatorFilter::DocumentLocatorFilter()
{
    setId(Constants::LANGUAGECLIENT_DOCUMENT_FILTER_ID);
    setDisplayName(Constants::LANGUAGECLIENT_DOCUMENT_FILTER_DISPLAY_NAME);
    setDescription(
        tr("Matches all symbols from the current document, based on a language server."));
    setDefaultShortcutString(".");
    setDefaultIncludedByDefault(false);
    setPriority(ILocatorFilter::Low);
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &DocumentLocatorFilter::updateCurrentClient);
}

void DocumentLocatorFilter::updateCurrentClient()
{
    resetSymbols();
    disconnect(m_resetSymbolsConnection);

    TextEditor::TextDocument *document = TextEditor::TextDocument::currentTextDocument();
    if (Client *client = LanguageClientManager::clientForDocument(document);
            client && (client->locatorsEnabled() || m_forced)) {

        setEnabled(!m_forced);
        if (m_symbolCache != client->documentSymbolCache()) {
            disconnect(m_updateSymbolsConnection);
            m_symbolCache = client->documentSymbolCache();
            m_updateSymbolsConnection = connect(m_symbolCache, &DocumentSymbolCache::gotSymbols,
                                                this, &DocumentLocatorFilter::updateSymbols);
        }
        m_resetSymbolsConnection = connect(document, &Core::IDocument::contentsChanged,
                                           this, &DocumentLocatorFilter::resetSymbols);
        m_currentUri = client->hostPathToServerUri(document->filePath());
        m_pathMapper = client->hostPathMapper();
    } else {
        disconnect(m_updateSymbolsConnection);
        m_symbolCache.clear();
        m_currentUri.clear();
        setEnabled(false);
        m_pathMapper = DocumentUri::PathMapper();
    }
}

void DocumentLocatorFilter::updateSymbols(const DocumentUri &uri,
                                          const DocumentSymbolsResult &symbols)
{
    if (uri != m_currentUri)
        return;
    QMutexLocker locker(&m_mutex);
    m_currentSymbols = symbols;
    emit symbolsUpToDate(QPrivateSignal());
}

void DocumentLocatorFilter::resetSymbols()
{
    QMutexLocker locker(&m_mutex);
    m_currentSymbols.reset();
}

static Core::LocatorFilterEntry generateLocatorEntry(const SymbolInformation &info,
                                                     Core::ILocatorFilter *filter,
                                                     DocumentUri::PathMapper pathMapper)
{
    Core::LocatorFilterEntry entry;
    entry.filter = filter;
    entry.displayName = info.name();
    if (std::optional<QString> container = info.containerName())
        entry.extraInfo = container.value_or(QString());
    entry.displayIcon = symbolIcon(info.kind());
    entry.internalData = QVariant::fromValue(info.location().toLink(pathMapper));
    return entry;
}

Core::LocatorFilterEntry DocumentLocatorFilter::generateLocatorEntry(const SymbolInformation &info)
{
    QTC_ASSERT(m_pathMapper, return {});
    return LanguageClient::generateLocatorEntry(info, this, m_pathMapper);
}

QList<Core::LocatorFilterEntry> DocumentLocatorFilter::generateLocatorEntries(
        const SymbolInformation &info, const QRegularExpression &regexp,
        const Core::LocatorFilterEntry &parent)
{
    Q_UNUSED(parent)
    if (regexp.match(info.name()).hasMatch())
        return {generateLocatorEntry(info)};
    return {};
}

Core::LocatorFilterEntry DocumentLocatorFilter::generateLocatorEntry(
        const DocumentSymbol &info,
        const Core::LocatorFilterEntry &parent)
{
    Q_UNUSED(parent)
    Core::LocatorFilterEntry entry;
    entry.filter = this;
    entry.displayName = info.name();
    if (std::optional<QString> detail = info.detail())
        entry.extraInfo = detail.value_or(QString());
    entry.displayIcon = symbolIcon(info.kind());
    const Position &pos = info.range().start();
    entry.internalData = QVariant::fromValue(Utils::LineColumn(pos.line(), pos.character()));
    return entry;
}

QList<Core::LocatorFilterEntry> DocumentLocatorFilter::generateLocatorEntries(
        const DocumentSymbol &info, const QRegularExpression &regexp,
        const Core::LocatorFilterEntry &parent)
{
    QList<Core::LocatorFilterEntry> entries;
    const QList<DocumentSymbol> children = info.children().value_or(QList<DocumentSymbol>());
    const bool hasMatch = regexp.match(info.name()).hasMatch();
    Core::LocatorFilterEntry entry;
    if (hasMatch || !children.isEmpty())
        entry = generateLocatorEntry(info, parent);
    if (hasMatch)
        entries << entry;
    for (const DocumentSymbol &child : children)
        entries << generateLocatorEntries(child, regexp, entry);
    return entries;
}

template<class T>
QList<Core::LocatorFilterEntry> DocumentLocatorFilter::generateEntries(const QList<T> &list,
                                                                       const QString &filter)
{
    QList<Core::LocatorFilterEntry> entries;
    FuzzyMatcher::CaseSensitivity caseSensitivity
        = ILocatorFilter::caseSensitivity(filter) == Qt::CaseSensitive
              ? FuzzyMatcher::CaseSensitivity::CaseSensitive
              : FuzzyMatcher::CaseSensitivity::CaseInsensitive;
    const QRegularExpression regexp = FuzzyMatcher::createRegExp(filter, caseSensitivity);
    if (!regexp.isValid())
        return entries;

    for (const T &item : list)
        entries << generateLocatorEntries(item, regexp, {});
    return entries;
}

void DocumentLocatorFilter::prepareSearch(const QString &/*entry*/)
{
    QMutexLocker locker(&m_mutex);
    if (m_symbolCache && !m_currentSymbols.has_value()) {
        locker.unlock();
        m_symbolCache->requestSymbols(m_currentUri, Schedule::Delayed);
    }
}

QList<Core::LocatorFilterEntry> DocumentLocatorFilter::matchesFor(
    QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    QMutexLocker locker(&m_mutex);
    if (!m_symbolCache)
        return {};
    if (!m_currentSymbols.has_value()) {
        QEventLoop loop;
        connect(this, &DocumentLocatorFilter::symbolsUpToDate, &loop, [&]() { loop.exit(1); });
        QFutureWatcher<Core::LocatorFilterEntry> watcher;
        connect(&watcher,
                &QFutureWatcher<Core::LocatorFilterEntry>::canceled,
                &loop,
                &QEventLoop::quit);
        watcher.setFuture(future.future());
        locker.unlock();
        if (!loop.exec())
            return {};
        locker.relock();
    }

    QTC_ASSERT(m_currentSymbols.has_value(), return {});

    if (auto list = std::get_if<QList<DocumentSymbol>>(&*m_currentSymbols))
        return generateEntries(*list, entry);
    else if (auto list = std::get_if<QList<SymbolInformation>>(&*m_currentSymbols))
        return generateEntries(*list, entry);

    return {};
}

void DocumentLocatorFilter::accept(const Core::LocatorFilterEntry &selection,
                                   QString * /*newText*/,
                                   int * /*selectionStart*/,
                                   int * /*selectionLength*/) const
{
    if (selection.internalData.canConvert<Utils::LineColumn>()) {
        QTC_ASSERT(m_pathMapper, return);
        auto lineColumn = qvariant_cast<Utils::LineColumn>(selection.internalData);
        const Utils::Link link(m_currentUri.toFilePath(m_pathMapper),
                               lineColumn.line + 1,
                               lineColumn.column);
        Core::EditorManager::openEditorAt(link, {}, Core::EditorManager::AllowExternalEditor);
    } else if (selection.internalData.canConvert<Utils::Link>()) {
        Core::EditorManager::openEditorAt(qvariant_cast<Utils::Link>(selection.internalData),
                                          {},
                                          Core::EditorManager::AllowExternalEditor);
    }
}

WorkspaceLocatorFilter::WorkspaceLocatorFilter()
    : WorkspaceLocatorFilter(QVector<SymbolKind>())
{}

WorkspaceLocatorFilter::WorkspaceLocatorFilter(const QVector<SymbolKind> &filter)
    : m_filterKinds(filter)
{
    setId(Constants::LANGUAGECLIENT_WORKSPACE_FILTER_ID);
    setDisplayName(Constants::LANGUAGECLIENT_WORKSPACE_FILTER_DISPLAY_NAME);
    setDefaultShortcutString(":");
    setDefaultIncludedByDefault(false);
    setPriority(ILocatorFilter::Low);
}

void WorkspaceLocatorFilter::prepareSearch(const QString &entry)
{
    prepareSearch(entry, LanguageClientManager::clients(), false);
}

void WorkspaceLocatorFilter::prepareSearch(const QString &entry, const QList<Client *> &clients)
{
    prepareSearch(entry, clients, true);
}

void WorkspaceLocatorFilter::prepareSearch(const QString &entry,
                                           const QList<Client *> &clients,
                                           bool force)
{
    m_pendingRequests.clear();
    m_results.clear();

    WorkspaceSymbolParams params;
    params.setQuery(entry);
    if (m_maxResultCount > 0)
        params.setLimit(m_maxResultCount);

    QMutexLocker locker(&m_mutex);
    for (auto client : std::as_const(clients)) {
        if (!client->reachable())
            continue;
        if (!(force || client->locatorsEnabled()))
            continue;
        std::optional<std::variant<bool, WorkDoneProgressOptions>> capability
            = client->capabilities().workspaceSymbolProvider();
        if (!capability.has_value())
            continue;
        if (std::holds_alternative<bool>(*capability) && !std::get<bool>(*capability))
            continue;
        WorkspaceSymbolRequest request(params);
        request.setResponseCallback(
            [this, client](const WorkspaceSymbolRequest::Response &response) {
                handleResponse(client, response);
            });
        m_pendingRequests[client] = request.id();
        client->sendMessage(request);
    }
}

QList<Core::LocatorFilterEntry> WorkspaceLocatorFilter::matchesFor(
    QFutureInterface<Core::LocatorFilterEntry> &future, const QString & /*entry*/)
{
    QMutexLocker locker(&m_mutex);
    if (!m_pendingRequests.isEmpty()) {
        QEventLoop loop;
        connect(this, &WorkspaceLocatorFilter::allRequestsFinished, &loop, [&]() { loop.exit(1); });
        QFutureWatcher<Core::LocatorFilterEntry> watcher;
        connect(&watcher,
                &QFutureWatcher<Core::LocatorFilterEntry>::canceled,
                &loop,
                &QEventLoop::quit);
        watcher.setFuture(future.future());
        locker.unlock();
        if (!loop.exec())
            return {};

        locker.relock();
    }


    if (!m_filterKinds.isEmpty()) {
        m_results = Utils::filtered(m_results, [&](const SymbolInfoWithPathMapper &info) {
            return m_filterKinds.contains(SymbolKind(info.symbol.kind()));
        });
    }
    auto generateEntry = [this](const SymbolInfoWithPathMapper &info) {
        return generateLocatorEntry(info.symbol, this, info.mapper);
    };
    return Utils::transform(m_results, generateEntry).toList();
}

void WorkspaceLocatorFilter::accept(const Core::LocatorFilterEntry &selection,
                                    QString * /*newText*/,
                                    int * /*selectionStart*/,
                                    int * /*selectionLength*/) const
{
    if (selection.internalData.canConvert<Utils::Link>())
        Core::EditorManager::openEditorAt(qvariant_cast<Utils::Link>(selection.internalData),
                                          {},
                                          Core::EditorManager::AllowExternalEditor);
}

void WorkspaceLocatorFilter::handleResponse(Client *client,
                                            const WorkspaceSymbolRequest::Response &response)
{
    QMutexLocker locker(&m_mutex);
    m_pendingRequests.remove(client);
    auto result = response.result().value_or(LanguageClientArray<SymbolInformation>());
    if (!result.isNull())
        m_results.append(
            Utils::transform(result.toList().toVector(), [client](const SymbolInformation &info) {
                return SymbolInfoWithPathMapper{info, client->hostPathMapper()};
            }));
    if (m_pendingRequests.isEmpty())
        emit allRequestsFinished(QPrivateSignal());
}

WorkspaceClassLocatorFilter::WorkspaceClassLocatorFilter()
    : WorkspaceLocatorFilter({SymbolKind::Class, SymbolKind::Struct})
{
    setId(Constants::LANGUAGECLIENT_WORKSPACE_CLASS_FILTER_ID);
    setDisplayName(Constants::LANGUAGECLIENT_WORKSPACE_CLASS_FILTER_DISPLAY_NAME);
    setDefaultShortcutString("c");
}

WorkspaceMethodLocatorFilter::WorkspaceMethodLocatorFilter()
    : WorkspaceLocatorFilter({SymbolKind::Method, SymbolKind::Function, SymbolKind::Constructor})
{
    setId(Constants::LANGUAGECLIENT_WORKSPACE_METHOD_FILTER_ID);
    setDisplayName(Constants::LANGUAGECLIENT_WORKSPACE_METHOD_FILTER_DISPLAY_NAME);
    setDefaultShortcutString("m");
}

} // namespace LanguageClient

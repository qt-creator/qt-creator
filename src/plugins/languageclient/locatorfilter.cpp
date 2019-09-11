/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "locatorfilter.h"

#include "languageclient_global.h"
#include "languageclientmanager.h"
#include "languageclientutils.h"

#include <coreplugin/editormanager/editormanager.h>
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
    setShortcutString(".");
    setIncludedByDefault(false);
    setPriority(ILocatorFilter::Low);
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &DocumentLocatorFilter::updateCurrentClient);
}

void DocumentLocatorFilter::updateCurrentClient()
{
    resetSymbols();
    disconnect(m_resetSymbolsConnection);

    TextEditor::TextDocument *document = TextEditor::TextDocument::currentTextDocument();
    if (Client *client = LanguageClientManager::clientForDocument(document)) {
        if (m_symbolCache != client->documentSymbolCache()) {
            disconnect(m_updateSymbolsConnection);
            m_symbolCache = client->documentSymbolCache();
            m_updateSymbolsConnection = connect(m_symbolCache, &DocumentSymbolCache::gotSymbols,
                                                this, &DocumentLocatorFilter::updateSymbols);
        }
        m_resetSymbolsConnection = connect(document, &Core::IDocument::contentsChanged,
                                           this, &DocumentLocatorFilter::resetSymbols);
        m_currentUri = DocumentUri::fromFilePath(document->filePath());
    } else {
        disconnect(m_updateSymbolsConnection);
        m_symbolCache.clear();
        m_currentUri.clear();
    }
}

void DocumentLocatorFilter::updateSymbols(const DocumentUri &uri,
                                          const DocumentSymbolsResult &symbols)
{
    if (uri != m_currentUri)
        return;
    QMutexLocker locker(&m_mutex);
    m_currentSymbols = symbols;
    emit symbolsUpToDate({});
}

void DocumentLocatorFilter::resetSymbols()
{
    QMutexLocker locker(&m_mutex);
    m_currentSymbols.reset();
}

Core::LocatorFilterEntry generateLocatorEntry(const SymbolInformation &info,
                                              Core::ILocatorFilter *filter)
{
    Core::LocatorFilterEntry entry;
    entry.filter = filter;
    entry.displayName = info.name();
    if (Utils::optional<QString> container = info.containerName())
        entry.extraInfo = container.value_or(QString());
    entry.displayIcon = symbolIcon(info.kind());
    entry.internalData = QVariant::fromValue(info.location().toLink());
    return entry;
}

Core::LocatorFilterEntry generateLocatorEntry(const DocumentSymbol &info,
                                              Core::ILocatorFilter *filter)
{
    Core::LocatorFilterEntry entry;
    entry.filter = filter;
    entry.displayName = info.name();
    if (Utils::optional<QString> detail = info.detail())
        entry.extraInfo = detail.value_or(QString());
    entry.displayIcon = symbolIcon(info.kind());
    const Position &pos = info.range().start();
    entry.internalData = QVariant::fromValue(Utils::LineColumn(pos.line(), pos.character()));
    return entry;
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

    for (const T &item : list) {
        QRegularExpressionMatch match = regexp.match(item.name());
        if (match.hasMatch())
            entries << generateLocatorEntry(item, this);
    }
    return entries;
}

void DocumentLocatorFilter::prepareSearch(const QString &/*entry*/)
{
    QMutexLocker locker(&m_mutex);
    if (m_symbolCache && !m_currentSymbols.has_value()) {
        locker.unlock();
        m_symbolCache->requestSymbols(m_currentUri);
    }
}

QList<Core::LocatorFilterEntry> DocumentLocatorFilter::matchesFor(
    QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    if (!m_symbolCache)
        return {};
    QMutexLocker locker(&m_mutex);
    if (!m_currentSymbols.has_value()) {
        QEventLoop loop;
        connect(this, &DocumentLocatorFilter::symbolsUpToDate, &loop, [&]() { loop.exit(1); });
        QFutureWatcher<Core::LocatorFilterEntry> watcher;
        watcher.setFuture(future.future());
        connect(&watcher,
                &QFutureWatcher<Core::LocatorFilterEntry>::canceled,
                &loop,
                &QEventLoop::quit);
        locker.unlock();
        if (!loop.exec())
            return {};
        locker.relock();
    }

    QTC_ASSERT(m_currentSymbols.has_value(), return {});

    if (auto list = Utils::get_if<QList<DocumentSymbol>>(&m_currentSymbols.value()))
        return generateEntries(*list, entry);
    else if (auto list = Utils::get_if<QList<SymbolInformation>>(&m_currentSymbols.value()))
        return generateEntries(*list, entry);

    return {};
}

void DocumentLocatorFilter::accept(Core::LocatorFilterEntry selection,
                                   QString * /*newText*/,
                                   int * /*selectionStart*/,
                                   int * /*selectionLength*/) const
{
    if (selection.internalData.canConvert<Utils::LineColumn>()) {
        auto lineColumn = qvariant_cast<Utils::LineColumn>(selection.internalData);
        Core::EditorManager::openEditorAt(m_currentUri.toFilePath().toString(),
                                          lineColumn.line + 1,
                                          lineColumn.column);
    } else if (selection.internalData.canConvert<Utils::Link>()) {
        auto link = qvariant_cast<Utils::Link>(selection.internalData);
        Core::EditorManager::openEditorAt(link.targetFileName, link.targetLine, link.targetColumn);
    }
}

void DocumentLocatorFilter::refresh(QFutureInterface<void> & /*future*/) {}

WorkspaceLocatorFilter::WorkspaceLocatorFilter()
    : WorkspaceLocatorFilter(QVector<SymbolKind>())
{}

WorkspaceLocatorFilter::WorkspaceLocatorFilter(const QVector<SymbolKind> &filter)
    : m_filterKinds(filter)
{
    setId(Constants::LANGUAGECLIENT_WORKSPACE_FILTER_ID);
    setDisplayName(Constants::LANGUAGECLIENT_WORKSPACE_FILTER_DISPLAY_NAME);
    setShortcutString(":");
    setIncludedByDefault(false);
    setPriority(ILocatorFilter::Low);
}

void WorkspaceLocatorFilter::prepareSearch(const QString &entry)
{
    m_pendingRequests.clear();
    m_results.clear();

    WorkspaceSymbolParams params;
    params.setQuery(entry);

    QMutexLocker locker(&m_mutex);
    for (auto client : Utils::filtered(LanguageClientManager::clients(), &Client::reachable)) {
        if (client->capabilities().workspaceSymbolProvider().value_or(false)) {
            WorkspaceSymbolRequest request(params);
            request.setResponseCallback(
                [this, client](const WorkspaceSymbolRequest::Response &response) {
                    handleResponse(client, response);
                });
            m_pendingRequests[client] = request.id();
            client->sendContent(request);
        }
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
        watcher.setFuture(future.future());
        connect(&watcher,
                &QFutureWatcher<Core::LocatorFilterEntry>::canceled,
                &loop,
                &QEventLoop::quit);
        locker.unlock();
        if (!loop.exec())
            return {};

        locker.relock();
    }


    if (!m_filterKinds.isEmpty()) {
        m_results = Utils::filtered(m_results, [&](const SymbolInformation &info) {
            return m_filterKinds.contains(SymbolKind(info.kind()));
        });
    }
    return Utils::transform(m_results,
                            [this](const SymbolInformation &info) {
                                return generateLocatorEntry(info, this);
                            })
        .toList();
}

void WorkspaceLocatorFilter::accept(Core::LocatorFilterEntry selection,
                                    QString * /*newText*/,
                                    int * /*selectionStart*/,
                                    int * /*selectionLength*/) const
{
    if (selection.internalData.canConvert<Utils::Link>()) {
        auto link = qvariant_cast<Utils::Link>(selection.internalData);
        Core::EditorManager::openEditorAt(link.targetFileName, link.targetLine, link.targetColumn);
    }
}

void WorkspaceLocatorFilter::refresh(QFutureInterface<void> & /*future*/) {}

void WorkspaceLocatorFilter::handleResponse(Client *client,
                                            const WorkspaceSymbolRequest::Response &response)
{
    QMutexLocker locker(&m_mutex);
    m_pendingRequests.remove(client);
    auto result = response.result().value_or(LanguageClientArray<SymbolInformation>());
    if (!result.isNull())
        m_results.append(result.toList().toVector());
    if (m_pendingRequests.isEmpty())
        emit allRequestsFinished(QPrivateSignal());
}

WorkspaceClassLocatorFilter::WorkspaceClassLocatorFilter()
    : WorkspaceLocatorFilter({SymbolKind::Class, SymbolKind::Struct})
{
    setId(Constants::LANGUAGECLIENT_WORKSPACE_CLASS_FILTER_ID);
    setDisplayName(Constants::LANGUAGECLIENT_WORKSPACE_CLASS_FILTER_DISPLAY_NAME);
    setShortcutString("c");
}

WorkspaceMethodLocatorFilter::WorkspaceMethodLocatorFilter()
    : WorkspaceLocatorFilter({SymbolKind::Method, SymbolKind::Function, SymbolKind::Constructor})
{
    setId(Constants::LANGUAGECLIENT_WORKSPACE_METHOD_FILTER_ID);
    setDisplayName(Constants::LANGUAGECLIENT_WORKSPACE_METHOD_FILTER_DISPLAY_NAME);
    setShortcutString("m");
}

} // namespace LanguageClient

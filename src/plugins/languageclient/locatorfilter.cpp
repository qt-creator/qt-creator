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
    Core::IEditor *editor = Core::EditorManager::currentEditor();
    resetSymbols();
    disconnect(m_resetSymbolsConnection);

    if (Client *client = LanguageClientManager::clientForEditor(editor)) {
        if (m_symbolCache != client->documentSymbolCache()) {
            disconnect(m_updateSymbolsConnection);
            m_symbolCache = client->documentSymbolCache();
            m_updateSymbolsConnection = connect(m_symbolCache,
                                                &DocumentSymbolCache::gotSymbols,
                                                this,
                                                &DocumentLocatorFilter::updateSymbols);
        }
        m_resetSymbolsConnection = connect(editor->document(),
                                           &Core::IDocument::contentsChanged,
                                           this,
                                           &DocumentLocatorFilter::resetSymbols);
        m_currentUri = DocumentUri::fromFileName(editor->document()->filePath());
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

template<class T>
QList<Core::LocatorFilterEntry> DocumentLocatorFilter::generateEntries(const QList<T> &list,
                                                                       const QString &filter)
{
    QList<Core::LocatorFilterEntry> entries;
    FuzzyMatcher::CaseSensitivity caseSensitivity
        = DocumentLocatorFilter::caseSensitivity(filter) == Qt::CaseSensitive
              ? FuzzyMatcher::CaseSensitivity::CaseSensitive
              : FuzzyMatcher::CaseSensitivity::CaseInsensitive;
    const QRegularExpression regexp = FuzzyMatcher::createRegExp(filter, caseSensitivity);
    if (!regexp.isValid())
        return entries;

    for (const T &item : list) {
        QRegularExpressionMatch match = regexp.match(item.name());
        if (match.hasMatch())
            entries << generateLocatorEntry(item);
    }
    return entries;
}

Core::LocatorFilterEntry DocumentLocatorFilter::generateLocatorEntry(const SymbolInformation &info)
{
    Core::LocatorFilterEntry entry;
    entry.filter = this;
    entry.displayName = info.name();
    if (Utils::optional<QString> container = info.containerName())
        entry.extraInfo = container.value_or(QString());
    entry.displayIcon = symbolIcon(info.kind());
    const Position &pos = info.location().range().start();
    entry.internalData = qVariantFromValue(Utils::LineColumn(pos.line(), pos.character()));
    return entry;
}

Core::LocatorFilterEntry DocumentLocatorFilter::generateLocatorEntry(const DocumentSymbol &info)
{
    Core::LocatorFilterEntry entry;
    entry.filter = this;
    entry.displayName = info.name();
    if (Utils::optional<QString> detail = info.detail())
        entry.extraInfo = detail.value_or(QString());
    entry.displayIcon = symbolIcon(info.kind());
    const Position &pos = info.range().start();
    entry.internalData = qVariantFromValue(Utils::LineColumn(pos.line(), pos.character()));
    return entry;
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
    auto lineColumn = qvariant_cast<Utils::LineColumn>(selection.internalData);
    Core::EditorManager::openEditorAt(m_currentUri.toFileName().toString(),
                                      lineColumn.line + 1,
                                      lineColumn.column);
}

void DocumentLocatorFilter::refresh(QFutureInterface<void> & /*future*/) {}

} // namespace LanguageClient

// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdlocatorfilters.h"

#include "clangdclient.h"
#include "clangmodelmanagersupport.h"

#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppeditortr.h>
#include <cppeditor/cpplocatorfilter.h>

#include <extensionsystem/pluginmanager.h>

#include <languageclient/currentdocumentsymbolsrequest.h>
#include <languageclient/locatorfilter.h>

#include <utils/algorithm.h>
#include <utils/async.h>

#include <QHash>

using namespace Core;
using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace ClangCodeModel::Internal {

const int MaxResultCount = 10000;

ClangdAllSymbolsFilter::ClangdAllSymbolsFilter()
{
    setId(CppEditor::Constants::LOCATOR_FILTER_ID);
    setDisplayName(::CppEditor::Tr::tr(CppEditor::Constants::LOCATOR_FILTER_DISPLAY_NAME));
    setDescription(::CppEditor::Tr::tr(CppEditor::Constants::LOCATOR_FILTER_DESCRIPTION));
    setDefaultShortcutString(":");
}

LocatorMatcherTasks ClangdAllSymbolsFilter::matchers()
{
    return CppEditor::cppMatchers(MatcherType::AllSymbols)
           + LanguageClient::languageClientMatchers(MatcherType::AllSymbols,
                             ClangModelManagerSupport::clientsForOpenProjects(), MaxResultCount);
}

ClangdClassesFilter::ClangdClassesFilter()
{
    setId(CppEditor::Constants::CLASSES_FILTER_ID);
    setDisplayName(::CppEditor::Tr::tr(CppEditor::Constants::CLASSES_FILTER_DISPLAY_NAME));
    setDescription(::CppEditor::Tr::tr(CppEditor::Constants::CLASSES_FILTER_DESCRIPTION));
    setDefaultShortcutString("c");
}

LocatorMatcherTasks ClangdClassesFilter::matchers()
{
    return CppEditor::cppMatchers(MatcherType::Classes)
           + LanguageClient::languageClientMatchers(MatcherType::Classes,
                             ClangModelManagerSupport::clientsForOpenProjects(), MaxResultCount);
}

ClangdFunctionsFilter::ClangdFunctionsFilter()
{
    setId(CppEditor::Constants::FUNCTIONS_FILTER_ID);
    setDisplayName(::CppEditor::Tr::tr(CppEditor::Constants::FUNCTIONS_FILTER_DISPLAY_NAME));
    setDescription(::CppEditor::Tr::tr(CppEditor::Constants::FUNCTIONS_FILTER_DESCRIPTION));
    setDefaultShortcutString("m");
}

LocatorMatcherTasks ClangdFunctionsFilter::matchers()
{
    return CppEditor::cppMatchers(MatcherType::Functions)
           + LanguageClient::languageClientMatchers(MatcherType::Functions,
                             ClangModelManagerSupport::clientsForOpenProjects(), MaxResultCount);
}

ClangdCurrentDocumentFilter::ClangdCurrentDocumentFilter()
{
    setId(CppEditor::Constants::CURRENT_DOCUMENT_FILTER_ID);
    setDisplayName(::CppEditor::Tr::tr(CppEditor::Constants::CURRENT_DOCUMENT_FILTER_DISPLAY_NAME));
    setDescription(::CppEditor::Tr::tr(CppEditor::Constants::CURRENT_DOCUMENT_FILTER_DESCRIPTION));
    setDefaultShortcutString(".");
    setPriority(High);
    setEnabled(false);
    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, [this](const IEditor *editor) { setEnabled(editor); });
}

static void filterCurrentResults(QPromise<void> &promise, const LocatorStorage &storage,
                                 const CurrentDocumentSymbolsData &currentSymbolsData,
                                 const QString &contents)
{
    Q_UNUSED(promise)
    struct Entry
    {
        LocatorFilterEntry entry;
        DocumentSymbol symbol;
    };
    QList<Entry> docEntries;

    const auto docSymbolModifier = [&docEntries](LocatorFilterEntry &entry,
                                                 const DocumentSymbol &info,
                                                 const LocatorFilterEntry &parent) {
        entry.displayName = ClangdClient::displayNameFromDocumentSymbol(
            static_cast<SymbolKind>(info.kind()), info.name(),
            info.detail().value_or(QString()));
        entry.extraInfo = parent.extraInfo;
        if (!entry.extraInfo.isEmpty())
            entry.extraInfo.append("::");
        entry.extraInfo.append(parent.displayName);

        // TODO: Can we extend clangd to send visibility information?
        docEntries.append({entry, info});
        return entry;
    };
    // TODO: Pass promise into currentSymbols
    const LocatorFilterEntries allMatches = LanguageClient::currentDocumentSymbols(storage.input(),
                                            currentSymbolsData, docSymbolModifier);
    if (docEntries.isEmpty()) {
        storage.reportOutput(allMatches);
        return; // SymbolInformation case
    }

    QTC_CHECK(docEntries.size() == allMatches.size());
    QHash<QString, QList<Entry>> possibleDuplicates;
    for (const Entry &e : std::as_const(docEntries))
        possibleDuplicates[e.entry.displayName + e.entry.extraInfo] << e;
    const QTextDocument doc(contents);
    for (auto it = possibleDuplicates.cbegin(); it != possibleDuplicates.cend(); ++it) {
        const QList<Entry> &duplicates = it.value();
        if (duplicates.size() == 1)
            continue;
        QList<Entry> declarations;
        QList<Entry> definitions;
        for (const Entry &candidate : duplicates) {
            const DocumentSymbol symbol = candidate.symbol;
            const SymbolKind kind = static_cast<SymbolKind>(symbol.kind());
            if (kind != SymbolKind::Class && kind != SymbolKind::Function)
                break;
            const Range range = symbol.range();
            const Range selectionRange = symbol.selectionRange();
            if (kind == SymbolKind::Class) {
                if (range.end() == selectionRange.end())
                    declarations << candidate;
                else
                    definitions << candidate;
                continue;
            }
            const int startPos = selectionRange.end().toPositionInDocument(&doc);
            const int endPos = range.end().toPositionInDocument(&doc);
            const QString functionBody = contents.mid(startPos, endPos - startPos);

            // Hacky, but I don't see anything better.
            if (functionBody.contains('{') && functionBody.contains('}'))
                definitions << candidate;
            else
                declarations << candidate;
        }
        if (definitions.size() == 1
            && declarations.size() + definitions.size() == duplicates.size()) {
            for (const Entry &decl : std::as_const(declarations)) {
                Utils::erase(docEntries, [&decl](const Entry &e) {
                    return e.symbol == decl.symbol;
                });
            }
        }
    }
    storage.reportOutput(Utils::transform(docEntries,
                                          [](const Entry &entry) { return entry.entry; }));
}

LocatorMatcherTask currentDocumentMatcher()
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;
    TreeStorage<CurrentDocumentSymbolsData> resultStorage;

    const auto onQuerySetup = [=](CurrentDocumentSymbolsRequest &request) {
        Q_UNUSED(request)
    };
    const auto onQueryDone = [resultStorage](const CurrentDocumentSymbolsRequest &request) {
        *resultStorage = request.currentDocumentSymbolsData();
    };

    const auto onFilterSetup = [=](Async<void> &async) {
        async.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        async.setConcurrentCallData(filterCurrentResults, *storage, *resultStorage,
                                    TextDocument::currentTextDocument()->plainText());
    };

    const Group root {
        Tasking::Storage(resultStorage),
        CurrentDocumentSymbolsRequestTask(onQuerySetup, onQueryDone),
        AsyncTask<void>(onFilterSetup)
    };
    return {root, storage};
}

LocatorMatcherTasks ClangdCurrentDocumentFilter::matchers()
{
    const auto doc = TextDocument::currentTextDocument();
    QTC_ASSERT(doc, return {});
    if (const ClangdClient *client = ClangModelManagerSupport::clientForFile(doc->filePath());
        client && client->reachable()) {
        return {currentDocumentMatcher()};
    }
    return CppEditor::cppMatchers(MatcherType::CurrentDocumentSymbols);
}

} // namespace ClangCodeModel::Internal

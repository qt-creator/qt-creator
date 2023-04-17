// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdlocatorfilters.h"

#include "clangdclient.h"
#include "clangmodelmanagersupport.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppeditortr.h>
#include <cppeditor/cpplocatorfilter.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/indexitem.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientutils.h>
#include <languageclient/locatorfilter.h>
#include <projectexplorer/projectmanager.h>
#include <utils/link.h>
#include <utils/algorithm.h>

#include <QHash>

using namespace Core;
using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClangCodeModel {
namespace Internal {

const int MaxResultCount = 10000;

class CppLocatorFilter : public CppEditor::CppLocatorFilter
{
public:
    CppLocatorFilter()
    {
        setId({});
        setDisplayName({});
        setDescription({});
        setDefaultShortcutString({});
        setEnabled(false);
        setHidden(true);
    }
};

class LspWorkspaceFilter : public WorkspaceLocatorFilter
{
public:
    LspWorkspaceFilter()
    {
        setId({});
        setDisplayName({});
        setDescription({});
        setDefaultShortcutString({});
        setEnabled(false);
        setHidden(true);
        setMaxResultCount(MaxResultCount);
    }
    void prepareSearch(const QString &entry) override
    {
        prepareSearchForClients(entry, ClangModelManagerSupport::clientsForOpenProjects());
    }
};


class CppClassesFilter : public CppEditor::CppClassesFilter
{
public:
    CppClassesFilter()
    {
        setId({});
        setDisplayName({});
        setDescription({});
        setDefaultShortcutString({});
        setEnabled(false);
        setHidden(true);
    }
};

class LspClassesFilter : public WorkspaceClassLocatorFilter
{
public:
    LspClassesFilter() {
        setId({});
        setDisplayName({});
        setDescription({});
        setDefaultShortcutString({});
        setEnabled(false);
        setHidden(true);
        setMaxResultCount(MaxResultCount);
    }
    void prepareSearch(const QString &entry) override
    {
        prepareSearchForClients(entry, ClangModelManagerSupport::clientsForOpenProjects());
    }
};

class CppFunctionsFilter : public CppEditor::CppFunctionsFilter
{
public:
    CppFunctionsFilter()
    {
        setId({});
        setDisplayName({});
        setDescription({});
        setDefaultShortcutString({});
        setEnabled(false);
        setHidden(true);
    }
};

class LspFunctionsFilter : public WorkspaceMethodLocatorFilter
{
public:
    LspFunctionsFilter()
    {
        setId({});
        setDisplayName({});
        setDescription({});
        setDefaultShortcutString({});
        setEnabled(false);
        setHidden(true);
        setMaxResultCount(MaxResultCount);
    }
    void prepareSearch(const QString &entry) override
    {
        prepareSearchForClients(entry, ClangModelManagerSupport::clientsForOpenProjects());
    }
};


ClangGlobalSymbolFilter::ClangGlobalSymbolFilter()
    : ClangGlobalSymbolFilter(new CppLocatorFilter, new LspWorkspaceFilter)
{
}

ClangGlobalSymbolFilter::ClangGlobalSymbolFilter(ILocatorFilter *cppFilter,
                                                 ILocatorFilter *lspFilter)
    : m_cppFilter(cppFilter), m_lspFilter(lspFilter)
{
    setId(CppEditor::Constants::LOCATOR_FILTER_ID);
    setDisplayName(::CppEditor::Tr::tr(CppEditor::Constants::LOCATOR_FILTER_DISPLAY_NAME));
    setDescription(::CppEditor::Tr::tr(CppEditor::Constants::LOCATOR_FILTER_DESCRIPTION));
    setDefaultShortcutString(":");
    setDefaultIncludedByDefault(false);
}

ClangGlobalSymbolFilter::~ClangGlobalSymbolFilter()
{
    delete m_cppFilter;
    delete m_lspFilter;
}

void ClangGlobalSymbolFilter::prepareSearch(const QString &entry)
{
    m_cppFilter->prepareSearch(entry);
    m_lspFilter->prepareSearch(entry);
}

QList<LocatorFilterEntry> ClangGlobalSymbolFilter::matchesFor(
        QFutureInterface<LocatorFilterEntry> &future, const QString &entry)
{
    return m_cppFilter->matchesFor(future, entry) + m_lspFilter->matchesFor(future, entry);
}

ClangClassesFilter::ClangClassesFilter()
    : ClangGlobalSymbolFilter(new CppClassesFilter, new LspClassesFilter)
{
    setId(CppEditor::Constants::CLASSES_FILTER_ID);
    setDisplayName(::CppEditor::Tr::tr(CppEditor::Constants::CLASSES_FILTER_DISPLAY_NAME));
    setDescription(::CppEditor::Tr::tr(CppEditor::Constants::CLASSES_FILTER_DESCRIPTION));
    setDefaultShortcutString("c");
    setDefaultIncludedByDefault(false);
}

ClangFunctionsFilter::ClangFunctionsFilter()
    : ClangGlobalSymbolFilter(new CppFunctionsFilter, new LspFunctionsFilter)
{
    setId(CppEditor::Constants::FUNCTIONS_FILTER_ID);
    setDisplayName(::CppEditor::Tr::tr(CppEditor::Constants::FUNCTIONS_FILTER_DISPLAY_NAME));
    setDescription(::CppEditor::Tr::tr(CppEditor::Constants::FUNCTIONS_FILTER_DESCRIPTION));
    setDefaultShortcutString("m");
    setDefaultIncludedByDefault(false);
}

class LspCurrentDocumentFilter : public DocumentLocatorFilter
{
public:
    LspCurrentDocumentFilter()
    {
        setId({});
        setDisplayName({});
        setDescription({});
        setDefaultShortcutString({});
        setEnabled(false);
        setHidden(true);
        forceUse();
    }

private:
    void prepareSearch(const QString &entry) override
    {
        DocumentLocatorFilter::prepareSearch(entry);
        m_content = TextEditor::TextDocument::currentTextDocument()->plainText();
    }

    // Filter out declarations for which a definition is also present.
    QList<LocatorFilterEntry> matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                         const QString &entry) override
    {
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
        };

        const QList<LocatorFilterEntry> allMatches = matchesForImpl(future, entry,
                                                                    docSymbolModifier);
        if (docEntries.isEmpty())
            return allMatches; // SymbolInformation case

        QTC_CHECK(docEntries.size() == allMatches.size());
        QHash<QString, QList<Entry>> possibleDuplicates;
        for (const Entry &e : std::as_const(docEntries))
            possibleDuplicates[e.entry.displayName + e.entry.extraInfo] << e;
        const QTextDocument doc(m_content);
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
                const QString functionBody = m_content.mid(startPos, endPos - startPos);

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
        return Utils::transform(docEntries, [](const Entry &entry) { return entry.entry; });
    }

    QString m_content;
};

class ClangdCurrentDocumentFilter::Private
{
public:
    ~Private() { delete cppFilter; }

    ILocatorFilter * const cppFilter
            = CppEditor::CppModelManager::createAuxiliaryCurrentDocumentFilter();
    LspCurrentDocumentFilter lspFilter;
    ILocatorFilter *activeFilter = nullptr;
};


ClangdCurrentDocumentFilter::ClangdCurrentDocumentFilter() : d(new Private)
{
    setId(CppEditor::Constants::CURRENT_DOCUMENT_FILTER_ID);
    setDisplayName(::CppEditor::Tr::tr(CppEditor::Constants::CURRENT_DOCUMENT_FILTER_DISPLAY_NAME));
    setDescription(::CppEditor::Tr::tr(CppEditor::Constants::CURRENT_DOCUMENT_FILTER_DESCRIPTION));
    setDefaultShortcutString(".");
    setPriority(High);
    setDefaultIncludedByDefault(false);
    setEnabled(false);
    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, [this](const IEditor *editor) { setEnabled(editor); });
}

ClangdCurrentDocumentFilter::~ClangdCurrentDocumentFilter() { delete d; }

void ClangdCurrentDocumentFilter::prepareSearch(const QString &entry)
{
    const auto doc = TextEditor::TextDocument::currentTextDocument();
    QTC_ASSERT(doc, return);
    if (const ClangdClient * const client = ClangModelManagerSupport::clientForFile
            (doc->filePath()); client && client->reachable()) {
        d->activeFilter = &d->lspFilter;
    } else {
        d->activeFilter = d->cppFilter;
    }
    d->activeFilter->prepareSearch(entry);
}

QList<LocatorFilterEntry> ClangdCurrentDocumentFilter::matchesFor(
        QFutureInterface<LocatorFilterEntry> &future, const QString &entry)
{
    QTC_ASSERT(d->activeFilter, return {});
    return d->activeFilter->matchesFor(future, entry);
}

} // namespace Internal
} // namespace ClangCodeModel

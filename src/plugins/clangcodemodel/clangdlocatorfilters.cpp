// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdlocatorfilters.h"

#include "clangdclient.h"
#include "clangmodelmanagersupport.h"

#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppeditortr.h>
#include <cppeditor/cpplocatorfilter.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/indexitem.h>

#include <languageclient/languageclientutils.h>
#include <languageclient/locatorfilter.h>

#include <projectexplorer/projectmanager.h>

#include <utils/link.h>
#include <utils/algorithm.h>

#include <QHash>

#include <set>
#include <tuple>

using namespace Core;
using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace Utils;

namespace ClangCodeModel {
namespace Internal {

const int MaxResultCount = 10000;

class CppLocatorFilter : public CppEditor::CppLocatorFilter
{
public:
    CppLocatorFilter()
        : CppEditor::CppLocatorFilter(CppEditor::CppModelManager::instance()->locatorData())
    {
        setId({});
        setDisplayName({});
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
        setDefaultShortcutString({});
        setEnabled(false);
        setHidden(true);
        setMaxResultCount(MaxResultCount);
    }
};


class CppClassesFilter : public CppEditor::CppClassesFilter
{
public:
    CppClassesFilter()
        : CppEditor::CppClassesFilter(CppEditor::CppModelManager::instance()->locatorData())
    {
        setId({});
        setDisplayName({});
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
        setDefaultShortcutString({});
        setEnabled(false);
        setHidden(true);
        setMaxResultCount(MaxResultCount);
    }
};

class CppFunctionsFilter : public CppEditor::CppFunctionsFilter
{
public:
    CppFunctionsFilter()
        : CppEditor::CppFunctionsFilter(CppEditor::CppModelManager::instance()->locatorData())
    {
        setId({});
        setDisplayName({});
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
        setDefaultShortcutString({});
        setEnabled(false);
        setHidden(true);
        setMaxResultCount(MaxResultCount);
    }
};


ClangGlobalSymbolFilter::ClangGlobalSymbolFilter()
    : ClangGlobalSymbolFilter(new CppLocatorFilter, new LspWorkspaceFilter)
{
}

ClangGlobalSymbolFilter::ClangGlobalSymbolFilter(ILocatorFilter *cppFilter,
                                                 WorkspaceLocatorFilter *lspFilter)
    : m_cppFilter(cppFilter), m_lspFilter(lspFilter)
{
    setId(CppEditor::Constants::LOCATOR_FILTER_ID);
    setDisplayName(::CppEditor::Tr::tr(CppEditor::Constants::LOCATOR_FILTER_DISPLAY_NAME));
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
    QList<Client *> clients;
    for (ProjectExplorer::Project * const project : ProjectExplorer::ProjectManager::projects()) {
        if (Client * const client = ClangModelManagerSupport::clientForProject(project))
            clients << client;
    }
    if (!clients.isEmpty())
        m_lspFilter->prepareSearch(entry, clients);
}

QList<LocatorFilterEntry> ClangGlobalSymbolFilter::matchesFor(
        QFutureInterface<LocatorFilterEntry> &future, const QString &entry)
{
    QList<LocatorFilterEntry> matches = m_cppFilter->matchesFor(future, entry);
    const QList<LocatorFilterEntry> lspMatches = m_lspFilter->matchesFor(future, entry);
    if (!lspMatches.isEmpty()) {
        std::set<std::tuple<FilePath, int, int>> locations;
        for (const auto &entry : std::as_const(matches)) {
            const CppEditor::IndexItem::Ptr item
                    = qvariant_cast<CppEditor::IndexItem::Ptr>(entry.internalData);
            locations.insert(std::make_tuple(item->filePath(), item->line(), item->column()));
        }
        for (const auto &entry : lspMatches) {
            if (!entry.internalData.canConvert<Link>())
                continue;
            const auto link = qvariant_cast<Link>(entry.internalData);
            if (locations.find(std::make_tuple(link.targetFilePath, link.targetLine,
                                               link.targetColumn)) == locations.cend()) {
                matches << entry; // TODO: Insert sorted?
            }
        }
    }

    return matches;
}

void ClangGlobalSymbolFilter::accept(const LocatorFilterEntry &selection, QString *newText,
                                     int *selectionStart, int *selectionLength) const
{
    if (qvariant_cast<CppEditor::IndexItem::Ptr>(selection.internalData))
        m_cppFilter->accept(selection, newText, selectionStart, selectionLength);
    else
        m_lspFilter->accept(selection, newText, selectionStart, selectionLength);
}


ClangClassesFilter::ClangClassesFilter()
    : ClangGlobalSymbolFilter(new CppClassesFilter, new LspClassesFilter)
{
    setId(CppEditor::Constants::CLASSES_FILTER_ID);
    setDisplayName(::CppEditor::Tr::tr(CppEditor::Constants::CLASSES_FILTER_DISPLAY_NAME));
    setDefaultShortcutString("c");
    setDefaultIncludedByDefault(false);
}

ClangFunctionsFilter::ClangFunctionsFilter()
    : ClangGlobalSymbolFilter(new CppFunctionsFilter, new LspFunctionsFilter)
{
    setId(CppEditor::Constants::FUNCTIONS_FILTER_ID);
    setDisplayName(::CppEditor::Tr::tr(CppEditor::Constants::FUNCTIONS_FILTER_DISPLAY_NAME));
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

    LocatorFilterEntry generateLocatorEntry(const DocumentSymbol &info,
                                            const LocatorFilterEntry &parent) override
    {
        LocatorFilterEntry entry;
        entry.filter = this;
        entry.displayName = ClangdClient::displayNameFromDocumentSymbol(
                    static_cast<SymbolKind>(info.kind()), info.name(),
                    info.detail().value_or(QString()));
        entry.internalData = QVariant::fromValue(info);
        entry.extraInfo = parent.extraInfo;
        if (!entry.extraInfo.isEmpty())
            entry.extraInfo.append("::");
        entry.extraInfo.append(parent.displayName);

        // TODO: Can we extend clangd to send visibility information?
        entry.displayIcon = LanguageClient::symbolIcon(info.kind());

        return entry;
    }

    // Filter out declarations for which a definition is also present.
    QList<LocatorFilterEntry> matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                         const QString &entry) override
    {
        QList<LocatorFilterEntry> allMatches
            = DocumentLocatorFilter::matchesFor(future, entry);
        QHash<QString, QList<LocatorFilterEntry>> possibleDuplicates;
        for (const LocatorFilterEntry &e : std::as_const(allMatches))
            possibleDuplicates[e.displayName + e.extraInfo] << e;
        const QTextDocument doc(m_content);
        for (auto it = possibleDuplicates.cbegin(); it != possibleDuplicates.cend(); ++it) {
            const QList<LocatorFilterEntry> &duplicates = it.value();
            if (duplicates.size() == 1)
                continue;
            QList<LocatorFilterEntry> declarations;
            QList<LocatorFilterEntry> definitions;
            for (const LocatorFilterEntry &candidate : duplicates) {
                const auto symbol = qvariant_cast<DocumentSymbol>(candidate.internalData);
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
                for (const LocatorFilterEntry &decl : std::as_const(declarations))
                    Utils::erase(allMatches, [&decl](const LocatorFilterEntry &e) {
                        return e.internalData == decl.internalData;
                    });
            }
        }

        // The base implementation expects the position in the internal data.
        for (LocatorFilterEntry &e : allMatches) {
            const Position pos = qvariant_cast<DocumentSymbol>(e.internalData).range().start();
            e.internalData = QVariant::fromValue(Utils::LineColumn(pos.line(), pos.character()));
        }

        return allMatches;
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
    setDefaultShortcutString(".");
    setPriority(High);
    setDefaultIncludedByDefault(false);
    setEnabled(false);
    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, [this](const IEditor *editor) { setEnabled(editor); });
}

ClangdCurrentDocumentFilter::~ClangdCurrentDocumentFilter() { delete d; }

void ClangdCurrentDocumentFilter::updateCurrentClient()
{
    d->lspFilter.updateCurrentClient();
}

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

void ClangdCurrentDocumentFilter::accept(const LocatorFilterEntry &selection, QString *newText,
                                         int *selectionStart, int *selectionLength) const
{
    QTC_ASSERT(d->activeFilter, return);
    d->activeFilter->accept(selection, newText, selectionStart, selectionLength);
}

} // namespace Internal
} // namespace ClangCodeModel

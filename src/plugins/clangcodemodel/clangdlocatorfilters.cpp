/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "clangdlocatorfilters.h"

#include "clangdclient.h"
#include "clangmodelmanagersupport.h"

#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cpplocatorfilter.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/indexitem.h>
#include <languageclient/languageclientutils.h>
#include <languageclient/locatorfilter.h>
#include <projectexplorer/session.h>
#include <utils/link.h>

#include <set>
#include <tuple>

using namespace LanguageServerProtocol;

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

class LspWorkspaceFilter : public LanguageClient::WorkspaceLocatorFilter
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

class LspClassesFilter : public LanguageClient::WorkspaceClassLocatorFilter
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

class LspFunctionsFilter : public LanguageClient::WorkspaceMethodLocatorFilter
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
                                                 ILocatorFilter *lspFilter)
    : m_cppFilter(cppFilter), m_lspFilter(lspFilter)
{
    setId(CppEditor::Constants::LOCATOR_FILTER_ID);
    setDisplayName(CppEditor::Constants::LOCATOR_FILTER_DISPLAY_NAME);
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
    QList<LanguageClient::Client *> clients;
    for (ProjectExplorer::Project * const project : ProjectExplorer::SessionManager::projects()) {
        LanguageClient::Client * const client
                = ClangModelManagerSupport::instance()->clientForProject(project);
        if (client)
            clients << client;
    }
    if (!clients.isEmpty()) {
        static_cast<LanguageClient::WorkspaceLocatorFilter *>(m_lspFilter)
            ->prepareSearch(entry, clients);
    }
}

QList<Core::LocatorFilterEntry> ClangGlobalSymbolFilter::matchesFor(
        QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    QList<Core::LocatorFilterEntry> matches = m_cppFilter->matchesFor(future, entry);
    const QList<Core::LocatorFilterEntry> lspMatches = m_lspFilter->matchesFor(future, entry);
    if (!lspMatches.isEmpty()) {
        std::set<std::tuple<Utils::FilePath, int, int>> locations;
        for (const auto &entry : qAsConst(matches)) {
            const CppEditor::IndexItem::Ptr item
                    = qvariant_cast<CppEditor::IndexItem::Ptr>(entry.internalData);
            locations.insert(std::make_tuple(Utils::FilePath::fromString(item->fileName()),
                                             item->line(),
                                             item->column()));
        }
        for (const auto &entry : lspMatches) {
            if (!entry.internalData.canConvert<Utils::Link>())
                continue;
            const auto link = qvariant_cast<Utils::Link>(entry.internalData);
            if (locations.find(std::make_tuple(link.targetFilePath, link.targetLine,
                                               link.targetColumn)) == locations.cend()) {
                matches << entry; // TODO: Insert sorted?
            }
        }
    }

    return matches;
}

void ClangGlobalSymbolFilter::accept(const Core::LocatorFilterEntry &selection, QString *newText,
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
    setDisplayName(CppEditor::Constants::CLASSES_FILTER_DISPLAY_NAME);
    setDefaultShortcutString("c");
    setDefaultIncludedByDefault(false);
}

ClangFunctionsFilter::ClangFunctionsFilter()
    : ClangGlobalSymbolFilter(new CppFunctionsFilter, new LspFunctionsFilter)
{
    setId(CppEditor::Constants::FUNCTIONS_FILTER_ID);
    setDisplayName(CppEditor::Constants::FUNCTIONS_FILTER_DISPLAY_NAME);
    setDefaultShortcutString("m");
    setDefaultIncludedByDefault(false);
}

class LspCurrentDocumentFilter : public LanguageClient::DocumentLocatorFilter
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
    Core::LocatorFilterEntry generateLocatorEntry(const DocumentSymbol &info,
                                                  const Core::LocatorFilterEntry &parent) override
    {
        Core::LocatorFilterEntry entry;
        entry.filter = this;
        entry.displayName = ClangdClient::displayNameFromDocumentSymbol(
                    static_cast<SymbolKind>(info.kind()), info.name(),
                    info.detail().value_or(QString()));
        const Position &pos = info.range().start();
        entry.internalData = QVariant::fromValue(Utils::LineColumn(pos.line(), pos.character()));
        entry.extraInfo = parent.extraInfo;
        if (!entry.extraInfo.isEmpty())
            entry.extraInfo.append("::");
        entry.extraInfo.append(parent.displayName);

        // TODO: Can we extend clangd to send visibility information?
        entry.displayIcon = LanguageClient::symbolIcon(info.kind());

        return entry;
    }
};

class ClangdCurrentDocumentFilter::Private
{
public:
    ~Private() { delete cppFilter; }

    Core::ILocatorFilter * const cppFilter
            = CppEditor::CppModelManager::createAuxiliaryCurrentDocumentFilter();
    LspCurrentDocumentFilter lspFilter;
    Core::ILocatorFilter *activeFilter = nullptr;
};


ClangdCurrentDocumentFilter::ClangdCurrentDocumentFilter() : d(new Private)
{
    setId(CppEditor::Constants::CURRENT_DOCUMENT_FILTER_ID);
    setDisplayName(CppEditor::Constants::CURRENT_DOCUMENT_FILTER_DISPLAY_NAME);
    setDefaultShortcutString(".");
    setPriority(High);
    setDefaultIncludedByDefault(false);
    setEnabled(false);
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, [this](const Core::IEditor *editor) { setEnabled(editor); });
}

ClangdCurrentDocumentFilter::~ClangdCurrentDocumentFilter() { delete d; }

void ClangdCurrentDocumentFilter::prepareSearch(const QString &entry)
{
    const auto doc = TextEditor::TextDocument::currentTextDocument();
    QTC_ASSERT(doc, return);
    if (const ClangdClient * const client = ClangModelManagerSupport::instance()
            ->clientForFile(doc->filePath()); client && client->reachable()) {
        d->activeFilter = &d->lspFilter;
    } else {
        d->activeFilter = d->cppFilter;
    }
    d->activeFilter->prepareSearch(entry);
}

QList<Core::LocatorFilterEntry> ClangdCurrentDocumentFilter::matchesFor(
        QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    QTC_ASSERT(d->activeFilter, return {});
    return d->activeFilter->matchesFor(future, entry);
}

void ClangdCurrentDocumentFilter::accept(const Core::LocatorFilterEntry &selection, QString *newText,
                                         int *selectionStart, int *selectionLength) const
{
    QTC_ASSERT(d->activeFilter, return);
    d->activeFilter->accept(selection, newText, selectionStart, selectionLength);
}

} // namespace Internal
} // namespace ClangCodeModel

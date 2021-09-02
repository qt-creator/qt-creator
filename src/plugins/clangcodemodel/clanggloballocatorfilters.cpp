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

#include "clanggloballocatorfilters.h"

#include "clangdclient.h"
#include "clangmodelmanagersupport.h"

#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cpplocatorfilter.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/indexitem.h>
#include <languageclient/locatorfilter.h>
#include <projectexplorer/session.h>
#include <utils/link.h>

#include <set>
#include <tuple>

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
    QVector<LanguageClient::Client *> clients;
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

void ClangGlobalSymbolFilter::accept(Core::LocatorFilterEntry selection, QString *newText,
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

} // namespace Internal
} // namespace ClangCodeModel

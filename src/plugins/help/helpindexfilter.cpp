// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "helpindexfilter.h"

#include "localhelpmanager.h"
#include "helpmanager.h"
#include "helpplugin.h"
#include "helptr.h"

#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/async.h>
#include <utils/utilsicons.h>

#include <QHelpEngine>
#include <QHelpFilterEngine>
#include <QHelpLink>

using namespace Core;
using namespace Help;
using namespace Help::Internal;
using namespace Tasking;
using namespace Utils;

HelpIndexFilter::HelpIndexFilter()
{
    setId("HelpIndexFilter");
    setDisplayName(Tr::tr("Help Index"));
    setDescription(Tr::tr("Locates help topics, for example in the Qt documentation."));
    setDefaultShortcutString("?");
    setRefreshRecipe(Sync([this] { invalidateCache(); }));

    m_icon = Utils::Icons::BOOKMARK.icon();
    connect(Core::HelpManager::Signals::instance(), &Core::HelpManager::Signals::setupFinished,
            this, &HelpIndexFilter::invalidateCache);
    connect(Core::HelpManager::Signals::instance(),
            &Core::HelpManager::Signals::documentationChanged,
            this,
            &HelpIndexFilter::invalidateCache);
    connect(HelpManager::instance(), &HelpManager::collectionFileChanged,
            this, &HelpIndexFilter::invalidateCache);
}

static void matches(QPromise<QStringList> &promise, const LocatorStorage &storage,
                    const QStringList &cache, const QIcon &icon)
{
    const QString input = storage.input();
    const Qt::CaseSensitivity cs = ILocatorFilter::caseSensitivity(input);
    QStringList bestKeywords;
    QStringList worseKeywords;
    bestKeywords.reserve(cache.size());
    worseKeywords.reserve(cache.size());
    for (const QString &keyword : cache) {
        if (promise.isCanceled())
            return;
        if (keyword.startsWith(input, cs))
            bestKeywords.append(keyword);
        else if (keyword.contains(input, cs))
            worseKeywords.append(keyword);
    }
    const QStringList lastIndicesCache = bestKeywords + worseKeywords;

    LocatorFilterEntries entries;
    for (const QString &key : lastIndicesCache) {
        if (promise.isCanceled())
            return;
        const int index = key.indexOf(input, 0, cs);
        LocatorFilterEntry filterEntry;
        filterEntry.displayName = key;
        filterEntry.acceptor = [key] {
            HelpPlugin::showLinksInCurrentViewer(LocalHelpManager::linksForKeyword(key), key);
            return AcceptResult();
        };
        filterEntry.displayIcon = icon;
        filterEntry.highlightInfo = {index, int(input.length())};
        entries.append(filterEntry);
    }
    storage.reportOutput(entries);
    promise.addResult(lastIndicesCache);
}

LocatorMatcherTasks HelpIndexFilter::matchers()
{
    TreeStorage<LocatorStorage> storage;

    const auto onSetup = [this, storage](Async<QStringList> &async) {
        if (m_needsUpdate) {
            m_needsUpdate = false;
            LocalHelpManager::setupGuiHelpEngine();
            m_allIndicesCache = LocalHelpManager::filterEngine()->indices({});
            m_lastIndicesCache.clear();
            m_lastEntry.clear();
        }
        const QStringList cache = m_lastEntry.isEmpty() || !storage->input().contains(m_lastEntry)
                                      ? m_allIndicesCache : m_lastIndicesCache;
        async.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        async.setConcurrentCallData(matches, *storage, cache, m_icon);
    };
    const auto onDone = [this, storage](const Async<QStringList> &async) {
        if (async.isResultAvailable()) {
            m_lastIndicesCache = async.result();
            m_lastEntry = storage->input();
        }
    };

    return {{AsyncTask<QStringList>(onSetup, onDone), storage}};
}

void HelpIndexFilter::invalidateCache()
{
    m_needsUpdate = true;
}

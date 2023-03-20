// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "helpindexfilter.h"

#include "helpmanager.h"
#include "helptr.h"
#include "localhelpmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/utilsicons.h>
#include <utils/tasktree.h>

#include <QHelpEngine>
#include <QHelpFilterEngine>
#include <QHelpLink>

using namespace Core;
using namespace Help;
using namespace Help::Internal;

HelpIndexFilter::HelpIndexFilter()
{
    setId("HelpIndexFilter");
    setDisplayName(Tr::tr("Help Index"));
    setDefaultIncludedByDefault(false);
    setDefaultShortcutString("?");
    setRefreshRecipe(Utils::Tasking::Sync([this] { invalidateCache(); return true; }));

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

void HelpIndexFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    if (!m_needsUpdate)
        return;

    m_needsUpdate = false;
    LocalHelpManager::setupGuiHelpEngine();
    m_allIndicesCache = LocalHelpManager::filterEngine()->indices({});
    m_lastIndicesCache.clear();
    m_lastEntry.clear();
}

QList<LocatorFilterEntry> HelpIndexFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                                      const QString &entry)
{
    const QStringList cache = m_lastEntry.isEmpty() || !entry.contains(m_lastEntry)
                                  ? m_allIndicesCache : m_lastIndicesCache;

    const Qt::CaseSensitivity cs = caseSensitivity(entry);
    QStringList bestKeywords;
    QStringList worseKeywords;
    bestKeywords.reserve(cache.size());
    worseKeywords.reserve(cache.size());
    for (const QString &keyword : cache) {
        if (future.isCanceled())
            return {};
        if (keyword.startsWith(entry, cs))
            bestKeywords.append(keyword);
        else if (keyword.contains(entry, cs))
            worseKeywords.append(keyword);
    }
    m_lastIndicesCache = bestKeywords + worseKeywords;
    m_lastEntry = entry;

    QList<LocatorFilterEntry> entries;
    for (const QString &keyword : std::as_const(m_lastIndicesCache)) {
        const int index = keyword.indexOf(entry, 0, cs);
        LocatorFilterEntry filterEntry(this, keyword);
        filterEntry.displayIcon = m_icon;
        filterEntry.highlightInfo = {index, int(entry.length())};
        entries.append(filterEntry);
    }
    return entries;
}

void HelpIndexFilter::accept(const LocatorFilterEntry &selection,
                             QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    const QString &key = selection.displayName;
    const QMultiMap<QString, QUrl> links = LocalHelpManager::linksForKeyword(key);
    emit linksActivated(links, key);
}

void HelpIndexFilter::invalidateCache()
{
    m_needsUpdate = true;
}

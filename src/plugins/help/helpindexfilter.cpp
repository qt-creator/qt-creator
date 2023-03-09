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
#include <QIcon>

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

HelpIndexFilter::~HelpIndexFilter() = default;

bool HelpIndexFilter::updateCache(QFutureInterface<LocatorFilterEntry> &future,
                                  const QStringList &cache, const QString &entry)
{
    const Qt::CaseSensitivity cs = caseSensitivity(entry);
    QStringList bestKeywords;
    QStringList worseKeywords;
    bestKeywords.reserve(cache.size());
    worseKeywords.reserve(cache.size());
    for (const QString &keyword : cache) {
        if (future.isCanceled())
            return false;
        if (keyword.startsWith(entry, cs))
            bestKeywords.append(keyword);
        else if (keyword.contains(entry, cs))
            worseKeywords.append(keyword);
    }
    bestKeywords << worseKeywords;
    m_lastIndicesCache = bestKeywords;
    m_lastEntry = entry;

    return true;
}

QList<LocatorFilterEntry> HelpIndexFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &entry)
{
    if (m_needsUpdate.exchange(false)) {
        QStringList indices;
        QMetaObject::invokeMethod(this, [this] { return allIndices(); },
                                  Qt::BlockingQueuedConnection, &indices);
        m_allIndicesCache = indices;
        // force updating the cache taking the m_allIndicesCache
        m_lastIndicesCache = QStringList();
        m_lastEntry = QString();
    }

    const QStringList cacheBase = m_lastEntry.isEmpty() || !entry.contains(m_lastEntry)
            ? m_allIndicesCache : m_lastIndicesCache;

    if (!updateCache(future, cacheBase, entry))
        return QList<LocatorFilterEntry>();

    const Qt::CaseSensitivity cs = caseSensitivity(entry);
    QList<LocatorFilterEntry> entries;
    for (const QString &keyword : std::as_const(m_lastIndicesCache)) {
        const int index = keyword.indexOf(entry, 0, cs);
        LocatorFilterEntry filterEntry(this, keyword, {}, m_icon);
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

QStringList HelpIndexFilter::allIndices() const
{
    LocalHelpManager::setupGuiHelpEngine();
    return LocalHelpManager::filterEngine()->indices(QString());
}

void HelpIndexFilter::invalidateCache()
{
    m_needsUpdate = true;
}

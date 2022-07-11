/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "helpindexfilter.h"

#include "helpicons.h"
#include "helpmanager.h"
#include "helptr.h"
#include "topicchooser.h"

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/utilsicons.h>

#include <QIcon>

#include "localhelpmanager.h"
#include <QtHelp/QHelpEngine>
#include <QtHelp/QHelpFilterEngine>
#include <QtHelp/QHelpLink>

using namespace Core;
using namespace Help;
using namespace Help::Internal;

HelpIndexFilter::HelpIndexFilter()
{
    setId("HelpIndexFilter");
    setDisplayName(Tr::tr("Help Index"));
    setDefaultIncludedByDefault(false);
    setDefaultShortcutString("?");

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
    for (const QString &keyword : qAsConst(m_lastIndicesCache)) {
        const int index = keyword.indexOf(entry, 0, cs);
        LocatorFilterEntry filterEntry(this, keyword, QVariant(), m_icon);
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
    QMultiMap<QString, QUrl> links;
    const QList<QHelpLink> docs = LocalHelpManager::helpEngine().documentsForKeyword(key, QString());
    for (const auto &doc : docs)
        links.insert(doc.title, doc.url);
    emit linksActivated(links, key);
}

void HelpIndexFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    invalidateCache();
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

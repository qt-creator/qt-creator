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

#include "centralwidget.h"
#include "helpicons.h"

#include <topicchooser.h>

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <utils/algorithm.h>
#include <utils/utilsicons.h>

#include <QIcon>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>


using namespace Core;
using namespace Help;
using namespace Help::Internal;

HelpIndexFilter::HelpIndexFilter()
    : m_needsUpdate(true)
{
    setId("HelpIndexFilter");
    setDisplayName(tr("Help Index"));
    setIncludedByDefault(false);
    setShortcutString(QString(QLatin1Char('?')));

    m_icon = Utils::Icons::BOOKMARK.icon();
    connect(HelpManager::instance(), &HelpManager::setupFinished,
            this, &HelpIndexFilter::invalidateCache);
    connect(HelpManager::instance(), &HelpManager::documentationChanged,
            this, &HelpIndexFilter::invalidateCache);
    connect(HelpManager::instance(), &HelpManager::collectionFileChanged,
            this, &HelpIndexFilter::invalidateCache);
}

HelpIndexFilter::~HelpIndexFilter()
{
}

void HelpIndexFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    QStringList namespaces = HelpManager::registeredNamespaces();
    m_helpDatabases = Utils::transform(namespaces, [](const QString &ns) {
        return HelpManager::fileFromNamespace(ns);
    });
}

QList<LocatorFilterEntry> HelpIndexFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &entry)
{
    m_mutex.lock(); // guard m_needsUpdate
    bool forceUpdate = m_needsUpdate;
    m_mutex.unlock();

    if (forceUpdate || m_searchTermCache.size() < 2 || m_searchTermCache.isEmpty()
            || !entry.contains(m_searchTermCache)) {
        int limit = entry.size() < 2 ? 200 : INT_MAX;
        QSet<QString> results;
        foreach (const QString &filePath, m_helpDatabases) {
            if (future.isCanceled())
                return QList<LocatorFilterEntry>();
            QSet<QString> result;
            QMetaObject::invokeMethod(this, "searchMatches", Qt::BlockingQueuedConnection,
                                      Q_RETURN_ARG(QSet<QString>, result),
                                      Q_ARG(QString, filePath),
                                      Q_ARG(QString, entry),
                                      Q_ARG(int, limit));
            results.unite(result);
        }
        m_mutex.lock(); // guard m_needsUpdate
        m_needsUpdate = false;
        m_mutex.unlock();
        m_keywordCache = results;
        m_searchTermCache = entry;
    }

    Qt::CaseSensitivity cs = caseSensitivity(entry);
    QList<LocatorFilterEntry> entries;
    QStringList keywords;
    QStringList unsortedKeywords;
    keywords.reserve(m_keywordCache.size());
    unsortedKeywords.reserve(m_keywordCache.size());
    QSet<QString> allresults;
    foreach (const QString &keyword, m_keywordCache) {
        if (future.isCanceled())
            return QList<LocatorFilterEntry>();
        if (keyword.startsWith(entry, cs)) {
            keywords.append(keyword);
            allresults.insert(keyword);
        } else if (keyword.contains(entry, cs)) {
            unsortedKeywords.append(keyword);
            allresults.insert(keyword);
        }
    }
    Utils::sort(keywords);
    keywords << unsortedKeywords;
    m_keywordCache = allresults;
    m_searchTermCache = entry;
    foreach (const QString &keyword, keywords) {
        const int index = keyword.indexOf(entry, 0, cs);
        LocatorFilterEntry filterEntry(this, keyword, QVariant(), m_icon);
        filterEntry.highlightInfo = {index, entry.length()};
        entries.append(filterEntry);
    }

    return entries;
}

void HelpIndexFilter::accept(LocatorFilterEntry selection,
                             QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    const QString &key = selection.displayName;
    const QMap<QString, QUrl> &links = HelpManager::linksForKeyword(key);
    emit linksActivated(links, key);
}

void HelpIndexFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    invalidateCache();
}

QSet<QString> HelpIndexFilter::searchMatches(const QString &databaseFilePath,
                                           const QString &term, int limit)
{
    static const QLatin1String sqlite("QSQLITE");
    static const QLatin1String name("HelpManager::findKeywords");

    QSet<QString> keywords;

    { // make sure db is destroyed before removeDatabase call
        QSqlDatabase db = QSqlDatabase::addDatabase(sqlite, name);
        if (db.driver() && db.driver()->lastError().type() == QSqlError::NoError) {
            db.setDatabaseName(databaseFilePath);
            if (db.open()) {
                QSqlQuery query = QSqlQuery(db);
                query.setForwardOnly(true);
                query.exec(QString::fromLatin1("SELECT DISTINCT Name FROM IndexTable WHERE Name LIKE "
                    "'%%1%' LIMIT %2").arg(term, QString::number(limit)));
                while (query.next()) {
                    const QString &keyValue = query.value(0).toString();
                    if (!keyValue.isEmpty())
                        keywords.insert(keyValue);
                }
                db.close();
            }
        }
    }
    QSqlDatabase::removeDatabase(name);
    return keywords;
}

void HelpIndexFilter::invalidateCache()
{
    m_mutex.lock();
    m_needsUpdate = true;
    m_mutex.unlock();
}

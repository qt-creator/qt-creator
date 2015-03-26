/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "spotlightlocatorfilter.h"

#include <utils/autoreleasepool.h>
#include <utils/qtcassert.h>

#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>

#include <Foundation/NSArray.h>
#include <Foundation/NSAutoreleasePool.h>
#include <Foundation/NSDictionary.h>
#include <Foundation/NSMetadata.h>
#include <Foundation/NSNotification.h>
#include <Foundation/NSOperation.h>
#include <Foundation/NSPredicate.h>

namespace Core {
namespace Internal {

// #pragma mark -- SpotlightIterator

class SpotlightIterator : public BaseFileFilter::Iterator
{
public:
    SpotlightIterator(const QString &expression);
    ~SpotlightIterator();

    void toFront();
    bool hasNext() const;
    QString next();
    QString filePath() const;
    QString fileName() const;

private:
    void ensureNext();

    NSMetadataQuery *m_query;
    id<NSObject> m_progressObserver;
    id<NSObject> m_finishObserver;
    QMutex m_mutex;
    QWaitCondition m_waitForItems;
    NSMutableArray *m_queue;
    QStringList m_filePaths;
    QStringList m_fileNames;
    int m_index;
    unsigned int m_queueIndex;
    bool m_finished;
};

SpotlightIterator::SpotlightIterator(const QString &expression)
    : m_index(-1),
      m_queueIndex(-1),
      m_finished(false)
{
    Utils::AutoreleasePool pool; Q_UNUSED(pool)
    NSPredicate *predicate = [NSPredicate predicateWithFormat:expression.toNSString()];
    m_query = [[NSMetadataQuery alloc] init];
    m_query.predicate = predicate;
    m_query.searchScopes = [NSArray arrayWithObject:NSMetadataQueryLocalComputerScope];
    m_queue = [[NSMutableArray alloc] init];
    m_progressObserver = [[[NSNotificationCenter defaultCenter]
            addObserverForName:NSMetadataQueryGatheringProgressNotification
                        object:m_query
                         queue:nil
                    usingBlock:^(NSNotification *note) {
                        [m_query disableUpdates];
                        QMutexLocker lock(&m_mutex); Q_UNUSED(lock)
                        [m_queue addObjectsFromArray:[note.userInfo objectForKey:NSMetadataQueryUpdateAddedItemsKey]];
                        [m_query enableUpdates];
                        m_waitForItems.wakeAll();
                    }] retain];
    m_finishObserver = [[[NSNotificationCenter defaultCenter]
            addObserverForName:NSMetadataQueryDidFinishGatheringNotification
                        object:m_query
                         queue:nil
                    usingBlock:^(NSNotification *) {
                        QMutexLocker lock(&m_mutex); Q_UNUSED(lock)
                        m_finished = true;
                        m_waitForItems.wakeAll();
                    }] retain];
    [m_query startQuery];
}

SpotlightIterator::~SpotlightIterator()
{
    [[NSNotificationCenter defaultCenter] removeObserver:m_progressObserver];
    [m_progressObserver release];
    [[NSNotificationCenter defaultCenter] removeObserver:m_finishObserver];
    [m_finishObserver release];
    [m_query stopQuery];
    [m_query release];
    [m_queue release];
}

void SpotlightIterator::toFront()
{
    m_index = -1;
}

bool SpotlightIterator::hasNext() const
{
    auto that = const_cast<SpotlightIterator *>(this);
    that->ensureNext();
    return (m_index + 1 < m_filePaths.size());
}

QString SpotlightIterator::next()
{
    ensureNext();
    ++m_index;
    QTC_ASSERT(m_index < m_filePaths.size(), return QString());
    return m_filePaths.at(m_index);
}

QString SpotlightIterator::filePath() const
{
    QTC_ASSERT(m_index < m_filePaths.size(), return QString());
    return m_filePaths.at(m_index);
}

QString SpotlightIterator::fileName() const
{
    QTC_ASSERT(m_index < m_fileNames.size(), return QString());
    return m_fileNames.at(m_index);
}

void SpotlightIterator::ensureNext()
{
    if (m_index + 1 < m_filePaths.size()) // nothing to do
        return;
    if (m_index >= 10000) // limit the amount of data that is passed on
        return;
    Utils::AutoreleasePool pool; Q_UNUSED(pool)
    // check if there are items in the queue, otherwise wait for some
    m_mutex.lock();
    bool itemAvailable = (m_queueIndex + 1 < m_queue.count);
    if (!itemAvailable && !m_finished) {
        m_waitForItems.wait(&m_mutex);
        itemAvailable = (m_queueIndex + 1 < m_queue.count);
    }
    if (itemAvailable) {
        ++m_queueIndex;
        NSMetadataItem *item = [m_queue objectAtIndex:m_queueIndex];
        m_filePaths.append(QString::fromNSString([item valueForAttribute:NSMetadataItemPathKey]));
        m_fileNames.append(QString::fromNSString([item valueForAttribute:NSMetadataItemFSNameKey]));

    }
    m_mutex.unlock();
}

// #pragma mark -- SpotlightLocatorFilter

SpotlightLocatorFilter::SpotlightLocatorFilter()
{
    setId("SpotlightFileNamesLocatorFilter");
    setDisplayName(tr("Spotlight File Name Index"));
    setShortcutString(QLatin1String("md"));
}

void SpotlightLocatorFilter::prepareSearch(const QString &entry)
{
    if (entry.isEmpty()) {
        setFileIterator(new BaseFileFilter::ListIterator(QStringList()));
    } else {
        // only pass the file name part to spotlight to allow searches like "somepath/*foo"
        int lastSlash = entry.lastIndexOf(QLatin1Char('/'));
        QString quoted = entry.mid(lastSlash + 1);
        quoted.replace(QLatin1Char('\\'), QLatin1String("\\\\"))
            .replace(QLatin1Char('\''), QLatin1String("\\\'"))
            .replace(QLatin1Char('\"'), QLatin1String("\\\""));
        setFileIterator(new SpotlightIterator(
                            QString::fromLatin1("kMDItemFSName like%1 \"*%2*\"")
                            .arg(caseSensitivity(entry) == Qt::CaseInsensitive ? QLatin1String("[c]")
                                                                               : QString())
                            .arg(quoted)));
    }
    BaseFileFilter::prepareSearch(entry);
}

void SpotlightLocatorFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
}

} // Internal
} // Core

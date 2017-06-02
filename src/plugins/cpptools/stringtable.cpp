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

#include "stringtable.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QThreadPool>
#include <QTime>

using namespace CppTools::Internal;

enum {
    GCTimeOut = 10 * 1000 // 10 seconds
};

StringTable::StringTable()
    : m_gcRunner(*this)
    , m_stopGCRequested(false)
{
    m_strings.reserve(1000);

    m_gcRunner.setAutoDelete(false);

    m_gcCountDown.setObjectName(QLatin1String("StringTable::m_gcCountDown"));
    m_gcCountDown.setSingleShot(true);
    m_gcCountDown.setInterval(GCTimeOut);
    connect(&m_gcCountDown, &QTimer::timeout, this, &StringTable::startGC);
}

QString StringTable::insert(const QString &string)
{
    if (string.isEmpty())
        return string;

#ifndef QT_NO_UNSHARABLE_CONTAINERS
    QTC_ASSERT(const_cast<QString&>(string).data_ptr()->ref.isSharable(), return string);
#endif

    m_stopGCRequested.fetchAndStoreAcquire(true);

    QMutexLocker locker(&m_lock);
    QString result = *m_strings.insert(string);
    m_stopGCRequested.fetchAndStoreRelease(false);
    return result;
}

void StringTable::scheduleGC()
{
    QMetaObject::invokeMethod(&m_gcCountDown, "start", Qt::QueuedConnection);
}

void StringTable::startGC()
{
    QThreadPool::globalInstance()->start(&m_gcRunner);
}

enum {
    DebugStringTable = 0
};

static inline bool isQStringInUse(const QString &string)
{
    QArrayData *data_ptr = const_cast<QString&>(string).data_ptr();
    return data_ptr->ref.isShared() || data_ptr->ref.isStatic();
}

void StringTable::GC()
{
    QMutexLocker locker(&m_lock);

    int initialSize = 0;
    QTime startTime;
    if (DebugStringTable) {
        initialSize = m_strings.size();
        startTime = QTime::currentTime();
    }

    // Collect all QStrings which have refcount 1. (One reference in m_strings and nowhere else.)
    for (QSet<QString>::iterator i = m_strings.begin(); i != m_strings.end();) {
        if (m_stopGCRequested.testAndSetRelease(true, false))
            return;

        if (!isQStringInUse(*i))
            i = m_strings.erase(i);
        else
            ++i;
    }

    if (DebugStringTable) {
        const int currentSize = m_strings.size();
        qDebug() << "StringTable::GC removed" << initialSize - currentSize
                 << "strings in" << startTime.msecsTo(QTime::currentTime())
                 << "ms, size is now" << currentSize;
    }
}

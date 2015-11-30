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

#if QT_SUPPORTS(UNSHARABLE_CONTAINERS)
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

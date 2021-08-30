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
#include <utils/runextensions.h>

#include <QDebug>
#include <QElapsedTimer>
#include <QMutex>
#include <QSet>
#include <QThreadPool>
#include <QTimer>

#ifdef WITH_TESTS
#include <extensionsystem/pluginmanager.h>
#endif

namespace CppEditor::Internal {

enum {
    GCTimeOut = 10 * 1000 // 10 seconds
};

enum {
    DebugStringTable = 0
};

class StringTablePrivate : public QObject
{
public:
    StringTablePrivate();
    ~StringTablePrivate() override { cancelAndWait(); }

    void cancelAndWait();
    QString insert(const QString &string);
    void startGC();
    void GC(QFutureInterface<void> &futureInterface);

    QFuture<void> m_future;
    QMutex m_lock;
    QSet<QString> m_strings;
    QTimer m_gcCountDown;
};

static StringTablePrivate *m_instance = nullptr;

StringTablePrivate::StringTablePrivate()
{
    m_strings.reserve(1000);

    m_gcCountDown.setObjectName(QLatin1String("StringTable::m_gcCountDown"));
    m_gcCountDown.setSingleShot(true);
    m_gcCountDown.setInterval(GCTimeOut);
    connect(&m_gcCountDown, &QTimer::timeout, this, &StringTablePrivate::startGC);
}

QString StringTable::insert(const QString &string)
{
    return m_instance->insert(string);
}

void StringTablePrivate::cancelAndWait()
{
    if (!m_future.isRunning())
        return;
    m_future.cancel();
    m_future.waitForFinished();
}

QString StringTablePrivate::insert(const QString &string)
{
    if (string.isEmpty())
        return string;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#ifndef QT_NO_UNSHARABLE_CONTAINERS
    QTC_ASSERT(const_cast<QString&>(string).data_ptr()->ref.isSharable(), return string);
#endif
#endif

    QMutexLocker locker(&m_lock);
    // From this point of time any possible new call to startGC() will be held until
    // we finish this function. So we are sure that after canceling the running GC() method now,
    // no new call to GC() will be executed until we finish this function.
    cancelAndWait();
    // A possibly running GC() thread already finished, so it's safe to modify m_strings from
    // now until we unlock the mutex.
    return *m_strings.insert(string);
}

void StringTablePrivate::startGC()
{
    QMutexLocker locker(&m_lock);
    cancelAndWait();
    m_future = Utils::runAsync(&StringTablePrivate::GC, this);
}

void StringTable::scheduleGC()
{
    QMetaObject::invokeMethod(&m_instance->m_gcCountDown, QOverload<>::of(&QTimer::start),
                              Qt::QueuedConnection);
}

StringTable::StringTable()
{
    m_instance = new StringTablePrivate;
}

StringTable::~StringTable()
{
    delete m_instance;
    m_instance = nullptr;
}

static inline bool isQStringInUse(const QString &string)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto data_ptr = const_cast<QString&>(string).data_ptr();
    return data_ptr->ref.isShared() || data_ptr->ref.isStatic() /* QStringLiteral ? */;
#else
    auto data_ptr = const_cast<QString&>(string).data_ptr();
    return data_ptr->isShared() || !data_ptr->isMutable() /* QStringLiteral ? */;
#endif
}

void StringTablePrivate::GC(QFutureInterface<void> &futureInterface)
{
#ifdef WITH_TESTS
    if (ExtensionSystem::PluginManager::isScenarioRunning("TestStringTable")) {
        if (ExtensionSystem::PluginManager::finishScenario())
            QThread::currentThread()->sleep(5);
    }
#endif

    int initialSize = 0;
    QElapsedTimer timer;
    if (DebugStringTable) {
        initialSize = m_strings.size();
        timer.start();
    }

    // Collect all QStrings which have refcount 1. (One reference in m_strings and nowhere else.)
    for (QSet<QString>::iterator i = m_strings.begin(); i != m_strings.end();) {
        if (futureInterface.isCanceled())
            return;

        if (!isQStringInUse(*i))
            i = m_strings.erase(i);
        else
            ++i;
    }

    if (DebugStringTable) {
        const int currentSize = m_strings.size();
        qDebug() << "StringTable::GC removed" << initialSize - currentSize
                 << "strings in" << timer.elapsed() << "ms, size is now" << currentSize;
    }
}

} // CppEditor::Internal

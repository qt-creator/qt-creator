// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stringtable.h"

#include "async.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QMutex>
#include <QSet>
#include <QTimer>

// FIXME: Provide tst_StringTable that would run GC, make a sleep inside GC being run in other
// thread and execute destructor of StringTable in main thread. In this case the test should
// ensure that destructor of StringTable waits for its internal thread to finish.

namespace Utils::StringTable {

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
    void GC(QPromise<void> &promise);

    QFuture<void> m_future;
    QMutex m_lock;
    QSet<QString> m_strings;
    QTimer m_gcCountDown;
};

static StringTablePrivate &stringTable()
{
    static StringTablePrivate theStringTable;
    return theStringTable;
}

StringTablePrivate::StringTablePrivate()
{
    m_strings.reserve(1000);

    m_gcCountDown.setObjectName(QLatin1String("StringTable::m_gcCountDown"));
    m_gcCountDown.setSingleShot(true);
    m_gcCountDown.setInterval(GCTimeOut);
    connect(&m_gcCountDown, &QTimer::timeout, this, &StringTablePrivate::startGC);
}

QTCREATOR_UTILS_EXPORT QString insert(const QString &string)
{
    return stringTable().insert(string);
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
    m_future = Utils::asyncRun(&StringTablePrivate::GC, this);
}

QTCREATOR_UTILS_EXPORT void scheduleGC()
{
    QMetaObject::invokeMethod(&stringTable().m_gcCountDown, QOverload<>::of(&QTimer::start),
                              Qt::QueuedConnection);
}

static int bytesSaved = 0;

static inline bool isQStringInUse(const QString &string)
{
    QStringPrivate data_ptr = const_cast<QString&>(string).data_ptr();
    if (DebugStringTable) {
        const int ref = data_ptr->d_ptr()->ref_;
        bytesSaved += (ref - 1) * string.size();
        if (ref > 10)
            qDebug() << ref << string.size() << string.left(50);
    }
    return data_ptr->isShared() || !data_ptr->isMutable() /* QStringLiteral ? */;
}

void StringTablePrivate::GC(QPromise<void> &promise)
{
    int initialSize = 0;
    bytesSaved = 0;
    QElapsedTimer timer;
    if (DebugStringTable) {
        initialSize = m_strings.size();
        timer.start();
    }

    // Collect all QStrings which have refcount 1. (One reference in m_strings and nowhere else.)
    for (QSet<QString>::iterator i = m_strings.begin(); i != m_strings.end();) {
        if (promise.isCanceled())
            return;

        if (!isQStringInUse(*i))
            i = m_strings.erase(i);
        else
            ++i;
    }

    if (DebugStringTable) {
        const int currentSize = m_strings.size();
        qDebug() << "StringTable::GC removed" << initialSize - currentSize
                 << "strings in" << timer.elapsed() << "ms, size is now" << currentSize
                 << "saved: " << bytesSaved << "bytes";
    }
}

} // Utils::StringTable

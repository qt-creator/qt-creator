// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtcassert.h"
#include "singleton.h"
#include "threadutils.h"

#include <QList>

#include <unordered_map>

using namespace Utils;

namespace Utils {

// The order of elements reflects dependencies, i.e.
// if B requires A then B will follow A on this list
static QList<Singleton *> s_singletonList;
static QMutex s_mutex;
static std::unordered_map<std::type_index, SingletonStaticData> s_staticDataList;

Singleton::~Singleton()
{
    QMutexLocker locker(&s_mutex);
    s_singletonList.removeAll(this);
}

void Singleton::addSingleton(Singleton *singleton)
{
    QMutexLocker locker(&s_mutex);
    s_singletonList.append(singleton);
}

SingletonStaticData &Singleton::staticData(std::type_index index)
{
    QMutexLocker locker(&s_mutex);
    return s_staticDataList[index];
}

// Note: it's caller responsibility to ensure that this function is being called when all other
// threads don't use any singleton. As a good practice: finish all other threads that were using
// singletons before this function is called.
// Some singletons may work only in main thread, so this method should be called from main thread
// only.
void Singleton::deleteAll()
{
    QTC_ASSERT(isMainThread(), return);
    QList<Singleton *> oldList;
    {
        QMutexLocker locker(&s_mutex);
        oldList = s_singletonList;
        s_singletonList = {};
    }
    // Keep the reverse order when deleting
    while (!oldList.isEmpty())
        delete oldList.takeLast();
}

} // namespace Utils

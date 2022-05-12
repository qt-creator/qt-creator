/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qtcassert.h"
#include "singleton.h"

#include <QCoreApplication>
#include <QList>
#include <QThread>

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
    QTC_ASSERT(QThread::currentThread() == qApp->thread(), return);
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

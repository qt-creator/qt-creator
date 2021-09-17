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

#pragma once

#include "utils_global.h"

#include <QMutex>
#include <QMutexLocker>

#include <type_traits>
#include <typeindex>

namespace Utils {

class Singleton;

struct SingletonStaticData
{
    Singleton *m_instance = nullptr;
    QMutex m_mutex;
};

class QTCREATOR_UTILS_EXPORT Singleton
{
    Q_DISABLE_COPY_MOVE(Singleton)
public:
    static void deleteAll();

private:
    template <typename SingletonSubClass, typename ...Dependencies>
    friend class SingletonWithOptionalDependencies;

    Singleton() = default;
    virtual ~Singleton();
    static void addSingleton(Singleton *singleton);
    static SingletonStaticData &staticData(std::type_index index);
};

template <typename SingletonSubClass, typename ...Dependencies>
class SingletonWithOptionalDependencies : public Singleton
{
public:
    Q_DISABLE_COPY_MOVE(SingletonWithOptionalDependencies)
    static SingletonSubClass *instance()
    {
        SingletonStaticData &data = staticData();
        QMutexLocker locker(&data.m_mutex);
        if (data.m_instance == nullptr) {
            // instantiate all dependencies first
            if constexpr (sizeof...(Dependencies))
                instantiateDependencies<Dependencies...>();
            data.m_instance = new SingletonSubClass;
            // put instance into static list of registered instances
            addSingleton(data.m_instance);
        }
        return static_cast<SingletonSubClass *>(data.m_instance);
    }

protected:
    SingletonWithOptionalDependencies() = default;
    ~SingletonWithOptionalDependencies() override
    {
        SingletonStaticData &data = staticData();
        QMutexLocker locker(&data.m_mutex);
        if (data.m_instance == this)
            data.m_instance = nullptr;
    }

private:
    template <typename ...Dependency> static void instantiateDependencies()
    {
        static_assert ((... && std::is_base_of_v<Singleton, Dependency>),
                "All Dependencies must derive from SingletonWithOptionalDependencies class.");
        (..., Dependency::instance());
    }
    static SingletonStaticData &staticData()
    {
        static SingletonStaticData &data = Singleton::staticData(std::type_index(typeid(SingletonSubClass)));
        return data;
    }
};

} // namespace Utils

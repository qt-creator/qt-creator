// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "aggregation_global.h"

#include <QObject>
#include <QList>
#include <QHash>
#include <QReadWriteLock>
#include <QReadLocker>

namespace Aggregation {

class AGGREGATION_EXPORT Aggregate : public QObject
{
    Q_OBJECT

public:
    Aggregate(QObject *parent = nullptr);
    ~Aggregate() override;

    void add(QObject *component);
    void remove(QObject *component);

    template <typename T> T *component() {
        QReadLocker locker(&lock());
        for (QObject *component : std::as_const(m_components)) {
            if (T *result = qobject_cast<T *>(component))
                return result;
        }
        return nullptr;
    }

    template <typename T> QList<T *> components() {
        QReadLocker locker(&lock());
        QList<T *> results;
        for (QObject *component : std::as_const(m_components)) {
            if (T *result = qobject_cast<T *>(component)) {
                results << result;
            }
        }
        return results;
    }

    static Aggregate *parentAggregate(QObject *obj);
    static QReadWriteLock &lock();

signals:
    void changed();

private:
    void deleteSelf(QObject *obj);

    static QHash<QObject *, Aggregate *> &aggregateMap();

    QList<QObject *> m_components;
};

// get a component via global template function
template <typename T> T *query(Aggregate *obj)
{
    if (!obj)
        return nullptr;
    return obj->template component<T>();
}

template <typename T> T *query(QObject *obj)
{
    if (!obj)
        return nullptr;
    T *result = qobject_cast<T *>(obj);
    if (!result) {
        QReadLocker locker(&Aggregate::lock());
        Aggregate *parentAggregation = Aggregate::parentAggregate(obj);
        result = (parentAggregation ? query<T>(parentAggregation) : nullptr);
    }
    return result;
}

// get all components of a specific type via template function
template <typename T> QList<T *> query_all(Aggregate *obj)
{
    if (!obj)
        return QList<T *>();
    return obj->template components<T>();
}

template <typename T> QList<T *> query_all(QObject *obj)
{
    if (!obj)
        return QList<T *>();
    QReadLocker locker(&Aggregate::lock());
    Aggregate *parentAggregation = Aggregate::parentAggregate(obj);
    QList<T *> results;
    if (parentAggregation)
        results = query_all<T>(parentAggregation);
    else if (T *result = qobject_cast<T *>(obj))
        results.append(result);
    return results;
}

} // namespace Aggregation

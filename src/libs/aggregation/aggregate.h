/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QAGGREGATION_H
#define QAGGREGATION_H

#include "aggregation_global.h"

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QReadWriteLock>
#include <QtCore/QReadLocker>

namespace Aggregation {

class AGGREGATION_EXPORT Aggregate : public QObject
{
    Q_OBJECT

public:
    Aggregate(QObject *parent = 0);
    virtual ~Aggregate();

    void add(QObject *component);
    void remove(QObject *component);

    template <typename T> T *component() {
        QReadLocker(&lock());
        foreach (QObject *component, m_components) {
            if (T *result = qobject_cast<T *>(component))
                return result;
        }
        return (T *)0;
    }

    template <typename T> QList<T *> components() {
        QReadLocker(&lock());
        QList<T *> results;
        foreach (QObject *component, m_components) {
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

private slots:
    void deleteSelf(QObject *obj);

private:
    static QHash<QObject *, Aggregate *> &aggregateMap();

    QList<QObject *> m_components;
};

// get a component via global template function
template <typename T> T *query(Aggregate *obj)
{
    if (!obj)
        return (T *)0;
    return obj->template component<T>();
}

template <typename T> T *query(QObject *obj)
{
    if (!obj)
        return (T *)0;
    T *result = qobject_cast<T *>(obj);
    if (!result) {
        QReadLocker(&lock());
        Aggregate *parentAggregation = Aggregate::parentAggregate(obj);
        result = (parentAggregation ? query<T>(parentAggregation) : 0);
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
    QReadLocker(&lock());
    Aggregate *parentAggregation = Aggregate::parentAggregate(obj);
    QList<T *> results;
    if (parentAggregation)
        results = query_all<T>(parentAggregation);
    else if (T *result = qobject_cast<T *>(obj))
        results.append(result);
    return results;
}

} // namespace Aggregation

#endif // QAGGREGATION_H

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aggregate.h"

#include <QWriteLocker>
#include <QDebug>

/*!
    \namespace Aggregation
    \inmodule QtCreator
    \brief The Aggregation namespace contains support for bundling related components,
           so that each component exposes the properties and behavior of the
           other components to the outside.

    Components that are bundled into an aggregate can be \e cast to each other
    and have a coupled life cycle. See the documentation of Aggregation::Aggregate for
    details and examples.
*/

/*!
    \class Aggregation::Aggregate
    \inheaderfile aggregation/aggregate.h
    \inmodule QtCreator
    \ingroup mainclasses
    \threadsafe

    \brief The Aggregate class defines a collection of related components that
    can be viewed as a unit.

    An aggregate is a collection of components that are handled as a unit,
    such that each component exposes the properties and behavior of the
    other components in the aggregate to the outside.
    Specifically that means:
    \list
    \li They can be \e cast to each other (using query() and query_all()
        functions).
    \li Their life cycle is coupled. That is, whenever one is deleted, all of
        them are.
    \endlist
    Components can be of any QObject derived type.

    You can use an aggregate to simulate multiple inheritance by aggregation.
    Assuming we have the following code:
    \code
        using namespace Aggregation;
        class MyInterface : public QObject { ........ };
        class MyInterfaceEx : public QObject { ........ };
        [...]
        MyInterface *object = new MyInterface; // this is single inheritance
    \endcode
    The query function works like a qobject_cast() with normal objects:
    \code
        Q_ASSERT(query<MyInterface>(object) == object);
        Q_ASSERT(query<MyInterfaceEx>(object) == 0);
    \endcode
    If we want \c object to also implement the class \c MyInterfaceEx,
    but don't want to or cannot use multiple inheritance, we can do it
    at any point using an aggregate:
    \code
        MyInterfaceEx *objectEx = new MyInterfaceEx;
        Aggregate::aggregate({object, objectEx})
    \endcode
    The aggregate bundles the two objects together.
    If we have any part of the collection we get all parts:
    \code
        Q_ASSERT(query<MyInterface>(object) == object);
        Q_ASSERT(query<MyInterfaceEx>(object) == objectEx);
        Q_ASSERT(query<MyInterface>(objectEx) == object);
        Q_ASSERT(query<MyInterfaceEx>(objectEx) == objectEx);
    \endcode
    The following deletes all three: \c object, \c objectEx and \c aggregate:
    \code
        delete objectEx;
        // or delete object;
        // or delete aggregate;
    \endcode

    Aggregation-aware code never uses qobject_cast(). It always uses
    Aggregation::query(), which behaves like a qobject_cast() as a fallback.
*/

/*!
    \fn template <typename T> T *Aggregation::Aggregate::component()

    Template function that returns the component with the given type, if there is one.
    If there are multiple components with that type, a random one is returned.

    \sa Aggregate::components(), add()
*/

/*!
    \fn template <typename T> QList<T *> Aggregation::Aggregate::components()

    Template function that returns all components with the given type, if there are any.

    \sa Aggregate::component(), add()
*/

/*!
    \relates Aggregation::Aggregate
    \fn template <typename T> T *Aggregation::query<T *>(QObject *obj)

    Performs a dynamic cast that is aware of a possible aggregate that \a obj
    might belong to. If \a obj itself is of the requested type, it is simply cast
    and returned. Otherwise, if \a obj belongs to an aggregate, all its components are
    checked. If it doesn't belong to an aggregate, null is returned.

    \sa Aggregate::component()
*/

/*!
    \relates Aggregation::Aggregate
    \fn template <typename T> QList<T *> Aggregation::query_all<T *>(QObject *obj)

    If \a obj belongs to an aggregate, all components that can be cast to the given
    type are returned. Otherwise, \a obj is returned if it is of the requested type.

    \sa Aggregate::components()
*/

/*!
    \fn void Aggregation::Aggregate::changed()

    This signal is emitted when a component is added to or removed from an
    aggregate.

    \sa add(), remove()
*/

namespace Aggregation {

/*!
    Returns the aggregate object of \a obj if there is one. Otherwise returns 0.
*/
Aggregate *Aggregate::parentAggregate(QObject *obj)
{
    QReadLocker locker(&lock());
    return aggregateMap().value(obj);
}

QHash<QObject *, Aggregate *> &Aggregate::aggregateMap()
{
    static QHash<QObject *, Aggregate *> map;
    return map;
}

/*!
    \internal
*/
QReadWriteLock &Aggregate::lock()
{
    static QReadWriteLock lock;
    return lock;
}

/*!
    Constructs without locking.
    \internal
*/
Aggregate::Aggregate(enum PrivateConstructor)
{
    construct();
}

void Aggregate::construct()
{
    aggregateMap().insert(this, this);
}

/*!
    Creates a new aggregate with the given \a parent.
    The parent is directly passed to the QObject part
    of the class and is not used beside that.
*/
Aggregate::Aggregate(QObject *parent)
    : QObject(parent)
{
    QWriteLocker locker(&lock());
    construct();
}

/*!
    Deleting the aggregate automatically deletes all its components.
*/
Aggregate::~Aggregate()
{
    QObjectList components;
    {
        QWriteLocker locker(&lock());
        for (QObject *component : std::as_const(m_components)) {
            disconnect(component, &QObject::destroyed, this, &Aggregate::deleteSelf);
            aggregateMap().remove(component);
        }
        components = m_components;
        m_components.clear();
        aggregateMap().remove(this);
    }
    qDeleteAll(components);
}

void Aggregate::deleteSelf(QObject *obj)
{
    {
        QWriteLocker locker(&lock());
        aggregateMap().remove(obj);
        m_components.removeAll(obj);
    }
    delete this;
}

/*!
    Adds the \a component to the aggregate.
    You cannot add a component that is part of a different aggregate
    or an aggregate itself.

    \sa remove()
*/
void Aggregate::add(QObject *component)
{
    if (!component)
        return;
    {
        QWriteLocker locker(&lock());
        Aggregate *parentAggregation = aggregateMap().value(component);
        if (parentAggregation == this)
            return;
        if (parentAggregation) {
            qWarning() << "Cannot add a component that belongs to a different aggregate" << component;
            return;
        }
        m_components.append(component);
        connect(component, &QObject::destroyed, this, &Aggregate::deleteSelf);
        aggregateMap().insert(component, this);
    }
    emit changed();
}

/*!
    Removes the \a component from the aggregate.

    \sa add()
*/
void Aggregate::remove(QObject *component)
{
    if (!component)
        return;
    {
        QWriteLocker locker(&lock());
        aggregateMap().remove(component);
        m_components.removeAll(component);
        disconnect(component, &QObject::destroyed, this, &Aggregate::deleteSelf);
    }
    emit changed();
}

/*!
    This is a convenience function that creates a new Aggregate and adds all
    \a components to it. If any components already belong to an Aggregate,
    the remaining components are added to that instead.
    The components may not belong to different Aggregates to begin with.

    \sa Aggregate
*/
void aggregate(const QObjectList &components)
{
    QWriteLocker locker(&Aggregate::lock());
    Aggregate *agg = nullptr;
    QObjectList toAdd;
    for (QObject *comp : components) {
        Aggregate *existing = Aggregate::aggregateMap().value(comp);
        if (existing) {
            if (agg && agg != existing) {
                qWarning() << "Cannot aggregate components that belong to different aggregates" << components;
                return;
            }
            agg = existing;
        } else {
            toAdd << comp;
        }
    }
    if (!agg)
        agg = new Aggregate(Aggregate::PrivateConstructor); // we already have locked
    for (QObject *comp : std::as_const(toAdd)) { // add
        agg->m_components.append(comp);
        QObject::connect(comp, &QObject::destroyed, agg, &Aggregate::deleteSelf);
        Aggregate::aggregateMap().insert(comp, agg);
    }
}

} // namespace Aggregation

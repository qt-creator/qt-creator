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

#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <qcompilerdetection.h> // for Q_REQUIRED_RESULT

#include <algorithm>
#include <functional>
#include <QStringList>

namespace Utils
{
//////////////////
// anyOf
/////////////////

// anyOf taking a member function pointer
template<typename T, typename R, typename S>
bool anyOf(const T &container, R (S::*predicate)() const)
{
    return std::any_of(container.begin(), container.end(), std::mem_fn(predicate));
}

template<typename T, typename F>
bool anyOf(const T &container, F predicate)
{
    return std::any_of(container.begin(), container.end(), predicate);
}

template<typename T, typename F>
int count(const T &container, F predicate)
{
    return std::count_if(container.begin(), container.end(), predicate);
}

//////////////////
// allOf
/////////////////
template<typename T, typename F>
bool allOf(const T &container, F predicate)
{
    return std::all_of(container.begin(), container.end(), predicate);
}

//////////////////
// erase
/////////////////
template<typename T, typename F>
void erase(QList<T> &container, F predicate)
{
    container.erase(std::remove_if(container.begin(), container.end(), predicate),
                    container.end());
}

//////////////////
// contains
/////////////////
template<typename T, typename F>
bool contains(const T &container, F function)
{
    typename T::const_iterator end = container.end();
    typename T::const_iterator begin = container.begin();

    typename T::const_iterator it = std::find_if(begin, end, function);
    return it != end;
}

//////////////////
// findOr
/////////////////
template<typename T, typename F>
typename T::value_type findOr(const T &container, typename T::value_type other, F function)
{
    typename T::const_iterator end = container.end();
    typename T::const_iterator begin = container.begin();

    typename T::const_iterator it = std::find_if(begin, end, function);
    if (it == end)
        return other;
    return *it;
}

template<typename T, typename F>
int indexOf(const T &container, F function)
{
    typename T::const_iterator end = container.end();
    typename T::const_iterator begin = container.begin();

    typename T::const_iterator it = std::find_if(begin, end, function);
    if (it == end)
        return -1;
    return it - begin;
}

template<typename T, typename F>
typename T::value_type findOrDefault(const T &container, F function)
{
    return findOr(container, typename T::value_type(), function);
}

//////////////////
// find helpers
//////////////////
template<typename R, typename S, typename T>
auto equal(R (S::*function)() const, T value)
    -> decltype(std::bind<bool>(std::equal_to<T>(), value, std::bind(function, std::placeholders::_1)))
{
    return std::bind<bool>(std::equal_to<T>(), value, std::bind(function, std::placeholders::_1));
}

//////////////////
// transform
/////////////////

namespace {
// needed for msvc 2010, that doesn't have a declval
// can be removed once we stop supporting it
template<typename T>
T &&declval();

/////////////////
// helper code for transform to use back_inserter and thus push_back for everything
// and insert for QSet<>
//

// QSetInsertIterator, straight from the standard for insert_iterator
// just without the additional parameter to insert
template <class Container>
  class QSetInsertIterator :
    public std::iterator<std::output_iterator_tag,void,void,void,void>
{
protected:
  Container *container;

public:
  typedef Container container_type;
  explicit QSetInsertIterator (Container &x)
    : container(&x) {}
  QSetInsertIterator<Container> &operator=(const typename Container::value_type &value)
    { container->insert(value); return *this; }
  QSetInsertIterator<Container> &operator= (typename Container::value_type &&value)
    { container->insert(std::move(value)); return *this; }
  QSetInsertIterator<Container >&operator*()
    { return *this; }
  QSetInsertIterator<Container> &operator++()
    { return *this; }
  QSetInsertIterator<Container> operator++(int)
    { return *this; }
};

// inserter helper function, returns a std::back_inserter for most containers
// and is overloaded for QSet<> to return a QSetInsertIterator
template<typename C>
inline std::back_insert_iterator<C>
inserter(C &container)
{
    return std::back_inserter(container);
}

template<typename X>
inline QSetInsertIterator<QSet<X>>
inserter(QSet<X> &container)
{
    return QSetInsertIterator<QSet<X>>(container);
}

// helper: removes const, volatile and references from a type
template<typename T>
struct RemoveCvAndReference
{
    typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type type;
};

// abstraction to treat Container<T> and QStringList similarly
template<typename T>
struct ContainerType
{

};

// specialization for qt container T_Container<T_Type>
template<template<typename> class T_Container, typename T_Type>
struct ContainerType<T_Container<T_Type>> {
    typedef T_Type ElementType;

    template<class NewElementType>
    struct WithElementType
    {
        typedef T_Container<NewElementType> type;
    };
};

// specialization for QStringList
template<>
struct ContainerType<QStringList>
{
    typedef QString ElementType;

    template<class NewElementType>
    struct WithElementType
    {
        typedef QList<NewElementType> type;
    };
};

}

// actual implementation of transform
template<typename C, // result container type
         typename SC> // input container type
struct TransformImpl {
    template <typename F>
    Q_REQUIRED_RESULT
    static C call(const SC &container, F function)
    {
        C result;
        result.reserve(container.size());
        std::transform(container.begin(), container.end(),
                       inserter(result),
                       function);
        return result;
    }

    template <typename R, typename S>
    Q_REQUIRED_RESULT
    static C call(const SC &container, R (S::*p)() const)
    {
        return call(container, std::mem_fn(p));
    }

};

// same container type for input and output, e.g. transforming a QList<QString> into QList<int>
// or QStringList -> QList<>
template<typename C, // container
         typename F>
Q_REQUIRED_RESULT
auto transform(const C &container, F function)
-> typename ContainerType<C>::template WithElementType< // the type C<stripped return type of F>
        typename RemoveCvAndReference< // the return type of F stripped
            decltype(declval<F>()(declval<typename ContainerType<C>::ElementType>())) // the return type of F
        >::type
    >::type
{
    return TransformImpl<
                typename ContainerType<C>::template WithElementType< // the type C<stripped return type>
                    typename RemoveCvAndReference< // the return type stripped
                        decltype(declval<F>()(declval<typename ContainerType<C>::ElementType>())) // the return type of F
                    >::type
                >::type,
                C
            >::call(container, function);
}

// same container type for member function pointer
template<typename C,
        typename R,
        typename S>
Q_REQUIRED_RESULT
auto transform(const C &container, R (S::*p)() const)
    ->typename ContainerType<C>::template WithElementType<typename RemoveCvAndReference<R>::type>::type
{
    return TransformImpl<
                typename ContainerType<C>::template WithElementType< // the type C<stripped R>
                    typename RemoveCvAndReference<R>::type // stripped R
                >::type,
                C
            >::call(container, p);
}

// different container types for input and output, e.g. transforming a QList into a QSet
template<template<typename> class C, // result container type
         typename SC, // input container type
         typename F> // function type
Q_REQUIRED_RESULT
auto transform(const SC &container, F function)
     -> C< // container C<stripped return type of F>
            typename RemoveCvAndReference< // stripped return type of F
                decltype(declval<F>()(declval<typename ContainerType<SC>::ElementType>())) // return type of F
            >::type>
{
    return TransformImpl<
                C< // result container type
                    typename RemoveCvAndReference< // stripped
                        decltype(declval<F>()(declval<typename ContainerType<SC>::ElementType>())) // return type of F
                    >::type>,
                SC
            >::call(container, function);
}

// different container types for input and output, e.g. transforming a QList into a QSet
// for member function pointers
template<template<typename> class C, // result container type
         typename SC, // input container type
         typename R,
         typename S>
Q_REQUIRED_RESULT
auto transform(const SC &container, R (S::*p)() const)
     -> C<typename RemoveCvAndReference<R>::type>
{
    return TransformImpl<
                C<typename RemoveCvAndReference<R>::type>,
                SC
            >::call(container, p);
}

//////////////////
// sort
/////////////////
template <typename Container>
inline void sort(Container &c)
{
    std::sort(c.begin(), c.end());
}

template <typename Container, typename Predicate>
inline void sort(Container &c, Predicate p)
{
    std::sort(c.begin(), c.end(), p);
}

}

#endif // ALGORITHM_H

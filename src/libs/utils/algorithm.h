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

#pragma once

#include "predicates.h"

#include <qcompilerdetection.h> // for Q_REQUIRED_RESULT

#include <algorithm>
#include <tuple>

#include <QObject>
#include <QStringList>

#include <memory>

namespace Utils
{
//////////////////
// anyOf
/////////////////
template<typename T, typename F>
bool anyOf(const T &container, F predicate)
{
    return std::any_of(std::begin(container), std::end(container), predicate);
}

// anyOf taking a member function pointer
template<typename T, typename R, typename S>
bool anyOf(const T &container, R (S::*predicate)() const)
{
    return std::any_of(std::begin(container), std::end(container), std::mem_fn(predicate));
}

// anyOf taking a member pointer
template<typename T, typename R, typename S>
bool anyOf(const T &container, R S::*member)
{
    return std::any_of(std::begin(container), std::end(container), std::mem_fn(member));
}


//////////////////
// count
/////////////////
template<typename T, typename F>
int count(const T &container, F predicate)
{
    return std::count_if(std::begin(container), std::end(container), predicate);
}

//////////////////
// allOf
/////////////////
template<typename T, typename F>
bool allOf(const T &container, F predicate)
{
    return std::all_of(std::begin(container), std::end(container), predicate);
}

//////////////////
// erase
/////////////////
template<typename T, typename F>
void erase(T &container, F predicate)
{
    container.erase(std::remove_if(std::begin(container), std::end(container), predicate),
                    std::end(container));
}


//////////////////
// contains
/////////////////
template<typename T, typename F>
bool contains(const T &container, F function)
{
    return anyOf(container, function);
}

// Contains for normal pointers in std::vector<std::unique_ptr>
template<template<typename, typename...> class C, typename T, typename... Args>
bool contains(const C<T, Args...> &container, typename T::element_type *other)
{
    return anyOf(container, [other](const typename C<T, Args...>::value_type &value) { return value.get() == other; });
}

template<typename T, typename R, typename S>
bool contains(const T &container, R (S::*function)() const)
{
    return anyOf(container, function);
}

template<template<typename, typename...> class C, typename T, typename R, typename S, typename... Args>
bool contains(const C<T, Args...> &container, R (S::*function)() const)
{
    return anyOf(container, function);
}

//////////////////
// findOr
/////////////////

// Containers containing std::unique_ptr:
template<template<typename, typename...> class C,
         typename T, typename D,
         typename F,
         typename... Args>
Q_REQUIRED_RESULT
T *findOr(const C<std::unique_ptr<T, D>, Args...> &container, T *other, F function)
{
    auto end = std::end(container);
    auto it = std::find_if(std::begin(container), end, function);
    return (it == end) ? other : it->get();
}

template<template<typename, typename...> class C,
         typename T, typename D,
         typename R, typename S,
         typename... Args>
Q_REQUIRED_RESULT
T *findOr(const C<std::unique_ptr<T, D>, Args...> &container, T *other, R (S::*function)() const)
{
    return findOr(container, other, std::mem_fn(function));
}

template<template<typename, typename...> class C, typename... Args,
         typename T, typename D,
         typename R, typename S>
Q_REQUIRED_RESULT
T *findOr(const C<std::unique_ptr<T, D>, Args...> &container, T *other, R S::*member)
{
    return findOr(container, other, std::mem_fn(member));
}

template<typename C, typename F>
Q_REQUIRED_RESULT
typename C::value_type findOr(const C &container, typename C::value_type other, F function)
{
    typename C::const_iterator begin = std::begin(container);
    typename C::const_iterator end = std::end(container);

    typename C::const_iterator it = std::find_if(begin, end, function);
    return it == end ? other : *it;
}

template<typename T, typename R, typename S>
Q_REQUIRED_RESULT
typename T::value_type findOr(const T &container, typename T::value_type other, R (S::*function)() const)
{
    return findOr(container, other, std::mem_fn(function));
}

template<typename T, typename R, typename S>
Q_REQUIRED_RESULT
typename T::value_type findOr(const T &container, typename T::value_type other, R S::*member)
{
    return findOr(container, other, std::mem_fn(member));
}

//////////////////
// findOrDefault
//////////////////

// Containers containing std::unique_ptr:
template<template<typename, typename...> class C,
         typename T, typename D,
         typename F,
         typename... Args>
Q_REQUIRED_RESULT
T *findOrDefault(const C<std::unique_ptr<T, D>, Args...> &container, F function)
{
    return findOr(container, static_cast<T*>(nullptr), function);
}

template<template<typename, typename...> class C,
         typename T, typename D,
         typename R, typename S,
         typename... Args>
Q_REQUIRED_RESULT
T *findOrDefault(const C<std::unique_ptr<T, D>, Args...> &container, R (S::*function)() const)
{
    return findOr(container, static_cast<T*>(nullptr), std::mem_fn(function));
}

template<template<typename, typename...> class C,
         typename T, typename D,
         typename R, typename S,
         typename... Args>
Q_REQUIRED_RESULT
T *findOrDefault(const C<std::unique_ptr<T, D>, Args...> &container, R S::*member)
{
    return findOr(container, static_cast<T*>(nullptr), std::mem_fn(member));
}


// Default implementation:
template<typename C, typename F>
Q_REQUIRED_RESULT
typename C::value_type findOrDefault(const C &container, F function)
{
    return findOr(container, typename C::value_type(), function);
}

template<typename C, typename R, typename S>
Q_REQUIRED_RESULT
typename C::value_type findOrDefault(const C &container, R (S::*function)() const)
{
    return findOr(container, typename C::value_type(), std::mem_fn(function));
}

template<typename C, typename R, typename S>
Q_REQUIRED_RESULT
typename C::value_type findOrDefault(const C &container, R S::*member)
{
    return findOr(container, typename C::value_type(), std::mem_fn(member));
}


//////////////////
// index of:
//////////////////

template<typename C, typename F>
Q_REQUIRED_RESULT
int indexOf(const C& container, F function)
{
    typename C::const_iterator begin = std::begin(container);
    typename C::const_iterator end = std::end(container);

    typename C::const_iterator it = std::find_if(begin, end, function);
    return it == end ? -1 : std::distance(begin, it);
}


//////////////////
// max element
//////////////////

template<typename T>
typename T::value_type maxElementOr(const T &container, typename T::value_type other)
{
    typename T::const_iterator begin = std::begin(container);
    typename T::const_iterator end = std::end(container);

    typename T::const_iterator it = std::max_element(begin, end);
    if (it == end)
        return other;
    return *it;
}


//////////////////
// transform
/////////////////

namespace {
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

} // anonymous

// --------------------------------------------------------------------
// Different containers for input and output:
// --------------------------------------------------------------------

// different container types for input and output, e.g. transforming a QList into a QSet

// function:
template<template<typename> class C, // result container type
         template<typename...> class SC, // input container type
         typename F, // function type
         typename... SCArgs, // Arguments to SC
         typename Value = typename SC<SCArgs...>::value_type,
         typename ResultContainer = C<std::decay_t<std::result_of_t<F(Value)>>>>
Q_REQUIRED_RESULT
decltype(auto) transform(const SC<SCArgs...> &container, F function)
{
    ResultContainer result;
    result.reserve(container.size());
    std::transform(std::begin(container), std::end(container), inserter(result), function);
    return result;
}

template<template<typename, typename> class C, // result container type
         template<typename...> class SC, // input container type
         typename F, // function type
         typename... SCArgs, // Arguments to SC
         typename Value = typename SC<SCArgs...>::value_type,
         typename Result = std::decay_t<std::result_of_t<F(Value)>>,
         typename ResultContainer = C<Result, std::allocator<Result>>>
Q_REQUIRED_RESULT
decltype(auto) transform(const SC<SCArgs...> &container, F function)
{
    ResultContainer result;
    result.reserve(container.size());
    std::transform(std::begin(container), std::end(container), inserter(result), function);
    return result;
}

// member function:
template<template<typename...> class C, // result container type
         template<typename...> class SC, // input container type
         typename R,
         typename S,
         typename... SCArgs> // Arguments to SC
Q_REQUIRED_RESULT
decltype(auto) transform(const SC<SCArgs...> &container, R (S::*p)() const)
{
    return transform<C, SC>(container, std::mem_fn(p));
}

// members:
template<template<typename...> class C, // result container
         template<typename...> class SC, // input container
         typename R,
         typename S,
         typename... SCArgs> // Arguments to SC
Q_REQUIRED_RESULT
decltype(auto) transform(const SC<SCArgs...> &container, R S::*p)
{
    return transform<C, SC>(container, std::mem_fn(p));
}

// different container types for input and output, e.g. transforming a QList into a QSet

// function:
template<template<typename...> class C, // container type
         typename F, // function type
         typename... CArgs> // Arguments to SC
Q_REQUIRED_RESULT
decltype(auto) transform(const C<CArgs...> &container, F function)
{
    return transform<C, C>(container, function);
}

// member function:
template<template<typename...> class C, // container type
         typename R,
         typename S,
         typename... CArgs> // Arguments to SC
Q_REQUIRED_RESULT
decltype(auto) transform(const C<CArgs...> &container, R (S::*p)() const)
{
    return transform<C, C>(container, std::mem_fn(p));
}

// members:
template<template<typename...> class C, // container
         typename R,
         typename S,
         typename... CArgs> // Arguments to SC
Q_REQUIRED_RESULT
decltype(auto) transform(const C<CArgs...> &container, R S::*p)
{
    return transform<C, C>(container, std::mem_fn(p));
}

// Specialization for QStringList:

template<template<typename...> class C = QList, // result container
         typename F> // Arguments to C
Q_REQUIRED_RESULT
decltype(auto) transform(const QStringList &container, F function)
{
    return transform<C, QList>(static_cast<QList<QString>>(container), function);
}

// member function:
template<template<typename...> class C = QList, // result container type
         typename R,
         typename S>
Q_REQUIRED_RESULT
decltype(auto) transform(const QStringList &container, R (S::*p)() const)
{
    return transform<C, QList>(static_cast<QList<QString>>(container), std::mem_fn(p));
}

// members:
template<template<typename...> class C = QList, // result container
         typename R,
         typename S>
Q_REQUIRED_RESULT
decltype(auto) transform(const QStringList &container, R S::*p)
{
    return transform<C, QList>(static_cast<QList<QString>>(container), std::mem_fn(p));
}

//////////////////
// filtered
/////////////////
template<typename C, typename F>
Q_REQUIRED_RESULT
C filtered(const C &container, F predicate)
{
    C out;
    std::copy_if(std::begin(container), std::end(container),
                 inserter(out), predicate);
    return out;
}

template<typename C, typename R, typename S>
Q_REQUIRED_RESULT
C filtered(const C &container, R (S::*predicate)() const)
{
    C out;
    std::copy_if(std::begin(container), std::end(container),
                 inserter(out), std::mem_fn(predicate));
    return out;
}

//////////////////
// partition
/////////////////

// Recommended usage:
// C hit;
// C miss;
// std::tie(hit, miss) = Utils::partition(container, predicate);

template<typename C, typename F>
Q_REQUIRED_RESULT
std::tuple<C, C> partition(const C &container, F predicate)
{
    C hit;
    C miss;
    auto hitIns = inserter(hit);
    auto missIns = inserter(miss);
    for (auto i : container) {
        if (predicate(i))
            hitIns = i;
        else
            missIns = i;
    }
    return std::make_tuple(hit, miss);
}

template<typename C, typename R, typename S>
Q_REQUIRED_RESULT
std::tuple<C, C> partition(const C &container, R (S::*predicate)() const)
{
    return partition(container, std::mem_fn(predicate));
}

//////////////////
// filteredUnique
/////////////////

template<typename C>
Q_REQUIRED_RESULT
C filteredUnique(const C &container)
{
    C result;
    auto ins = inserter(result);

    QSet<typename C::value_type> seen;
    int setSize = 0;

    auto endIt = std::end(container);
    for (auto it = std::begin(container); it != endIt; ++it) {
        seen.insert(*it);
        if (setSize == seen.size()) // unchanged size => was already seen
            continue;
        ++setSize;
        ins = *it;
    }
    return result;
}

//////////////////
// qobject_container_cast
/////////////////
template <class T, template<typename> class Container, typename Base>
Container<T> qobject_container_cast(const Container<Base> &container)
{
    Container<T> result;
    auto ins = inserter(result);
    for (Base val : container) {
        if (T target = qobject_cast<T>(val))
            ins = target;
    }
    return result;
}

//////////////////
// sort
/////////////////
template <typename Container>
inline void sort(Container &container)
{
    std::sort(std::begin(container), std::end(container));
}

template <typename Container, typename Predicate>
inline void sort(Container &container, Predicate p)
{
    std::sort(std::begin(container), std::end(container), p);
}

// pointer to member
template <typename Container, typename R, typename S>
inline void sort(Container &container, R S::*member)
{
    auto f = std::mem_fn(member);
    using const_ref = typename Container::const_reference;
    std::sort(std::begin(container), std::end(container),
              [&f](const_ref a, const_ref b) {
        return f(a) < f(b);
    });
}

// pointer to member function
template <typename Container, typename R, typename S>
inline void sort(Container &container, R (S::*function)() const)
{
    auto f = std::mem_fn(function);
    using const_ref = typename Container::const_reference;
    std::sort(std::begin(container), std::end(container),
              [&f](const_ref a, const_ref b) {
        return f(a) < f(b);
    });
}

//////////////////
// reverseForeach
/////////////////
template <typename Container, typename Op>
inline void reverseForeach(const Container &c, const Op &operation)
{
    auto rend = c.rend();
    for (auto it = c.rbegin(); it != rend; ++it)
        operation(*it);
}

}

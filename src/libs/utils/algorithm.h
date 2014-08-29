/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <qcompilerdetection.h> // for Q_REQUIRED_RESULT

#include <algorithm>
#include <functional>

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

// transform taking a member function pointer
template<typename T, typename R, typename S>
Q_REQUIRED_RESULT
auto transform(const QList<T> &container, R (S::*p)() const) -> QList<R>
{
    QList<R> result;
    result.reserve(container.size());
    std::transform(container.begin(), container.end(),
                   std::back_inserter(result),
                   std::mem_fn(p));
    return result;
}

namespace {
// needed for msvc 2010, that doesn't have a declval
// can be removed once we stop supporting it
template<typename T>
T &&declval();
}

// Note: add overloads for other container types as needed
template<typename T, typename F>
Q_REQUIRED_RESULT
auto transform(const QList<T> &container, F function)
     -> QList<typename std::remove_reference<decltype(function(declval<T>()))>::type>
{
    QList<typename std::remove_reference<decltype(function(declval<T>()))>::type> result;
    result.reserve(container.size());
    std::transform(container.begin(), container.end(),
                   std::back_inserter(result),
                   function);
    return result;
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

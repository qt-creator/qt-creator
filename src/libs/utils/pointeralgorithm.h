/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "algorithm.h"

#include <QDebug>

namespace Utils
{

template<typename C>
using ValueType = typename C::value_type;

template<typename C>
using PointerType = typename C::value_type::element_type*;

//////////////////
// anyOf
/////////////////
template<typename C>
bool anyOf(const C &container, PointerType<C> p)
{
    return anyOf(container, [p](const ValueType<C> &v) { return v.get() == p; });
}

template<typename C>
bool anyOf(const C &container, std::nullptr_t)
{
    return anyOf(container, static_cast<PointerType<C>>(nullptr));
}

//////////////////
// count
/////////////////
template<typename C>
int count(const C &container, PointerType<C> p)
{
    return count(container, [p](const ValueType<C> &v) { return v.get() == p; });
}

template<typename C>
int count(const C &container, std::nullptr_t)
{
    return count(container, static_cast<PointerType<C>>(nullptr));
}

//////////////////
// allOf
/////////////////
template<typename C>
bool allOf(const C &container, PointerType<C> p)
{
    return allOf(container, [p](const ValueType<C> &v) { return v.get() == p; });
}

template<typename C>
int allOf(const C &container, std::nullptr_t)
{
    return allOf(container, static_cast<PointerType<C>>(nullptr));
}

//////////////////
// erase
/////////////////
template<typename C>
void erase(C &container, PointerType<C> p)
{
    return erase(container, [p](const ValueType<C> &v) { return v.get() == p; });
}

template<typename C>
int erase(const C &container, std::nullptr_t)
{
    return erase(container, static_cast<PointerType<C>>(nullptr));
}

//////////////////
// contains
/////////////////
template<typename C>
bool contains(const C &container, PointerType<C> p)
{
    return anyOf(container, p);
}

template<typename C>
bool contains(const C &container, std::nullptr_t)
{
    return anyOf(container, nullptr);
}

//////////////////
// findOr
/////////////////

template<typename C, typename F>
Q_REQUIRED_RESULT
PointerType<C> findOr(const C &container, PointerType<C> other, F function)
{
    typename C::const_iterator begin = std::begin(container);
    typename C::const_iterator end = std::end(container);

    typename C::const_iterator it = std::find_if(begin, end, function);
    return it == end ? other : it->get();
}

template<typename C, typename R, typename S>
Q_REQUIRED_RESULT
PointerType<C> findOr(const C &container, PointerType<C> other, R (S::*function)() const)
{
    return findOr(container, other, std::mem_fn(function));
}

template<typename C, typename R, typename S>
Q_REQUIRED_RESULT
PointerType<C> findOr(const C &container, PointerType<C> other, R S::*member)
{
    return findOr(container, other, std::mem_fn(member));
}


template<typename C, typename F>
Q_REQUIRED_RESULT
PointerType<C> findOr(const C &container, std::nullptr_t, F function)
{
    return findOr(container, static_cast<PointerType<C>>(nullptr), function);
}

template<typename C, typename R, typename S>
Q_REQUIRED_RESULT
PointerType<C> findOr(const C &container, std::nullptr_t, R (S::*function)() const)
{
    return findOr(container, static_cast<PointerType<C>>(nullptr), function);
}

template<typename C, typename R, typename S>
Q_REQUIRED_RESULT
PointerType<C> findOr(const C &container, std::nullptr_t, R S::*member)
{
    return findOr(container, static_cast<PointerType<C>>(nullptr), member);
}

template<typename C>
Q_REQUIRED_RESULT
PointerType<C> findOr(const C &container, PointerType<C> other, PointerType<C> p)
{
    return findOr(container, other, [p](const ValueType<C> &v) { return v.get() == p; });
}

template<typename C>
Q_REQUIRED_RESULT
PointerType<C> findOr(const C &container, PointerType<C> other, std::nullptr_t)
{
    return findOr(container, other, static_cast<PointerType<C>>(nullptr));
}

template<typename C>
Q_REQUIRED_RESULT
PointerType<C> findOr(const C &container, std::nullptr_t, PointerType<C> p)
{
    return findOr(container, static_cast<PointerType<C>>(nullptr), p);
}

template<typename C>
Q_REQUIRED_RESULT
PointerType<C> findOr(const C &container, std::nullptr_t, std::nullptr_t)
{
    return findOr(container, static_cast<PointerType<C>>(nullptr), static_cast<PointerType<C>>(nullptr));
}

//////////////////
// findOrDefault
/////////////////
template<typename C,typename F>
Q_REQUIRED_RESULT
PointerType<C> findOrDefault(const C &container, F function)
{
    return findOr(container, static_cast<PointerType<C>>(nullptr), function);
}

template<typename C, typename R, typename S>
Q_REQUIRED_RESULT
PointerType<C> findOrDefault(const C &container, R (S::*function)() const)
{
    return findOr(container, static_cast<PointerType<C>>(nullptr), std::mem_fn(function));
}

template<typename C, typename R, typename S>
Q_REQUIRED_RESULT
PointerType<C> findOrDefault(const C &container, R S::*member)
{
    return findOr(container, static_cast<PointerType<C>>(nullptr), std::mem_fn(member));
}

template<typename C>
Q_REQUIRED_RESULT
PointerType<C> findOrDefault(const C &container, PointerType<C> p)
{
    return findOr(container, static_cast<PointerType<C>>(nullptr), p);
}

template<typename C>
Q_REQUIRED_RESULT
PointerType<C> findOrDefault(const C &container, std::nullptr_t)
{
    return findOr(container, static_cast<PointerType<C>>(nullptr), static_cast<PointerType<C>>(nullptr));
}

//////////////////
// index of:
//////////////////
template<typename C>
Q_REQUIRED_RESULT
int indexOf(const C& container, PointerType<C> p)
{
    return indexOf(container, [p](const ValueType<C> &v) { return v.get() == p; });
}

template<typename C>
Q_REQUIRED_RESULT
int indexOf(const C& container, std::nullptr_t)
{
    return indexOf(container, static_cast<PointerType<C>>(nullptr));
}

//////////////////
// toRawPointer
/////////////////
template <typename ResultContainer,
          typename SourceContainer>
ResultContainer toRawPointer(const SourceContainer &sources)
{
    return transform<ResultContainer>(sources, [] (const auto &pointer) { return pointer.get(); });
}

template <template<typename...> class ResultContainer,
          template<typename...> class SourceContainer,
          typename... SCArgs>
auto toRawPointer(const SourceContainer<SCArgs...> &sources)
{
    return transform<ResultContainer, const SourceContainer<SCArgs...> &>(sources, [] (const auto &pointer) { return pointer.get(); });
}

template <class SourceContainer>
auto toRawPointer(const SourceContainer &sources)
{
    return transform(sources, [] (const auto &pointer) { return pointer.get(); });
}

//////////////////
// take:
/////////////////
template<typename C>
Q_REQUIRED_RESULT optional<ValueType<C>> take(C &container, PointerType<C> p)
{
    return take(container, [p](const ValueType<C> &v) { return v.get() == p; });
}

template <typename C>
Q_REQUIRED_RESULT optional<ValueType<C>> take(C &container, std::nullptr_t)
{
    return take(container, static_cast<PointerType<C>>(nullptr));
}

//////////////////
// takeOrDefault:
/////////////////
template<typename C>
Q_REQUIRED_RESULT ValueType<C> takeOrDefault(C &container, PointerType<C> p)
{
    auto result = take(container, [p](const ValueType<C> &v) { return v.get() == p; });
    return bool(result) ? std::move(result.value()) : std::move(ValueType<C>());
}

template <typename C>
Q_REQUIRED_RESULT ValueType<C> takeOrDefault(C &container, std::nullptr_t)
{
    return takeOrDefault(container, static_cast<PointerType<C>>(nullptr));
}

} // namespace Utils

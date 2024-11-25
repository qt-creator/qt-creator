// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <functional>

namespace Building {

class NestId {};

template <typename Id, typename Arg>
class IdAndArg
{
public:
    IdAndArg(Id, const Arg &arg) : arg(arg) {}
    const Arg arg; // FIXME: Could be const &, but this would currently break bindTo().
};

// The main dispatcher

void doit(auto x, auto id, auto p);

template <typename X> class BuilderItem
{
public:
    // Property setter
    template <typename Id, typename Arg>
    BuilderItem(IdAndArg<Id, Arg> && idarg)
        : apply([&idarg](X *x) { doit(x, Id{}, idarg.arg); })
    {}

    // Nested child object
    template <typename Inner>
    BuilderItem(Inner && p)
        : apply([&p](X *x) { doit(x, NestId{}, std::forward<Inner>(p)); })
    {}

    const std::function<void(X *)> apply;
};

#define QTC_DEFINE_BUILDER_SETTER(name, setter) \
class name##_TAG {}; \
template <typename ...Args> \
inline auto name(Args &&...args) { \
    return Building::IdAndArg{name##_TAG{}, std::tuple<Args...>{std::forward<Args>(args)...}}; \
} \
template <typename L, typename ...Args> \
inline void doit(L *x, name##_TAG, const std::tuple<Args...> &arg) { \
    std::apply(&L::setter, std::tuple_cat(std::make_tuple(x), arg)); \
}

} // Building

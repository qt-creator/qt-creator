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
    /* The requires expressions must live outside the lambda: MSVC raises a \
       hard error instead of an unsatisfied constraint when a requires \
       expression in a lambda body names a non-existent member. */ \
    if constexpr (requires(L *l, const Args &...a) { l->setter(a...); }) \
        std::apply([x](const auto &...a) { x->setter(a...); }, arg); \
    else if constexpr (requires(L &l, const Args &...a) { setter(l, a...); }) \
        std::apply([x](const auto &...a) { setter(*x, a...); }, arg); \
    else \
        static_assert(sizeof...(Args) + 1 == 0, \
                      "no matching member or free function \"" #setter "\""); \
}

} // Building

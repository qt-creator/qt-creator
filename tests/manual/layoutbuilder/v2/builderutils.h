// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <concepts>
#include <functional>
#include <tuple>
#include <utility>

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

// Defines a property setter for use in builder expressions. For example
//
//    inline constexpr auto text = Building::setter([](auto &x, auto &&...a) { x.setText(a...); });
//
// creates a builder item `text(...)` that invokes the given function object
// with the builder object followed by the arguments of the use site.

template <typename SetterFunction>
class Setter
{
public:
    template <typename ...Args>
    auto operator()(Args &&...args) const
    {
        return IdAndArg{*this, std::tuple<Args...>{std::forward<Args>(args)...}};
    }
};

template <typename SetterFunction, typename L, typename ...Args, std::size_t ...I>
void applySetter(L *x, const std::tuple<Args...> &arg, std::index_sequence<I...>)
{
    SetterFunction{}(*x, std::get<I>(arg)...);
}

template <typename L, typename SetterFunction, typename ...Args>
void doit(L *x, Setter<SetterFunction>, const std::tuple<Args...> &arg)
{
    applySetter<SetterFunction>(x, arg, std::make_index_sequence<sizeof...(Args)>{});
}

// The constraint rejects capturing lambdas at the definition site; the
// function object is recreated from its type in applySetter, so it must
// be default-constructible.
template <std::default_initializable SetterFunction>
constexpr Setter<SetterFunction> setter(SetterFunction)
{
    return {};
}

} // Building

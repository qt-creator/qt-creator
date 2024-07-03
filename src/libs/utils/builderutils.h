// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <functional>

#if defined(UTILS_LIBRARY)
#  define QTCREATOR_UTILS_EXPORT Q_DECL_EXPORT
#elif defined(UTILS_STATIC_LIBRARY)
#  define QTCREATOR_UTILS_EXPORT
#else
#  define QTCREATOR_UTILS_EXPORT Q_DECL_IMPORT
#endif

namespace Building {

class NestId {};

template <typename Id, typename Arg>
class IdAndArg
{
public:
    IdAndArg(Id, const Arg &arg) : arg(arg) {}
    const Arg arg; // FIXME: Could be const &, but this would currently break bindTo().
};

template<typename T1, typename T2>
struct Arg2
{
    Arg2(const T1 &a1, const T2 &a2)
        : p1(a1)
        , p2(a2)
    {}
    const T1 p1;
    const T2 p2;
};

template<typename T1, typename T2, typename T3>
struct Arg3
{
    Arg3(const T1 &a1, const T2 &a2, const T3 &a3)
        : p1(a1)
        , p2(a2)
        , p3(a3)
    {}
    const T1 p1;
    const T2 p2;
    const T3 p3;
};

template<typename T1, typename T2, typename T3, typename T4>
struct Arg4
{
    Arg4(const T1 &a1, const T2 &a2, const T3 &a3, const T4 &a4)
        : p1(a1)
        , p2(a2)
        , p3(a3)
        , p4(a4)
    {}
    const T1 p1;
    const T2 p2;
    const T3 p3;
    const T4 p4;
};

// The main dispatcher

void doit(auto x, auto id, auto p);

template <typename X> class BuilderItem
{
public:
    // Property setter
    template <typename Id, typename Arg>
    BuilderItem(IdAndArg<Id, Arg> && idarg)
    {
        apply = [&idarg](X *x) { doit(x, Id{}, idarg.arg); };
    }

    // Nested child object
    template <typename Inner>
    BuilderItem(Inner && p)
    {
        apply = [&p](X *x) { doit(x, NestId{}, std::forward<Inner>(p)); };
    }

    std::function<void(X *)> apply;
};

#define QTC_DEFINE_BUILDER_SETTER(name, setter) \
    class name##_TAG {}; \
    inline auto name(auto p) { return Building::IdAndArg{name##_TAG{}, p}; } \
    inline void doit(auto x, name##_TAG, auto p) { x->setter(p); }

#define QTC_DEFINE_BUILDER_SETTER2(name, setter) \
    class name##_TAG {}; \
    inline auto name(auto p1, auto p2) { return Building::IdAndArg{name##_TAG{}, Building::Arg2{p1, p2}}; } \
    inline void doit(auto x, name##_TAG, auto p) { x->setter(p.p1, p.p2); }

#define QTC_DEFINE_BUILDER_SETTER3(name, setter) \
    class name##_TAG {}; \
    inline auto name(auto p1, auto p2, auto p3) { return Building::IdAndArg{name##_TAG{}, Building::Arg3{p1, p2, p3}}; } \
    inline void doit(auto x, name##_TAG, auto p) { x->setter(p.p1, p.p2, p.p3); }

#define QTC_DEFINE_BUILDER_SETTER4(name, setter) \
    class name##_TAG {}; \
    inline auto name(auto p1, auto p2, auto p3, auto p4) { return Building::IdAndArg{name##_TAG{}, Building::Arg4{p1, p2, p3, p4}}; } \
    inline void doit(auto x, name##_TAG, auto p) { x->setter(p.p1, p.p2, p.p3, p.p4); }

} // Building

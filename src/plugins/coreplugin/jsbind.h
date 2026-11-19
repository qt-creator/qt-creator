// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

// A small, type-safe binding layer for exposing C++ callables to the embedded
// JavaScript engine. Because Qt Creator owns both sides of the bridge,
// extensions no longer need to be QObjects with Q_INVOKABLE methods reflected
// at runtime: they register plain callables, and argument/return marshalling is
// generated from the callable's signature at compile time.
//
// The engine itself (QuickJS) is confined to the Core plugin. This header is
// deliberately engine-agnostic - it marshals only to/from QVariant - so that
// extensions in any plugin can use it without depending on the engine. Add a
// fromVariant()/QVariant round-trip for a new parameter or return type here.

#include <utils/filepath.h>

#include <QVariant>
#include <QVariantList>

#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace Core::JsBind {

// QVariant -> C++ argument type.
template <class T>
T fromVariant(const QVariant &value)
{
    if constexpr (std::is_same_v<T, Utils::FilePath>)
        return Utils::FilePath::fromUserInput(value.toString());
    else if constexpr (std::is_same_v<T, QString>)
        return value.toString();
    else if constexpr (std::is_same_v<T, QStringList>)
        return value.toStringList();
    else if constexpr (std::is_same_v<T, bool>)
        return value.toBool();
    else if constexpr (std::is_same_v<T, int>)
        return value.toInt();
    else
        return value.value<T>();
}

// C++ return value -> QVariant (a QVariant return is passed through unchanged).
template <class T>
QVariant toVariant(T &&value)
{
    if constexpr (std::is_same_v<std::decay_t<T>, QVariant>)
        return std::forward<T>(value);
    else
        return QVariant::fromValue(std::forward<T>(value));
}

// A call whose arguments and result are marshalled as QVariant. The Core plugin
// turns this into an engine function.
using VariantFunction = std::function<QVariant(const QVariantList &)>;

// Deduce a callable's return type and (decayed) argument types.
template <class T>
struct Signature : Signature<decltype(&T::operator())> {};
template <class R, class C, class... A>
struct Signature<R (C::*)(A...) const> {
    using Return = R;
    using Args = std::tuple<std::decay_t<A>...>;
};
template <class R, class... A>
struct Signature<R (*)(A...)> {
    using Return = R;
    using Args = std::tuple<std::decay_t<A>...>;
};

template <class F, class Args, std::size_t... I>
VariantFunction makeVariantFunctionImpl(F fn, std::index_sequence<I...>)
{
    return [fn = std::move(fn)](const QVariantList &args) -> QVariant {
        (void) args;
        using Return = typename Signature<F>::Return;
        if constexpr (std::is_void_v<Return>) {
            fn(fromVariant<std::tuple_element_t<I, Args>>(args.value(int(I)))...);
            return {};
        } else {
            return toVariant(fn(fromVariant<std::tuple_element_t<I, Args>>(args.value(int(I)))...));
        }
    };
}

template <class F>
VariantFunction makeVariantFunction(F &&fn)
{
    using Args = typename Signature<std::decay_t<F>>::Args;
    return makeVariantFunctionImpl<std::decay_t<F>, Args>(
        std::forward<F>(fn), std::make_index_sequence<std::tuple_size_v<Args>>{});
}

// Populates a JavaScript object with bound C++ callables. set() covers the
// common fixed-signature case (types deduced from the callable); setRaw() is the
// escape hatch for overloaded or variadic functions that inspect the arguments
// themselves.
class Registrar
{
public:
    explicit Registrar(std::function<void(const char *, VariantFunction)> sink)
        : m_sink(std::move(sink))
    {}

    template <class F>
    void set(const char *name, F &&fn) { m_sink(name, makeVariantFunction(std::forward<F>(fn))); }

    void setRaw(const char *name, VariantFunction fn) { m_sink(name, std::move(fn)); }

private:
    std::function<void(const char *, VariantFunction)> m_sink;
};

} // namespace Core::JsBind

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

#include <tuple>

namespace Utils {

/*
    struct functionTraits<Function>
    {
        using ResultType; // Return type of Function
        struct argument<unsigned index>
        {
            using type; // type of Function argument at index (starting with 0)
        }
    }

    struct functionTakesArgument<Function, unsigned index, ArgumentType>;

    Is derived from std::true_type if Function takes an argument of type ArgumentType at index.
    Otherwise derived from std::false_type.
*/

////////////////////
// functionTraits
////////////////////

// for callables. defined below.
template <typename Callable>
struct functionTraits;

// function
template <typename Result, typename... Args>
struct functionTraits<Result(Args...)>
{
    using ResultType = Result;
    static const unsigned arity = sizeof...(Args); // TODO const -> constexpr with MSVC2015

    template <unsigned i>
    struct argument
    {
        using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
    };
};

// function pointer
template <typename Result, typename... Args>
struct functionTraits<Result(*)(Args...)> : public functionTraits<Result(Args...)>
{
};

// const function pointer
template <typename Result, typename... Args>
struct functionTraits<Result(* const)(Args...)> : public functionTraits<Result(Args...)>
{
};

// member function
template <typename Type, typename Result, typename... Args>
struct functionTraits<Result(Type::*)(Args...)> : public functionTraits<Result(Type&,Args...)>
{
};

// const member function
template <typename Type, typename Result, typename... Args>
struct functionTraits<Result(Type::*)(Args...) const> : public functionTraits<Result(Type&,Args...)>
{
};

// const pointer to member function
template <typename Type, typename Result, typename... Args>
struct functionTraits<Result(Type::* const)(Args...)> : public functionTraits<Result(Type&,Args...)>
{
};

// const pointer to const member function
template <typename Type, typename Result, typename... Args>
struct functionTraits<Result(Type::* const)(Args...) const> : public functionTraits<Result(Type&,Args...)>
{
};

// TODO: enable lvalue and rvalue ref member function later (MSVC 2015?)
//// lvalue ref member function
//template <typename Type, typename Result, typename... Args>
//struct functionTraits<Result(Type::*)(Args...) &> : public functionTraits<Result(Type&,Args...)>
//{
//};

//// const lvalue ref member function
//template <typename Type, typename Result, typename... Args>
//struct functionTraits<Result(Type::*)(Args...) const &> : public functionTraits<Result(Type&,Args...)>
//{
//};

//// rvalue ref member function
//template <typename Type, typename Result, typename... Args>
//struct functionTraits<Result(Type::*)(Args...) &&> : public functionTraits<Result(Type&,Args...)>
//{
//};

// callables. only works if operator() is not overloaded.
template <typename Callable>
struct functionTraits
{
    using callableTraits = functionTraits<decltype(&Callable::operator())>;
    using ResultType = typename callableTraits::ResultType;
    static const unsigned arity = callableTraits::arity - 1; // ignore object pointer arg // TODO const -> constexpr with MSVC2015

    template <unsigned i>
    struct argument
    {
        using type = typename callableTraits::template argument<i+1>::type; // ignore object pointer arg
    };
};

// lvalue ref callables
template <typename Callable>
struct functionTraits<Callable&> : public functionTraits<Callable>
{
};

// const lvalue ref callables
template <typename Callable>
struct functionTraits<const Callable&> : public functionTraits<Callable>
{
};

// rvalue ref callables
template <typename Callable>
struct functionTraits<Callable&&> : public functionTraits<Callable>
{
};

template <typename F>
using functionResult_t = typename functionTraits<F>::ResultType;

////////////////////
// functionTakesArgument
////////////////////
namespace Internal {

template <typename HasArity/*true_type or false_type*/,
          typename Function, unsigned index, typename T>
struct functionTakesArgumentArityDispatch;

template <typename Function, unsigned index, typename T>
struct functionTakesArgumentArityDispatch<std::false_type, Function, index, T>
        : public std::false_type
{
};

template <typename Function, unsigned index, typename T>
struct functionTakesArgumentArityDispatch<std::true_type, Function, index, T>
        : public std::is_same<T, typename functionTraits<Function>::template argument<index>::type>
{
};

} // Internal

template <typename Function, unsigned index, typename T>
struct functionTakesArgument : public Internal::functionTakesArgumentArityDispatch<
        std::integral_constant<bool, (functionTraits<Function>::arity > index)>,
        Function, index, T>
{
};

} // Utils

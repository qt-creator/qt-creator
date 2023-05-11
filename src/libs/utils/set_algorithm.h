// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <algorithm>

namespace Utils {

template<class Callable>
class function_output_iterator
{
public:
    typedef std::output_iterator_tag iterator_category;
    typedef void value_type;
    typedef void difference_type;
    typedef void pointer;
    typedef void reference;

    explicit function_output_iterator() {}

    explicit function_output_iterator(const Callable &callable)
        : m_callable(&callable)
    {}

    struct helper
    {
        helper(const Callable *callable)
            : m_callable(callable)
        {}
        template<class T>
        helper &operator=(T &&value)
        {
            (*m_callable)(std::forward<T>(value));
            return *this;
        }
        const Callable *m_callable;
    };

    helper operator*() { return helper(m_callable); }
    function_output_iterator &operator++() { return *this; }
    function_output_iterator &operator++(int) { return *this; }

private:
    const Callable *m_callable;
};

template<typename Callable>
function_output_iterator<Callable> make_iterator(const Callable &callable)
{
    return function_output_iterator<Callable>(callable);
}

template<class InputIt1, class InputIt2, class Callable, class Compare>
bool set_intersection_compare(
    InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, Callable call, Compare comp)
{
    while (first1 != last1 && first2 != last2) {
        if (comp(*first1, *first2)) {
            ++first1;
        } else {
            if (!comp(*first2, *first1)) {
                if constexpr (std::is_void_v<std::invoke_result_t<Callable,
                                                                  decltype(*first1),
                                                                  decltype(*first2)>>) {
                    call(*first1, *first2);
                    ++first1;
                } else {
                    auto success = call(*first1, *first2);
                    ++first1;
                    if (success)
                        return true;
                }
            }
            ++first2;
        }
    }

    return false;
}

template<class InputIt1, class InputIt2, class Callable, class Compare>
bool set_greedy_intersection_compare(
    InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, Callable call, Compare comp)
{
    while (first1 != last1 && first2 != last2) {
        if (comp(*first1, *first2)) {
            ++first1;
        } else {
            if (!comp(*first2, *first1)) {
                if constexpr (std::is_void_v<std::invoke_result_t<Callable,
                                                                  decltype(*first1),
                                                                  decltype(*first2)>>) {
                    call(*first1, *first2);
                    ++first1;
                } else {
                    auto success = call(*first1, *first2);
                    ++first1;
                    if (success)
                        return true;
                }
            } else {
                ++first2;
            }
        }
    }

    return false;
}

template<typename InputIt1, typename InputIt2, typename OutputIt>
constexpr OutputIt set_greedy_intersection(
    InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, OutputIt result)
{
    while (first1 != last1 && first2 != last2)
        if (*first1 < *first2)
            ++first1;
        else if (*first2 < *first1)
            ++first2;
        else {
            *result = *first1;
            ++first1;
            ++result;
        }
    return result;
}

template<class InputIt1, class InputIt2, class Callable, class Compare>
void set_greedy_difference(
    InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, Callable call, Compare comp)
{
    while (first1 != last1 && first2 != last2) {
        if (comp(*first1, *first2)) {
            call(*first1++);
        } else if (comp(*first2, *first1)) {
            ++first2;
        } else {
            ++first1;
        }
    }

    while (first1 != last1)
        call(*first1++);
}

template<typename InputIt1, typename InputIt2, typename BinaryPredicate, typename Callable, typename Value>
Value mismatch_collect(InputIt1 first1,
                       InputIt1 last1,
                       InputIt2 first2,
                       InputIt2 last2,
                       Value value,
                       BinaryPredicate predicate,
                       Callable callable)
{
    while (first1 != last1 && first2 != last2) {
        if (predicate(*first1, *first2))
            value = callable(*first1, *first2, value);
        ++first1, ++first2;
    }

    return value;
}

} // namespace Utils

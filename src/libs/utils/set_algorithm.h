// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <ranges>

namespace Utils {

template<class Callable>
class function_output_iterator
{
public:
    typedef std::output_iterator_tag iterator_category;
    typedef void value_type;
    typedef std::ptrdiff_t difference_type;
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

template<typename Iterator1,
         typename Iterator2,
         typename Relation = std::ranges::less,
         typename Projection1 = std::identity,
         typename Projection2 = std::identity>
concept callmergeable = std::input_iterator<Iterator1> && std::input_iterator<Iterator2>
                        && std::indirect_strict_weak_order<Relation,
                                                           std::projected<Iterator1, Projection1>,
                                                           std::projected<Iterator2, Projection2>>;

template<typename Iterator1, typename Iterator2, typename OutIterator>
using set_intersection_result = std::ranges::set_intersection_result<Iterator1, Iterator2, OutIterator>;

struct set_greedy_intersection_functor
{
    template<std::input_iterator Iterator1,
             std::sentinel_for<Iterator1> Sentinel1,
             std::input_iterator Iterator2,
             std::sentinel_for<Iterator2> Sentinel2,
             std::invocable<std::iter_value_t<Iterator1> &> Callable,
             typename Comp = std::ranges::less,
             typename Projection1 = std::identity,
             typename Projection2 = std::identity>
        requires callmergeable<Iterator1, Iterator2, Comp, Projection1, Projection2>
    constexpr void operator()(Iterator1 first1,
                              Sentinel1 last1,
                              Iterator2 first2,
                              Sentinel2 last2,
                              Callable &&callable,
                              Comp comp = {},
                              Projection1 proj1 = {},
                              Projection2 proj2 = {}) const
    {
        while (first1 != last1 && first2 != last2)
            if (std::invoke(comp, std::invoke(proj1, *first1), std::invoke(proj2, *first2)))
                ++first1;
            else if (std::invoke(comp, std::invoke(proj2, *first2), std::invoke(proj1, *first1)))
                ++first2;
            else {
                std::invoke(callable, *first1);
                ++first1;
            }
    }

    template<std::ranges::input_range Range1,
             std::ranges::input_range Range2,
             std::invocable<std::iter_value_t<Range1> &> Callable,
             typename Comp = std::ranges::less,
             typename Projection1 = std::identity,
             typename Projection2 = std::identity>
        requires callmergeable<std::ranges::iterator_t<Range1>, std::ranges::iterator_t<Range2>, Comp, Projection1, Projection2>
    constexpr void operator()(Range1 &&range1,
                              Range2 &&range2,
                              Callable callable,
                              Comp comp = {},
                              Projection1 proj1 = {},
                              Projection2 proj2 = {}) const
    {
        (*this)(std::ranges::begin(range1),
                std::ranges::end(range1),
                std::ranges::begin(range2),
                std::ranges::end(range2),
                std::forward<Callable>(callable),
                std::move(comp),
                std::move(proj1),
                std::move(proj2));
    }

    template<std::input_iterator Iterator1,
             std::sentinel_for<Iterator1> Sentinel1,
             std::input_iterator Iterator2,
             std::sentinel_for<Iterator2> Sentinel2,
             std::invocable<std::iter_value_t<Iterator1> &, std::iter_value_t<Iterator2> &> Callable,
             typename Comp = std::ranges::less,
             typename Projection1 = std::identity,
             typename Projection2 = std::identity>
        requires callmergeable<Iterator1, Iterator2, Comp, Projection1, Projection2>
    constexpr void operator()(Iterator1 first1,
                              Sentinel1 last1,
                              Iterator2 first2,
                              Sentinel2 last2,
                              Callable &&callable,
                              Comp comp = {},
                              Projection1 proj1 = {},
                              Projection2 proj2 = {}) const
    {
        while (first1 != last1 && first2 != last2)
            if (std::invoke(comp, std::invoke(proj1, *first1), std::invoke(proj2, *first2)))
                ++first1;
            else if (std::invoke(comp, std::invoke(proj2, *first2), std::invoke(proj1, *first1)))
                ++first2;
            else {
                std::invoke(callable, *first1, *first2);
                ++first1;
            }
    }

    template<std::ranges::input_range Range1,
             std::ranges::input_range Range2,
             std::invocable<std::iter_value_t<Range1> &, std::iter_value_t<Range2> &> Callable,
             typename Comp = std::ranges::less,
             typename Projection1 = std::identity,
             typename Projection2 = std::identity>
        requires callmergeable<std::ranges::iterator_t<Range1>, std::ranges::iterator_t<Range2>, Comp, Projection1, Projection2>
    constexpr void operator()(Range1 &&range1,
                              Range2 &&range2,
                              Callable &&callable,
                              Comp comp = {},
                              Projection1 proj1 = {},
                              Projection2 proj2 = {}) const
    {
        (*this)(std::ranges::begin(range1),
                std::ranges::end(range1),
                std::ranges::begin(range2),
                std::ranges::end(range2),
                std::forward<Callable>(callable),
                std::move(comp),
                std::move(proj1),
                std::move(proj2));
    }
};

inline constexpr set_greedy_intersection_functor set_greedy_intersection{};

struct set_greedy_difference_functor
{
    template<std::input_iterator Iterator1,
             std::sentinel_for<Iterator1> Sentinel1,
             std::input_iterator Iterator2,
             std::sentinel_for<Iterator2> Sentinel2,
             std::invocable<std::iter_value_t<Iterator1> &> Callable,
             typename Comp = std::ranges::less,
             typename Projection1 = std::identity,
             typename Projection2 = std::identity>
        requires callmergeable<Iterator1, Iterator2, Comp, Projection1, Projection2>
    constexpr void operator()(Iterator1 first1,
                              Sentinel1 last1,
                              Iterator2 first2,
                              Sentinel2 last2,
                              Callable &&callable,
                              Comp comp = {},
                              Projection1 proj1 = {},
                              Projection2 proj2 = {}) const
    {
        while (first1 != last1 && first2 != last2) {
            if (std::invoke(comp, std::invoke(proj1, *first1), std::invoke(proj2, *first2))) {
                std::invoke(callable, *first1);
                ++first1;
            } else if (std::invoke(comp, std::invoke(proj2, *first2), std::invoke(proj1, *first1))) {
                ++first2;
            } else {
                ++first1;
            }
        }

        while (first1 != last1) {
            std::invoke(callable, *first1);
            ++first1;
        }
    }

    template<std::ranges::input_range Range1,
             std::ranges::input_range Range2,
             std::invocable<std::iter_value_t<Range1> &> Callable,
             typename Comp = std::ranges::less,
             typename Projection1 = std::identity,
             typename Projection2 = std::identity>
        requires callmergeable<std::ranges::iterator_t<Range1>, std::ranges::iterator_t<Range2>, Comp, Projection1, Projection2>
    constexpr void operator()(Range1 &&range1,
                              Range2 &&range2,
                              Callable &&callable,
                              Comp comp = {},
                              Projection1 proj1 = {},
                              Projection2 proj2 = {}) const
    {
        (*this)(std::ranges::begin(range1),
                std::ranges::end(range1),
                std::ranges::begin(range2),
                std::ranges::end(range2),
                std::forward<Callable>(callable),
                std::move(comp),
                std::move(proj1),
                std::move(proj2));
    }
};

inline constexpr set_greedy_difference_functor set_greedy_difference{};

} // namespace Utils

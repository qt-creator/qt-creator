// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <ranges>

#include <QMetaEnum>

namespace Utils {

namespace ranges {

template<typename ENUMTYPE>
struct MetaEnum
{
    struct Iterator
    {
        using iterator_category = std::forward_iterator_tag;
        using value_type = int;
        using difference_type = std::ptrdiff_t;
        using pointer = int *;
        using reference = int &;

        Iterator() = default;

        Iterator(const QMetaEnum *e, int idx)
            : m_enum(e)
            , m_index(idx)
        {}

        int operator*() const { return m_enum->value(m_index); }
        Iterator &operator++()
        {
            ++m_index;
            return *this;
        }
        Iterator operator++(int) // post-incrementable, returns prev value
        {
            Iterator temp = *this;
            ++*this;
            return temp;
        }

        bool operator!=(const Iterator &other) const
        {
            return m_index != other.m_index && m_enum == other.m_enum;
        }
        bool operator==(const Iterator &other) const
        {
            return m_index == other.m_index && m_enum == other.m_enum;
        }

        const QMetaEnum *m_enum{nullptr};
        int m_index{-1};
    };

    using value_type = int;

    MetaEnum()
        : m_enum(QMetaEnum::fromType<ENUMTYPE>())
    {}
    Iterator begin() const { return Iterator(&m_enum, 0); }
    Iterator end() const { return Iterator(&m_enum, m_enum.keyCount()); }
    size_t size() const { return m_enum.keyCount(); }

    QMetaEnum m_enum;
};

} // namespace ranges

} // namespace Utils

#if defined(__cpp_lib_ranges_cache_latest) && __cpp_lib_ranges >= 202411L
#  include <ranges>

namespace views {
using std::views::cache_latest_view;
}

} // namespace Utils
#else

namespace Utils::ranges {

namespace Internal {
template<typename Type>
constexpr Type &as_lvalue(Type &&type)
{
    return static_cast<Type &>(type);
}

template<class Type>
    requires(std::is_object_v<Type>)
class non_propagating_cache : public std::optional<Type>
{
public:
    using std::optional<Type>::operator=;
    non_propagating_cache() = default;

    constexpr non_propagating_cache(std::nullopt_t) noexcept
        : std::optional<Type>{}
    {}

    constexpr non_propagating_cache(const non_propagating_cache &) noexcept
        : std::optional<Type>{}
    {}

    constexpr non_propagating_cache(non_propagating_cache &&other) noexcept
        : std::optional<Type>{}
    {
        other.reset();
    }

    constexpr non_propagating_cache &operator=(const non_propagating_cache &other) noexcept
    {
        if (std::addressof(other) != this)
            this->reset();
        return *this;
    }

    constexpr non_propagating_cache &operator=(non_propagating_cache &&other) noexcept
    {
        this->reset();
        other.reset();
        return *this;
    }

    template<class Iterator>
    constexpr Type &emplace_deref(const Iterator &i)
    {
        return this->emplace(*i);
    }
};
} // namespace Internal

template<std::ranges::input_range Range>
    requires std::ranges::view<Range>
class cache_latest_view : public std::ranges::view_interface<cache_latest_view<Range>>
{
    using cache_t = std::conditional_t<std::is_reference_v<std::ranges::range_reference_t<Range>>,
                                       std::add_pointer_t<std::ranges::range_reference_t<Range>>,
                                       std::ranges::range_reference_t<Range>>;

    Range m_base = Range{};

    Internal::non_propagating_cache<cache_t> m_cache;

public:
    class Sentinel;

    class Iterator
    {
        cache_latest_view *m_view = nullptr;

        constexpr explicit Iterator(cache_latest_view &view)
            : m_view(std::addressof(view))
            , m_current(std::ranges::begin(view.m_base))
        {}

        friend cache_latest_view;
        friend Sentinel;

    public:
        using difference_type = std::ranges::range_difference_t<Range>;
        using value_type = std::ranges::range_value_t<Range>;
        using iterator_concept = std::input_iterator_tag;

        std::ranges::iterator_t<Range> m_current; // needs to be public because otherwise old compiler complain

        Iterator(Iterator &&) = default;

        Iterator &operator=(Iterator &&) = default;

        constexpr std::ranges::iterator_t<Range> base() && { return std::move(m_current); }

        constexpr const std::ranges::iterator_t<Range> &base() const & noexcept
        {
            return m_current;
        }

        constexpr Iterator &operator++()
        {
            m_view->m_cache.reset();
            ++m_current;
            return *this;
        }

        constexpr void operator++(int) { ++*this; }

        constexpr std::ranges::range_reference_t<Range> &operator*() const
        {
            if constexpr (std::is_reference_v<std::ranges::range_reference_t<Range>>) {
                if (!m_view->m_cache)
                    m_view->m_cache = std::addressof(Internal::as_lvalue(*m_current));
                return **m_view->m_cache;
            } else {
                if (!m_view->m_cache)
                    m_view->m_cache.emplace_deref(m_current);
                return *m_view->m_cache;
            }
        }

        friend constexpr std::ranges::range_rvalue_reference_t<Range> iter_move(
            const Iterator &iterator) noexcept(noexcept(std::ranges::iter_move(iterator.m_current)))
        {
            return std::ranges::iter_move(iterator.m_current);
        }

        friend constexpr void iter_swap(const Iterator &first, const Iterator &second) noexcept(
            noexcept(std::ranges::iter_swap(first.m_current, second.m_current)))
            requires std::indirectly_swappable<std::ranges::iterator_t<Range>>
        {
            std::ranges::iter_swap(first.m_current, second.m_current);
        }
    };

    class Sentinel
    {
        std::ranges::sentinel_t<Range> m_end = std::ranges::sentinel_t<Range>();

        constexpr explicit Sentinel(cache_latest_view &view)
            : m_end(std::ranges::end(view.m_base))
        {}

        friend cache_latest_view;

    public:
        Sentinel() = default;

        constexpr std::ranges::sentinel_t<Range> base() const { return m_end; }

        friend constexpr bool operator==(const Iterator &first, const Sentinel &second)
        {
            return first.m_current == second.m_end;
        }

        friend constexpr std::ranges::range_difference_t<Range> operator-(const Iterator &first,
                                                                          const Sentinel &second)
            requires std::sized_sentinel_for<std::ranges::sentinel_t<Range>, std::ranges::iterator_t<Range>>
        {
            return first.m_current - second.m_end;
        }

        friend constexpr std::ranges::range_difference_t<Range> operator-(const Sentinel &first,
                                                                          const Iterator &second)
            requires std::sized_sentinel_for<std::ranges::sentinel_t<Range>, std::ranges::iterator_t<Range>>
        {
            return first.m_end - second.m_current;
        }
    };

    cache_latest_view()
        requires std::default_initializable<Range>
    = default;

    constexpr explicit cache_latest_view(Range base)
        : m_base(std::move(base))
    {}

    constexpr Range base() const &
        requires std::copy_constructible<Range>
    {
        return m_base;
    }

    constexpr Range base() && { return std::move(m_base); }

    constexpr auto begin() { return Iterator(*this); }

    constexpr auto end() { return Sentinel(*this); }

    constexpr auto size()
        requires std::ranges::sized_range<Range>
    {
        return std::ranges::size(m_base);
    }

    constexpr auto size() const
        requires std::ranges::sized_range<const Range>
    {
        return std::ranges::size(m_base);
    }
};

template<typename Range>
cache_latest_view(Range &&) -> cache_latest_view<std::views::all_t<Range>>;

template<typename Type>
concept can_cache_latest = requires { cache_latest_view(std::declval<Type>()); };

template<class Type>
    requires std::is_class_v<Type> && std::same_as<Type, std::remove_cv_t<Type>>
struct range_adaptor_closure
{};

namespace Internal {
template<class Function>
struct pipeable : Function,
                  range_adaptor_closure<pipeable<Function>>
{
    constexpr explicit pipeable(Function &&function)
        : Function(std::move(function))
    {}
};

template<class Function, class... Functions>
constexpr auto compose(Function &&arg, Functions &&...args)
{
    return [function = std::forward<Function>(arg),
            ... functions = std::forward<Functions>(args)]<class... Arguments>(
               Arguments &&...arguments) mutable
        requires std::invocable<Function, Arguments...>
    {
        if constexpr (sizeof...(Functions)) {
            return compose(std::forward<Functions>(functions)...)(
                std::invoke(std::forward<Function>(function), std::forward<Arguments>(arguments)...));
        } else {
            return std::invoke(std::forward<Function>(function),
                               std::forward<Arguments>(arguments)...);
        }
    };
}
} // namespace Internal
template<class Type>
Type derived_from_range_adaptor_closure(range_adaptor_closure<Type> *);

template<class Type>
concept RangeAdaptorClosure = !std::ranges::range<std::remove_cvref_t<Type>> && requires {
    {
        derived_from_range_adaptor_closure(static_cast<std::remove_cvref_t<Type> *>(nullptr))
    } -> std::same_as<std::remove_cvref_t<Type>>;
};

template<std::ranges::range Range, RangeAdaptorClosure Closure>
    requires std::invocable<Closure, Range>
[[nodiscard]] constexpr decltype(auto) operator|(Range &&range, Closure &&closure) noexcept(
    std::is_nothrow_invocable_v<Closure, Range>)
{
    return std::invoke(std::forward<Closure>(closure), std::forward<Range>(range));
}

template<RangeAdaptorClosure FirstClosure, RangeAdaptorClosure SecondClosure>
    requires std::constructible_from<std::decay_t<FirstClosure>, FirstClosure>
             && std::constructible_from<std::decay_t<SecondClosure>, SecondClosure>
[[nodiscard]] constexpr auto operator|(FirstClosure &&first, SecondClosure &&second) noexcept(
    std::is_nothrow_constructible_v<std::decay_t<FirstClosure>, FirstClosure>
    && std::is_nothrow_constructible_v<std::decay_t<SecondClosure>, SecondClosure>)
{
    return Internal::pipeable(
        Internal::compose(std::forward<FirstClosure>(second), std::forward<SecondClosure>(first)));
}

namespace views {

struct CacheLatestFunctor : range_adaptor_closure<CacheLatestFunctor>
{
#  if defined(__GNUC__) && !defined(__clang__) && __GNUC__ == 10 && __GNUC_MINOR__ < 5
    template<std::ranges::viewable_range Range>
    constexpr auto operator() [[nodiscard]] (Range &&range) const
    {
        return std::forward<Range>(range);
    }
#  else
    template<std::ranges::viewable_range Range>
        requires can_cache_latest<Range>
    constexpr auto operator() [[nodiscard]] (Range &&range) const
    {
        return cache_latest_view(std::forward<Range>(range));
    }
#  endif
};

inline constexpr CacheLatestFunctor cache_latest;
} // namespace views
} // namespace Utils::ranges

namespace Utils::views {
using namespace Utils::ranges::views;
} // namespace Utils::views

#endif

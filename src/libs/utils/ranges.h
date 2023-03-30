// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#if __cplusplus >= 202002L
#include <ranges>

namespace Utils {

namespace ranges {
using std::ranges::reverse_view;
}

namespace views {
using std::views::reverse;
}

} // namespace Utils
#else
#include <stdexcept>

namespace Utils {

namespace ranges {

template <typename Container>
class reverse_view
{
public:
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;
    using Type = value_type;
    using pointer = Type *;
    using const_pointer = const Type *;
    using reference = Type &;
    using const_reference = const Type &;

    using reverse_iterator = typename Container::iterator;
    using iterator = std::reverse_iterator<reverse_iterator>;

    using const_reverse_iterator = typename Container::const_iterator;
    using const_iterator = std::reverse_iterator<const_reverse_iterator>;

    using Iterator = iterator;
    using ConstIterator = const_iterator;

    reverse_view(const Container &k) : d(&k) {}

    const_reverse_iterator rbegin() const noexcept { return d->begin(); }
    const_reverse_iterator rend() const noexcept { return d->end(); }
    const_reverse_iterator crbegin() const noexcept { return d->begin(); }
    const_reverse_iterator crend() const noexcept { return d->end(); }

    const_iterator begin() const noexcept { return const_iterator(rend()); }
    const_iterator end() const noexcept { return const_iterator(rbegin()); }
    const_iterator cbegin() const noexcept { return const_iterator(rend()); }
    const_iterator cend() const noexcept { return const_iterator(rbegin()); }
    const_iterator constBegin() const noexcept { return const_iterator(rend()); }
    const_iterator constEnd() const noexcept { return const_iterator(rbegin()); }

    const_reference front() const noexcept { return *cbegin(); }
    const_reference back() const noexcept { return *crbegin(); }

    [[nodiscard]] size_type size() const noexcept { return d->size(); }
    [[nodiscard]] bool empty() const noexcept { return d->size() == 0; }
    explicit operator bool() const { return d->size(); }

    const_reference operator[](size_type idx) const
    {
        if (idx < size())
            return *(begin() + idx);

        throw std::out_of_range("bad index in reverse side");
    }

private:
    const Container *d;
};
} // namespace ranges

namespace views {

constexpr struct {} reverse;

template <typename T>
inline ranges::reverse_view<T> operator|(const T &container, const decltype(reverse)&)
{
    return ranges::reverse_view(container);
}

} // namsepace views
} // namespace Utils

#endif

// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QPointer>

#include <memory.h>
#include <type_traits>

namespace Utils {

namespace Internal {

template<typename Type>
class UniqueObjectInternalPointer : public QPointer<Type>
{
public:
    using QPointer<Type>::QPointer;

    template<typename UpType,
             typename = std::enable_if_t<std::is_convertible_v<UpType *, Type *>
                                         && !std::is_same_v<std::decay_t<UpType>, std::decay_t<Type>>>>
    UniqueObjectInternalPointer(const UniqueObjectInternalPointer<UpType> &p) noexcept
        : QPointer<Type>{p.data()}
    {}
};

template<typename Type>
struct UniqueObjectPtrDeleter
{
    using pointer = UniqueObjectInternalPointer<Type>;

    constexpr UniqueObjectPtrDeleter() noexcept = default;
    template<typename UpType, typename = std::enable_if_t<std::is_convertible_v<UpType *, Type *>>>
    constexpr UniqueObjectPtrDeleter(const UniqueObjectPtrDeleter<UpType> &) noexcept
    {}

    constexpr void operator()(pointer p) const
    {
        static_assert(!std::is_void_v<Type>, "can't delete pointer to incomplete type");
        static_assert(sizeof(Type) > 0, "can't delete pointer to incomplete type");

        delete p.data();
    }
};

template<typename Type>
struct UniqueObjectPtrLateDeleter
{
    using pointer = UniqueObjectInternalPointer<Type>;

    constexpr UniqueObjectPtrLateDeleter() noexcept = default;
    template<typename UpType, typename = std::enable_if_t<std::is_convertible_v<UpType *, Type *>>>
    constexpr UniqueObjectPtrLateDeleter(const UniqueObjectPtrLateDeleter<UpType> &) noexcept
    {}

    constexpr void operator()(pointer p) const
    {
        static_assert(!std::is_void_v<Type>, "can't delete pointer to incomplete type");
        static_assert(sizeof(Type) > 0, "can't delete pointer to incomplete type");

        p->deleteLater();
    }
};

} // namespace Internal

template<typename Type>
using UniqueObjectPtr = std::unique_ptr<Type, Internal::UniqueObjectPtrDeleter<Type>>;

template<typename Type, typename... Arguments>
auto makeUniqueObjectPtr(Arguments &&...arguments)
{
    return UniqueObjectPtr<Type>{new Type(std::forward<Arguments>(arguments)...)};
}

template<typename Type>
using UniqueObjectLatePtr = std::unique_ptr<Type, Internal::UniqueObjectPtrLateDeleter<Type>>;

template<typename Type, typename... Arguments>
auto makeUniqueObjectLatePtr(Arguments &&...arguments)
{
    return UniqueObjectLatePtr<Type>{new Type(std::forward<Arguments>(arguments)...)};
}
} // namespace Utils

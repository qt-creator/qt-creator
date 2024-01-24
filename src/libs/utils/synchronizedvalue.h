// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <functional>
#include <mutex>
#include <shared_mutex>

namespace Utils {

/*!
    \brief A wrapper that provides thread-safe access to the wrapped type using a read/write mutex.

    Examples:

    \code

    void writeAndGet() {
        SynchronizedValue<QString> synchronizedString;
        // To update the value of the synchronized object, you can use the write function.
        synchronizedString.write([](QString &str) { str = "Hello World"; });

        // If you just need a value from the synchronized object, you can use the get function
        qDebug() << "New value is:" << synchronizedString.get<QString>([](const QString &str) { return str; });
    }

    void read() {
        SynchronizedValue<QPair<QString, QString>> synchronized;

        QString both;

        // If you want to access multiple members of the synchronized object, you can use the read function
        synchronized.read([&both](const QPair<QString, QString> &pair) {
            qDebug() << "First value is:" << pair.first();
            qDebug() << "Second value is:" << pair.second();
            both = pair.first() + pair.second();
            // ...
        });
    }

    // You can use the SynchronizedValue<T>::update() to return whether the value was changed:
    void setString(const QString &newString) {
        const bool wasChanged = m_synchronizedString.update([&newString](QString &str) {
            if (newString == str)
                return false;
            str = newString;
            return true;
        }));

        if (wasChanged)
            emit stringChanged(newString);
    }

    // You can also use a lock type to get access
    void withLocks() {
        SynchronizedValue<QString> synchronizedData;
        *synchronizedData.writeLocked() = "Hello World";
        qDebug() << *synchronizedData.readLocked() << "== Hello World";

        auto lk = synchronizedData.writeLocked();
        assert(lk.ownsLock());
        *lk = "I am locked";
    }

    \endcode
*/
template<typename T>
class SynchronizedValue
{
    template<typename... SV>
    friend std::tuple<typename SV::unique_lock...> synchronize(SV &...sv);

public:
    SynchronizedValue() = default;

    SynchronizedValue(const SynchronizedValue<T> &other)
    {
        std::shared_lock lk(other.mutex);
        value = other.value;
    }

    SynchronizedValue(const T &other)
        : value(other)
    {}

    template <typename U, template<typename> typename LockType>
    class Lock
    {
    public:
        Lock(U &value_, std::shared_mutex &mutex)
            : m_lock(mutex)
            , m_value(value_)
        {}

        Lock(U &value_, std::shared_mutex &mutex, std::try_to_lock_t)
            : m_lock(mutex, std::try_to_lock)
            , m_value(value_)
        {}

        Lock(U &value_, std::shared_mutex &mutex, std::defer_lock_t)
            : m_lock(mutex, std::defer_lock)
            , m_value(value_)
        {}

        Lock(U &value_, std::shared_mutex &mutex, std::adopt_lock_t)
            : m_lock(mutex, std::adopt_lock)
            , m_value(value_)
        {}

        bool ownsLock() const { return m_lock.owns_lock(); }

        void lock() { m_lock.lock(); }
        void unlock() { m_lock.unlock(); }

        U *operator->() const
        {
            Q_ASSERT(ownsLock());
            return &m_value;
        }

        U &operator*() const
        {
            Q_ASSERT(ownsLock());
            return m_value;
        }

    private:
        LockType<std::shared_mutex> m_lock;
        U &m_value;
    };

    using shared_lock = Lock<const T, std::shared_lock>;
    using unique_lock = Lock<T, std::unique_lock>;

    [[nodiscard]] shared_lock readLocked() const { return shared_lock(value, mutex); }
    [[nodiscard]] shared_lock readLocked(std::try_to_lock_t) const
    {
        return shared_lock(value, mutex, std::try_to_lock);
    }

    [[nodiscard]] unique_lock writeLocked() { return unique_lock(value, mutex); }
    [[nodiscard]] unique_lock writeLocked(std::try_to_lock_t)
    {
        return unique_lock(value, mutex, std::try_to_lock);
    }

    //! Call func with a const reference to the wrapped object
    void read(const std::function<void(const T &)> &func) const
    {
        std::shared_lock lk(mutex);
        func(value);
    }

    //! Call func with a const reference to the wrapped object and returns the result of func
    template<typename R>
    [[nodiscard]] R get(const std::function<R(const T &)> &func) const
    {
        std::shared_lock lk(mutex);
        return func(value);
    }

    [[nodiscard]] T get() const
    {
        std::shared_lock lk(mutex);
        return value;
    }

    //! Call func with a mutable reference to the wrapped object
    void write(const std::function<void(T &)> &func)
    {
        std::unique_lock lk(mutex);
        func(value);
    }

    //! Call func with a mutable reference to the wrapped object and returns the result of func
    template<typename R>
    [[nodiscard]] R update(const std::function<R(T &)> &func)
    {
        std::unique_lock lk(mutex);
        return func(value);
    }

    SynchronizedValue<T> &operator=(const SynchronizedValue<T> &other)
    {
        std::unique_lock lk(mutex, std::defer_lock);
        std::shared_lock lkOther(other.mutex, std::defer_lock);
        std::lock(lk, lkOther);
        value = other.value;
        return *this;
    }

    SynchronizedValue<T> &operator=(const T &other)
    {
        std::unique_lock lk(mutex);
        value = other;
        return *this;
    }

    bool operator!=(const SynchronizedValue<T> &rhs) const
    {
        std::shared_lock lk(mutex, std ::defer_lock);
        std::shared_lock lkOther(rhs.mutex, std ::defer_lock);
        std::lock(lk, lkOther);
        return value != rhs.value;
    }

    bool operator==(const SynchronizedValue<T> &rhs) const
    {
        std::shared_lock lk(mutex, std ::defer_lock);
        std::shared_lock lkOther(rhs.mutex, std ::defer_lock);
        std::lock(lk, lkOther);
        return value == rhs.value;
    }

    bool operator<(const SynchronizedValue<T> &rhs) const
    {
        std::shared_lock lk(mutex, std ::defer_lock);
        std::shared_lock lkOther(rhs.mutex, std ::defer_lock);
        std::lock(lk, lkOther);
        return value < rhs.value;
    }

    bool operator<=(const SynchronizedValue<T> &rhs) const
    {
        std::shared_lock lk(mutex, std ::defer_lock);
        std::shared_lock lkOther(rhs.mutex, std ::defer_lock);
        std::lock(lk, lkOther);
        return value <= rhs.value;
    }

    bool operator>(const SynchronizedValue<T> &rhs) const
    {
        std::shared_lock lk(mutex, std ::defer_lock);
        std::shared_lock lkOther(rhs.mutex, std ::defer_lock);
        std::lock(lk, lkOther);
        return value > rhs.value;
    }

    bool operator>=(const SynchronizedValue<T> &rhs) const
    {
        std::shared_lock lk(mutex, std ::defer_lock);
        std::shared_lock lkOther(rhs.mutex, std ::defer_lock);
        std::lock(lk, lkOther);
        return value >= rhs.value;
    }

    bool operator>(const T &rhs) const
    {
        std::shared_lock lk(mutex);
        return value > rhs;
    }

    bool operator>=(const T &rhs) const
    {
        std::shared_lock lk(mutex);
        return value >= rhs;
    }

    bool operator!=(const T &rhs) const
    {
        std::shared_lock lk(mutex);
        return value != rhs;
    }

    bool operator==(const T &rhs) const
    {
        std::shared_lock lk(mutex);
        return value == rhs;
    }

    bool operator<(const T &rhs) const
    {
        std::shared_lock lk(mutex);
        return value < rhs;
    }

    bool operator<=(const T &rhs) const
    {
        std::shared_lock lk(mutex);
        return value <= rhs;
    }

private:
    template<typename L>
    friend bool operator!=(const L &lhs, const SynchronizedValue<T> &rhs)
    {
        return rhs != lhs;
    }

    template<typename L>
    friend bool operator==(const L &lhs, const SynchronizedValue<T> &rhs)
    {
        return rhs == lhs;
    }

    template<typename L>
    friend bool operator<(const L &lhs, const SynchronizedValue<T> &rhs)
    {
        return rhs > lhs;
    }

    template<typename L>
    friend bool operator<=(const L &lhs, const SynchronizedValue<T> &rhs)
    {
        return rhs >= lhs;
    }

    template<typename L>
    friend bool operator>(const L &lhs, const SynchronizedValue<T> &rhs)
    {
        return rhs < lhs;
    }

    template<typename L>
    friend bool operator>=(const L &lhs, const SynchronizedValue<T> &rhs)
    {
        return rhs <= lhs;
    }

private:
    mutable std::shared_mutex mutex;
    T value;
};

//! Lock a number of SynchronizedValue's using a dead-lock free algorithm. ( see std::lock() )
template<typename... SV>
std::tuple<typename SV::unique_lock...> synchronize(SV &...sv)
{
    std::lock(sv.mutex...);
    typedef std::tuple<typename SV::unique_lock...> t_type;
    return t_type(typename SV::unique_lock(sv.value, sv.mutex, std::adopt_lock)...);
}

} // namespace Utils

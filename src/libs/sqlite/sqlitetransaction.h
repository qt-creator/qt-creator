// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sourcelocation.h"
#include "sqliteglobal.h"

#include <utils/smallstringview.h>

#include <exception>
#include <mutex>

namespace Sqlite {

class DatabaseBackend;
class Database;

class TransactionInterface
{
public:
    TransactionInterface() = default;
    TransactionInterface(const TransactionInterface &) = delete;
    TransactionInterface &operator=(const TransactionInterface &) = delete;

    virtual void deferredBegin(const source_location &sourceLocation) = 0;
    virtual void immediateBegin(const source_location &sourceLocation) = 0;
    virtual void exclusiveBegin(const source_location &sourceLocation) = 0;
    virtual void commit(const source_location &sourceLocation) = 0;
    virtual void rollback(const source_location &sourceLocation) = 0;
    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual void immediateSessionBegin(const source_location &sourceLocation) = 0;
    virtual void sessionCommit(const source_location &sourceLocation) = 0;
    virtual void sessionRollback(const source_location &sourceLocation) = 0;

protected:
    ~TransactionInterface() = default;
};

template<typename TransactionInterface>
class AbstractTransaction
{
public:
    using Transaction = TransactionInterface;

    AbstractTransaction(const AbstractTransaction &) = delete;
    AbstractTransaction &operator=(const AbstractTransaction &) = delete;

    void commit(const source_location &sourceLocation = source_location::current())
    {
        m_interface.commit(sourceLocation);
        m_isAlreadyCommited = true;
        m_locker.unlock();
    }

protected:
    ~AbstractTransaction() = default;

    AbstractTransaction(TransactionInterface &transactionInterface)
        : m_interface(transactionInterface)
    {
    }

protected:
    TransactionInterface &m_interface;
    std::unique_lock<TransactionInterface> m_locker{m_interface};
    bool m_isAlreadyCommited = false;
    bool m_rollback = false;
};

template<typename TransactionInterface>
class ImplicitTransaction
{
public:
    using Transaction = TransactionInterface;

    ~ImplicitTransaction() = default;
    ImplicitTransaction(TransactionInterface &transactionInterface)
        : m_locker(transactionInterface)
    {}

    ImplicitTransaction(const ImplicitTransaction &) = delete;
    ImplicitTransaction &operator=(const ImplicitTransaction &) = delete;

protected:
    std::unique_lock<TransactionInterface> m_locker;
};

template<typename TransactionInterface>
class AbstractThrowingSessionTransaction
{
public:
    using Transaction = TransactionInterface;

    AbstractThrowingSessionTransaction(const AbstractThrowingSessionTransaction &) = delete;
    AbstractThrowingSessionTransaction &operator=(const AbstractThrowingSessionTransaction &) = delete;

    void commit(const source_location &sourceLocation = source_location::current())
    {
        m_interface.sessionCommit(sourceLocation);
        m_isAlreadyCommited = true;
        m_locker.unlock();
    }

    ~AbstractThrowingSessionTransaction() noexcept(false)
    {
        try {
            if (m_rollback)
                m_interface.sessionRollback(m_sourceLocation);
        } catch (...) {
            if (!std::uncaught_exceptions())
                throw;
        }
    }

protected:
    AbstractThrowingSessionTransaction(TransactionInterface &transactionInterface,
                                       const source_location &sourceLocation)
        : m_sourceLocation(sourceLocation)
        , m_interface(transactionInterface)
    {}

protected:
    source_location m_sourceLocation;
    TransactionInterface &m_interface;
    std::unique_lock<TransactionInterface> m_locker{m_interface};
    bool m_isAlreadyCommited = false;
    bool m_rollback = false;
};

template<typename TransactionInterface>
class AbstractThrowingTransaction : public AbstractTransaction<TransactionInterface>
{
    using Base = AbstractTransaction<TransactionInterface>;

public:
    AbstractThrowingTransaction(const AbstractThrowingTransaction &) = delete;
    AbstractThrowingTransaction &operator=(const AbstractThrowingTransaction &) = delete;
    ~AbstractThrowingTransaction() noexcept(false)
    {
        try {
            if (Base::m_rollback)
                Base::m_interface.rollback(m_sourceLocation);
        } catch (...) {
            if (!std::uncaught_exceptions())
                throw;
        }
    }

protected:
    AbstractThrowingTransaction(TransactionInterface &transactionInterface,
                                const source_location &sourceLocation)
        : AbstractTransaction<TransactionInterface>(transactionInterface)
        , m_sourceLocation(sourceLocation)
    {
    }

private:
    source_location m_sourceLocation;
};

template<typename TransactionInterface>
class AbstractNonThrowingDestructorTransaction : public AbstractTransaction<TransactionInterface>
{
    using Base = AbstractTransaction<TransactionInterface>;

public:
    AbstractNonThrowingDestructorTransaction(const AbstractNonThrowingDestructorTransaction &) = delete;
    AbstractNonThrowingDestructorTransaction &operator=(const AbstractNonThrowingDestructorTransaction &) = delete;
    ~AbstractNonThrowingDestructorTransaction()
    {
        try {
            if (Base::m_rollback)
                Base::m_interface.rollback(m_sourceLocation);
        } catch (...) {
        }
    }

protected:
    AbstractNonThrowingDestructorTransaction(TransactionInterface &transactionInterface,
                                             const source_location &sourceLocation)
        : AbstractTransaction<TransactionInterface>(transactionInterface)
        , m_sourceLocation(sourceLocation)

    {
    }

private:
    source_location m_sourceLocation;
};

template<typename BaseTransaction>
class BasicDeferredTransaction : public BaseTransaction
{
public:
    BasicDeferredTransaction(typename BaseTransaction::Transaction &transactionInterface,
                             const source_location &sourceLocation = source_location::current())
        : BaseTransaction(transactionInterface, sourceLocation)
    {
        transactionInterface.deferredBegin(sourceLocation);
    }

    ~BasicDeferredTransaction()
    {
        BaseTransaction::m_rollback = !BaseTransaction::m_isAlreadyCommited;
    }
};

template<typename TransactionInterface>
class DeferredTransaction final
    : public BasicDeferredTransaction<AbstractThrowingTransaction<TransactionInterface>>
{
    using Base = BasicDeferredTransaction<AbstractThrowingTransaction<TransactionInterface>>;

public:
    using Base::Base;
};

template<typename Transaction, typename TransactionInterface, typename Callable>
auto withTransaction(TransactionInterface &transactionInterface,
                     Callable &&callable,
                     const source_location &sourceLocation) -> std::invoke_result_t<Callable>
{
    Transaction transaction{transactionInterface, sourceLocation};

    if constexpr (std::is_void_v<std::invoke_result_t<Callable>>) {
        callable();

        transaction.commit(sourceLocation);
    } else {
        auto results = callable();

        transaction.commit(sourceLocation);

        return results;
    }
}

template<typename TransactionInterface, typename Callable>
auto withImplicitTransaction(TransactionInterface &transactionInterface, Callable &&callable)
{
    ImplicitTransaction transaction{transactionInterface};

    if constexpr (std::is_void_v<std::invoke_result_t<Callable>>) {
        callable();
    } else {
        return callable();
    }
}

template<typename TransactionInterface, typename Callable>
auto withDeferredTransaction(TransactionInterface &transactionInterface,
                             Callable &&callable,
                             const source_location &sourceLocation = source_location::current())
{
    if constexpr (std::is_void_v<std::invoke_result_t<Callable>>) {
        withTransaction<DeferredTransaction<TransactionInterface>>(transactionInterface,
                                                                   std::forward<Callable>(callable),
                                                                   sourceLocation);
    } else {
        return withTransaction<DeferredTransaction<TransactionInterface>>(transactionInterface,
                                                                          std::forward<Callable>(
                                                                              callable),
                                                                          sourceLocation);
    }
}

template<typename TransactionInterface>
DeferredTransaction(TransactionInterface &, const source_location &)
    -> DeferredTransaction<TransactionInterface>;
template<typename TransactionInterface>
DeferredTransaction(TransactionInterface &) -> DeferredTransaction<TransactionInterface>;

template<typename TransactionInterface>
class DeferredNonThrowingDestructorTransaction final
    : public BasicDeferredTransaction<AbstractNonThrowingDestructorTransaction<TransactionInterface>>
{
    using Base = BasicDeferredTransaction<AbstractNonThrowingDestructorTransaction<TransactionInterface>>;

public:
    using Base::Base;
};

template<typename TransactionInterface>
DeferredNonThrowingDestructorTransaction(TransactionInterface &, const source_location &)
    -> DeferredNonThrowingDestructorTransaction<TransactionInterface>;
template<typename TransactionInterface>
DeferredNonThrowingDestructorTransaction(TransactionInterface &)
    -> DeferredNonThrowingDestructorTransaction<TransactionInterface>;

template<typename BaseTransaction>
class BasicImmediateTransaction : public BaseTransaction
{
public:
    BasicImmediateTransaction(typename BaseTransaction::Transaction &transactionInterface,
                              const source_location &sourceLocation = source_location::current())
        : BaseTransaction(transactionInterface, sourceLocation)
    {
        transactionInterface.immediateBegin(sourceLocation);
    }

    ~BasicImmediateTransaction()
    {
        BaseTransaction::m_rollback = !BaseTransaction::m_isAlreadyCommited;
    }
};

template<typename TransactionInterface>
class ImmediateTransaction final
    : public BasicImmediateTransaction<AbstractThrowingTransaction<TransactionInterface>>
{
    using Base = BasicImmediateTransaction<AbstractThrowingTransaction<TransactionInterface>>;

public:
    using Base::Base;
};

template<typename TransactionInterface, typename Callable>
auto withImmediateTransaction(TransactionInterface &transactionInterface,
                              Callable &&callable,
                              const source_location &sourceLocation = source_location::current())
{
    if constexpr (std::is_void_v<std::invoke_result_t<Callable>>) {
        withTransaction<ImmediateTransaction<TransactionInterface>>(transactionInterface,
                                                                    std::forward<Callable>(callable),
                                                                    sourceLocation);
    } else {
        return withTransaction<ImmediateTransaction<TransactionInterface>>(transactionInterface,
                                                                           std::forward<Callable>(
                                                                               callable),
                                                                           sourceLocation);
    }
}

template<typename TransactionInterface>
ImmediateTransaction(TransactionInterface &, const source_location &)
    -> ImmediateTransaction<TransactionInterface>;
template<typename TransactionInterface>
ImmediateTransaction(TransactionInterface &) -> ImmediateTransaction<TransactionInterface>;

template<typename TransactionInterface>
class ImmediateNonThrowingDestructorTransaction final
    : public BasicImmediateTransaction<AbstractNonThrowingDestructorTransaction<TransactionInterface>>
{
    using Base = BasicImmediateTransaction<AbstractNonThrowingDestructorTransaction<TransactionInterface>>;

public:
    using Base::Base;
};

template<typename TransactionInterface>
ImmediateNonThrowingDestructorTransaction(TransactionInterface &, const source_location &)
    -> ImmediateNonThrowingDestructorTransaction<TransactionInterface>;
template<typename TransactionInterface>
ImmediateNonThrowingDestructorTransaction(TransactionInterface &)
    -> ImmediateNonThrowingDestructorTransaction<TransactionInterface>;

template<typename BaseTransaction>
class BasicExclusiveTransaction : public BaseTransaction
{
public:
    BasicExclusiveTransaction(typename BaseTransaction::Transaction &transactionInterface,
                              const source_location &sourceLocation = source_location::current())
        : BaseTransaction(transactionInterface, sourceLocation)
    {
        transactionInterface.exclusiveBegin(sourceLocation);
    }

    ~BasicExclusiveTransaction()
    {
        BaseTransaction::m_rollback = !BaseTransaction::m_isAlreadyCommited;
    }
};

template<typename TransactionInterface>
class ExclusiveTransaction final
    : public BasicExclusiveTransaction<AbstractThrowingTransaction<TransactionInterface>>
{
    using Base = BasicExclusiveTransaction<AbstractThrowingTransaction<TransactionInterface>>;

public:
    using Base::Base;
};

template<typename TransactionInterface>
ExclusiveTransaction(TransactionInterface &, const source_location &)
    -> ExclusiveTransaction<TransactionInterface>;
template<typename TransactionInterface>
ExclusiveTransaction(TransactionInterface &) -> ExclusiveTransaction<TransactionInterface>;

template<typename TransactionInterface>
class ExclusiveNonThrowingDestructorTransaction final
    : public BasicExclusiveTransaction<AbstractNonThrowingDestructorTransaction<TransactionInterface>>
{
    using Base = BasicExclusiveTransaction<AbstractNonThrowingDestructorTransaction<TransactionInterface>>;

public:
    using Base::Base;
};

template<typename TransactionInterface>
ExclusiveNonThrowingDestructorTransaction(TransactionInterface &, const source_location &)
    -> ExclusiveNonThrowingDestructorTransaction<TransactionInterface>;
template<typename TransactionInterface>
ExclusiveNonThrowingDestructorTransaction(TransactionInterface &)
    -> ExclusiveNonThrowingDestructorTransaction<TransactionInterface>;

template<typename TransactionInterface>
class ImmediateSessionTransaction final
    : public AbstractThrowingSessionTransaction<TransactionInterface>
{
    using Base = AbstractThrowingSessionTransaction<TransactionInterface>;

public:
    ImmediateSessionTransaction(typename Base::Transaction &transactionInterface,
                                const source_location &sourceLocation = source_location::current())
        : AbstractThrowingSessionTransaction<TransactionInterface>(transactionInterface, sourceLocation)
    {
        transactionInterface.immediateSessionBegin(sourceLocation);
    }

    ~ImmediateSessionTransaction() { Base::m_rollback = !Base::m_isAlreadyCommited; }
};

template<typename TransactionInterface>
ImmediateSessionTransaction(TransactionInterface &, const source_location &)
    -> ImmediateSessionTransaction<TransactionInterface>;
template<typename TransactionInterface>
ImmediateSessionTransaction(TransactionInterface &)
    -> ImmediateSessionTransaction<TransactionInterface>;
} // namespace Sqlite

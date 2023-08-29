// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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

    virtual void deferredBegin() = 0;
    virtual void immediateBegin() = 0;
    virtual void exclusiveBegin() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual void immediateSessionBegin() = 0;
    virtual void sessionCommit() = 0;
    virtual void sessionRollback() = 0;

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

    void commit()
    {
        m_interface.commit();
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

    void commit()
    {
        m_interface.sessionCommit();
        m_isAlreadyCommited = true;
        m_locker.unlock();
    }

    ~AbstractThrowingSessionTransaction() noexcept(false)
    {
        try {
            if (m_rollback)
                m_interface.sessionRollback();
        } catch (...) {
            if (!std::uncaught_exceptions())
                throw;
        }
    }

protected:
    AbstractThrowingSessionTransaction(TransactionInterface &transactionInterface)
        : m_interface(transactionInterface)
    {}

protected:
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
                Base::m_interface.rollback();
        } catch (...) {
            if (!std::uncaught_exceptions())
                throw;
        }
    }

protected:
    AbstractThrowingTransaction(TransactionInterface &transactionInterface)
        : AbstractTransaction<TransactionInterface>(transactionInterface)
    {
    }
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
                Base::m_interface.rollback();
        } catch (...) {
        }
    }

protected:
    AbstractNonThrowingDestructorTransaction(TransactionInterface &transactionInterface)
        : AbstractTransaction<TransactionInterface>(transactionInterface)
    {
    }
};

template<typename BaseTransaction>
class BasicDeferredTransaction : public BaseTransaction
{
public:
    BasicDeferredTransaction(typename BaseTransaction::Transaction &transactionInterface)
        : BaseTransaction(transactionInterface)
    {
        transactionInterface.deferredBegin();
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
auto withTransaction(TransactionInterface &transactionInterface, Callable &&callable)
    -> std::invoke_result_t<Callable>
{
    Transaction transaction{transactionInterface};

    if constexpr (std::is_void_v<std::invoke_result_t<Callable>>) {
        callable();

        transaction.commit();
    } else {
        auto results = callable();

        transaction.commit();

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
auto withDeferredTransaction(TransactionInterface &transactionInterface, Callable &&callable)
{
    if constexpr (std::is_void_v<std::invoke_result_t<Callable>>) {
        withTransaction<DeferredTransaction<TransactionInterface>>(transactionInterface,
                                                                   std::forward<Callable>(callable));
    } else {
        return withTransaction<DeferredTransaction<TransactionInterface>>(transactionInterface,
                                                                          std::forward<Callable>(
                                                                              callable));
    }
}

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
DeferredNonThrowingDestructorTransaction(TransactionInterface &)
    -> DeferredNonThrowingDestructorTransaction<TransactionInterface>;

template<typename BaseTransaction>
class BasicImmediateTransaction : public BaseTransaction
{
public:
    BasicImmediateTransaction(typename BaseTransaction::Transaction &transactionInterface)
        : BaseTransaction(transactionInterface)
    {
        transactionInterface.immediateBegin();
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
auto withImmediateTransaction(TransactionInterface &transactionInterface, Callable &&callable)
{
    if constexpr (std::is_void_v<std::invoke_result_t<Callable>>) {
        withTransaction<ImmediateTransaction<TransactionInterface>>(transactionInterface,
                                                                    std::forward<Callable>(
                                                                        callable));
    } else {
        return withTransaction<ImmediateTransaction<TransactionInterface>>(transactionInterface,
                                                                           std::forward<Callable>(
                                                                               callable));
    }
}

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
ImmediateNonThrowingDestructorTransaction(TransactionInterface &)
    -> ImmediateNonThrowingDestructorTransaction<TransactionInterface>;

template<typename BaseTransaction>
class BasicExclusiveTransaction : public BaseTransaction
{
public:
    BasicExclusiveTransaction(typename BaseTransaction::Transaction &transactionInterface)
        : BaseTransaction(transactionInterface)
    {
        transactionInterface.exclusiveBegin();
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
ExclusiveNonThrowingDestructorTransaction(TransactionInterface &)
    -> ExclusiveNonThrowingDestructorTransaction<TransactionInterface>;

template<typename TransactionInterface>
class ImmediateSessionTransaction final
    : public AbstractThrowingSessionTransaction<TransactionInterface>
{
    using Base = AbstractThrowingSessionTransaction<TransactionInterface>;

public:
    ImmediateSessionTransaction(typename Base::Transaction &transactionInterface)
        : AbstractThrowingSessionTransaction<TransactionInterface>(transactionInterface)
    {
        transactionInterface.immediateSessionBegin();
    }

    ~ImmediateSessionTransaction() { Base::m_rollback = !Base::m_isAlreadyCommited; }
};

template<typename TransactionInterface>
ImmediateSessionTransaction(TransactionInterface &)
    -> ImmediateSessionTransaction<TransactionInterface>;

} // namespace Sqlite

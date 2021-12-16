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

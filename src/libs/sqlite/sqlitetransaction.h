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

class AbstractTransaction
{
public:
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
    AbstractTransaction(TransactionInterface &interface)
        : m_interface(interface)
    {
    }


protected:
    TransactionInterface &m_interface;
    std::unique_lock<TransactionInterface> m_locker{m_interface};
    bool m_isAlreadyCommited = false;
    bool m_rollback = false;
};

class AbstractThrowingSessionTransaction
{
public:
    AbstractThrowingSessionTransaction(const AbstractTransaction &) = delete;
    AbstractThrowingSessionTransaction &operator=(const AbstractTransaction &) = delete;

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
    AbstractThrowingSessionTransaction(TransactionInterface &interface)
        : m_interface(interface)
    {}

protected:
    TransactionInterface &m_interface;
    std::unique_lock<TransactionInterface> m_locker{m_interface};
    bool m_isAlreadyCommited = false;
    bool m_rollback = false;
};

class AbstractThrowingTransaction : public AbstractTransaction
{
public:
    AbstractThrowingTransaction(const AbstractThrowingTransaction &) = delete;
    AbstractThrowingTransaction &operator=(const AbstractThrowingTransaction &) = delete;
    ~AbstractThrowingTransaction() noexcept(false)
    {
        try {
            if (m_rollback)
                m_interface.rollback();
        } catch (...) {
            if (!std::uncaught_exceptions())
                throw;
        }
    }

protected:
    AbstractThrowingTransaction(TransactionInterface &interface)
        : AbstractTransaction(interface)
    {
    }
};

class AbstractNonThrowingDestructorTransaction : public AbstractTransaction
{
public:
    AbstractNonThrowingDestructorTransaction(const AbstractNonThrowingDestructorTransaction &) = delete;
    AbstractNonThrowingDestructorTransaction &operator=(const AbstractNonThrowingDestructorTransaction &) = delete;
    ~AbstractNonThrowingDestructorTransaction()
    {
        try {
            if (m_rollback)
                m_interface.rollback();
        } catch (...) {
        }
    }

protected:
    AbstractNonThrowingDestructorTransaction(TransactionInterface &interface)
        : AbstractTransaction(interface)
    {
    }
};

template <typename BaseTransaction>
class BasicDeferredTransaction final : public BaseTransaction
{
public:
    BasicDeferredTransaction(TransactionInterface &interface)
        : BaseTransaction(interface)
    {
        interface.deferredBegin();
    }

    ~BasicDeferredTransaction()
    {
        BaseTransaction::m_rollback = !BaseTransaction::m_isAlreadyCommited;
    }
};

using DeferredTransaction = BasicDeferredTransaction<AbstractThrowingTransaction>;
using DeferredNonThrowingDestructorTransaction = BasicDeferredTransaction<AbstractNonThrowingDestructorTransaction>;

template <typename BaseTransaction>
class BasicImmediateTransaction final : public BaseTransaction
{
public:
    BasicImmediateTransaction(TransactionInterface &interface)
        : BaseTransaction(interface)
    {
        interface.immediateBegin();
    }

    ~BasicImmediateTransaction()
    {
        BaseTransaction::m_rollback = !BaseTransaction::m_isAlreadyCommited;
    }
};

using ImmediateTransaction = BasicImmediateTransaction<AbstractThrowingTransaction>;
using ImmediateNonThrowingDestructorTransaction = BasicImmediateTransaction<AbstractNonThrowingDestructorTransaction>;

template <typename BaseTransaction>
class BasicExclusiveTransaction final : public BaseTransaction
{
public:
    BasicExclusiveTransaction(TransactionInterface &interface)
        : BaseTransaction(interface)
    {
        interface.exclusiveBegin();
    }

    ~BasicExclusiveTransaction()
    {
        BaseTransaction::m_rollback = !BaseTransaction::m_isAlreadyCommited;
    }
};

using ExclusiveTransaction = BasicExclusiveTransaction<AbstractThrowingTransaction>;
using ExclusiveNonThrowingDestructorTransaction
    = BasicExclusiveTransaction<AbstractNonThrowingDestructorTransaction>;

class ImmediateSessionTransaction final : public AbstractThrowingSessionTransaction
{
public:
    ImmediateSessionTransaction(TransactionInterface &interface)
        : AbstractThrowingSessionTransaction(interface)
    {
        interface.immediateSessionBegin();
    }

    ~ImmediateSessionTransaction()
    {
        AbstractThrowingSessionTransaction::m_rollback
            = !AbstractThrowingSessionTransaction::m_isAlreadyCommited;
    }
};

} // namespace Sqlite

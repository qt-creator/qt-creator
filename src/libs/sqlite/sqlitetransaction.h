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

#include <mutex>

namespace Sqlite {

class DatabaseBackend;
class Database;

class TransactionInterface
{
public:
    TransactionInterface() = default;
    virtual ~TransactionInterface();
    TransactionInterface(const TransactionInterface &) = delete;
    TransactionInterface &operator=(const TransactionInterface &) = delete;

    virtual void deferredBegin() = 0;
    virtual void immediateBegin() = 0;
    virtual void exclusiveBegin() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
};

class AbstractTransaction
{
public:
    ~AbstractTransaction()
    {
        if (!m_isAlreadyCommited)
            m_interface.rollback();
    }

    AbstractTransaction(const AbstractTransaction &) = delete;
    AbstractTransaction &operator=(const AbstractTransaction &) = delete;

    void commit()
    {
        m_interface.commit();
        m_isAlreadyCommited = true;
    }

protected:
    AbstractTransaction(TransactionInterface &interface)
        : m_interface(interface)
    {
    }

private:
    TransactionInterface &m_interface;
    bool m_isAlreadyCommited = false;
};

class DeferredTransaction final : public AbstractTransaction
{
public:
    DeferredTransaction(TransactionInterface &interface)
        : AbstractTransaction(interface)
    {
        interface.deferredBegin();
    }
};

class ImmediateTransaction final : public AbstractTransaction
{
public:
    ImmediateTransaction(TransactionInterface &interface)
        : AbstractTransaction(interface)
    {
        interface.immediateBegin();
    }
};

class ExclusiveTransaction final : public AbstractTransaction
{
public:
    ExclusiveTransaction(TransactionInterface &interface)
        : AbstractTransaction(interface)
    {
        interface.exclusiveBegin();
    }
};

} // namespace Sqlite

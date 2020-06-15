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

#include <utils/smallstring.h>

#include <exception>
#include <iostream>

namespace Sqlite {

class SQLITE_EXPORT Exception : public std::exception
{
public:
    Exception(const char *whatErrorHasHappen,
              Utils::SmallString &&sqliteErrorMessage = Utils::SmallString())
        : m_whatErrorHasHappen(whatErrorHasHappen)
        , m_sqliteErrorMessage(std::move(sqliteErrorMessage))
    {}

    const char *what() const noexcept override { return m_whatErrorHasHappen; }

    void printWarning() const;

private:
    const char *m_whatErrorHasHappen;
    Utils::SmallString m_sqliteErrorMessage;
};

class StatementIsBusy : public Exception
{
public:
    StatementIsBusy(const char *whatErrorHasHappen,
                    Utils::SmallString &&sqliteErrorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(sqliteErrorMessage))
    {
    }
};

class DatabaseIsBusy : public Exception
{
public:
    DatabaseIsBusy(const char *whatErrorHasHappen)
        : Exception(whatErrorHasHappen)
    {
    }
};

class StatementHasError : public Exception
{
public:
    StatementHasError(const char *whatErrorHasHappen,
                      Utils::SmallString &&sqliteErrorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(sqliteErrorMessage))
    {
    }
};

class StatementIsMisused : public Exception
{
public:
    StatementIsMisused(const char *whatErrorHasHappen,
                       Utils::SmallString &&sqliteErrorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(sqliteErrorMessage))
    {
    }
};

class InputOutputError : public Exception
{
public:
    InputOutputError(const char *whatErrorHasHappen)
        : Exception(whatErrorHasHappen)
    {
    }
};

class ConstraintPreventsModification : public Exception
{
public:
    ConstraintPreventsModification(const char *whatErrorHasHappen,
                                   Utils::SmallString &&sqliteErrorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(sqliteErrorMessage))
    {
    }
};

class NoValuesToFetch : public Exception
{
public:
    NoValuesToFetch(const char *whatErrorHasHappen)
        : Exception(whatErrorHasHappen)
    {
    }
};

class ColumnCountDoesNotMatch : public Exception
{
public:
    ColumnCountDoesNotMatch(const char *whatErrorHasHappen)
        : Exception(whatErrorHasHappen)
    {}
};

class BindingIndexIsOutOfRange : public Exception
{
public:
    BindingIndexIsOutOfRange(const char *whatErrorHasHappen,
                             Utils::SmallString &&sqliteErrorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(sqliteErrorMessage))
    {
    }
};

class WrongBindingName : public Exception
{
public:
    WrongBindingName(const char *whatErrorHasHappen)
        : Exception(whatErrorHasHappen)
    {
    }
};

class DatabaseIsNotOpen : public Exception
{
public:
    DatabaseIsNotOpen(const char *whatErrorHasHappen)
        : Exception(whatErrorHasHappen)
    {
    }
};

class DatabaseCannotBeOpened : public Exception
{
public:
    DatabaseCannotBeOpened(const char *whatErrorHasHappen,
                           Utils::SmallString &&errorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(errorMessage))
    {
    }
};

class DatabaseFilePathIsEmpty : public DatabaseCannotBeOpened
{
public:
    DatabaseFilePathIsEmpty(const char *whatErrorHasHappen)
        : DatabaseCannotBeOpened(whatErrorHasHappen)
    {
    }
};

class DatabaseIsAlreadyOpen : public DatabaseCannotBeOpened
{
public:
    DatabaseIsAlreadyOpen(const char *whatErrorHasHappen)
        : DatabaseCannotBeOpened(whatErrorHasHappen)
    {
    }
};

class DatabaseCannotBeClosed : public Exception
{
public:
    DatabaseCannotBeClosed(const char *whatErrorHasHappen)
        : Exception(whatErrorHasHappen)
    {
    }
};

class DatabaseIsAlreadyClosed : public DatabaseCannotBeClosed
{
public:
    DatabaseIsAlreadyClosed(const char *whatErrorHasHappen)
        : DatabaseCannotBeClosed(whatErrorHasHappen)
    {
    }
};

class WrongFilePath : public DatabaseCannotBeOpened
{
public:
    WrongFilePath(const char *whatErrorHasHappen,
                  Utils::SmallString &&errorMessage = Utils::SmallString())
        : DatabaseCannotBeOpened(whatErrorHasHappen, std::move(errorMessage))
    {
    }
};

class PragmaValueNotSet : public Exception
{
public:
    PragmaValueNotSet(const char *whatErrorHasHappen)
        : Exception(whatErrorHasHappen)
    {
    }
};

class NotReadOnlySqlStatement : public Exception
{
public:
    NotReadOnlySqlStatement(const char *whatErrorHasHappen)
        : Exception(whatErrorHasHappen)
    {
    }
};

class NotWriteSqlStatement : public Exception
{
public:
    NotWriteSqlStatement(const char *whatErrorHasHappen)
        : Exception(whatErrorHasHappen)
    {
    }
};

class DeadLock : public Exception
{
public:
    DeadLock(const char *whatErrorHasHappen)
        : Exception(whatErrorHasHappen)
    {
    }
};

class UnknowError : public Exception
{
public:
    UnknowError(const char *whatErrorHasHappen,
                Utils::SmallString &&errorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(errorMessage))
    {
    }
};

class BindingTooBig : public Exception
{
public:
    BindingTooBig(const char *whatErrorHasHappen,
                  Utils::SmallString &&errorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(errorMessage))
    {
    }
};

class TooBig : public Exception
{
public:
    TooBig(const char *whatErrorHasHappen, Utils::SmallString &&errorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(errorMessage))
    {}
};

class CannotConvert : public Exception
{
public:
    CannotConvert(const char *whatErrorHasHappen)
        : Exception(whatErrorHasHappen)
    {}
};

class ForeignKeyColumnIsNotUnique : public Exception
{
public:
    ForeignKeyColumnIsNotUnique(const char *whatErrorHasHappen)
        : Exception(whatErrorHasHappen)
    {}
};

class CannotApplyChangeSet : public Exception
{
public:
    CannotApplyChangeSet(const char *whatErrorHasHappen)
        : Exception(whatErrorHasHappen)
    {}
};

class ChangeSetIsMisused : public Exception
{
public:
    ChangeSetIsMisused(const char *whatErrorHasHappen)
        : Exception(whatErrorHasHappen)
    {}
};

class SchemeChangeError : public Exception
{
public:
    SchemeChangeError(const char *whatErrorHasHappen,
                      Utils::SmallString &&errorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(errorMessage))
    {}
};

class CannotWriteToReadOnlyConnection : public Exception
{
public:
    CannotWriteToReadOnlyConnection(const char *whatErrorHasHappen,
                                    Utils::SmallString &&errorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(errorMessage))
    {}
};

class ProtocolError : public Exception
{
public:
    ProtocolError(const char *whatErrorHasHappen,
                  Utils::SmallString &&errorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(errorMessage))
    {}
};

class DatabaseExceedsMaximumFileSize : public Exception
{
public:
    DatabaseExceedsMaximumFileSize(const char *whatErrorHasHappen,
                                   Utils::SmallString &&errorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(errorMessage))
    {}
};

class DataTypeMismatch : public Exception
{
public:
    DataTypeMismatch(const char *whatErrorHasHappen,
                     Utils::SmallString &&errorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(errorMessage))
    {}
};

class ConnectionIsLocked : public Exception
{
public:
    ConnectionIsLocked(const char *whatErrorHasHappen,
                       Utils::SmallString &&errorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(errorMessage))
    {}
};

class ExecutionInterrupted : public Exception
{
public:
    ExecutionInterrupted(const char *whatErrorHasHappen,
                         Utils::SmallString &&errorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(errorMessage))
    {}
};

class DatabaseIsCorrupt : public Exception
{
public:
    DatabaseIsCorrupt(const char *whatErrorHasHappen,
                      Utils::SmallString &&errorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(errorMessage))
    {}
};

class CannotOpen : public Exception
{
public:
    CannotOpen(const char *whatErrorHasHappen,
               Utils::SmallString &&errorMessage = Utils::SmallString())
        : Exception(whatErrorHasHappen, std::move(errorMessage))
    {}
};
} // namespace Sqlite

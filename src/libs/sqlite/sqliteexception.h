// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqliteglobal.h"

#include <utils/smallstring.h>

#include <exception>
#include <iostream>

namespace Sqlite {

class SQLITE_EXPORT Exception : public std::exception
{
public:
    Exception(const char *whatErrorHasHappen)
        : m_whatErrorHasHappen(whatErrorHasHappen)
    {}

    const char *what() const noexcept override { return m_whatErrorHasHappen; }

private:
    const char *m_whatErrorHasHappen;
};

class SQLITE_EXPORT ExceptionWithMessage : public Exception
{
public:
    ExceptionWithMessage(const char *whatErrorHasHappen,
                         Utils::SmallString &&sqliteErrorMessage = Utils::SmallString{})
        : Exception{whatErrorHasHappen}
        , m_sqliteErrorMessage(std::move(sqliteErrorMessage))
    {}

    void printWarning() const;

private:
    Utils::SmallString m_sqliteErrorMessage;
};

class StatementIsBusy : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class DatabaseIsBusy : public Exception
{
public:
    using Exception::Exception;
};

class StatementHasError : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class StatementIsMisused : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class InputOutputError : public Exception
{
public:
    using Exception::Exception;
};

class ConstraintPreventsModification : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class NoValuesToFetch : public Exception
{
public:
    using Exception::Exception;
};

class BindingIndexIsOutOfRange : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class WrongBindingName : public Exception
{
public:
    using Exception::Exception;
};

class DatabaseIsNotOpen : public Exception
{
public:
    using Exception::Exception;
};

class DatabaseCannotBeOpened : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class DatabaseFilePathIsEmpty : public DatabaseCannotBeOpened
{
public:
    using DatabaseCannotBeOpened::DatabaseCannotBeOpened;
};

class DatabaseIsAlreadyOpen : public DatabaseCannotBeOpened
{
public:
    using DatabaseCannotBeOpened::DatabaseCannotBeOpened;
};

class DatabaseCannotBeClosed : public Exception
{
public:
    using Exception::Exception;
};

class DatabaseIsAlreadyClosed : public DatabaseCannotBeClosed
{
public:
    using DatabaseCannotBeClosed::DatabaseCannotBeClosed;
};

class WrongFilePath : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class PragmaValueNotSet : public Exception
{
public:
    using Exception::Exception;
};

class NotReadOnlySqlStatement : public Exception
{
public:
    using Exception::Exception;
};

class NotWriteSqlStatement : public Exception
{
public:
    using Exception::Exception;
};

class DeadLock : public Exception
{
public:
    using Exception::Exception;
};

class UnknowError : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class BindingTooBig : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class TooBig : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class CannotConvert : public Exception
{
public:
    using Exception::Exception;
};

class ForeignKeyColumnIsNotUnique : public Exception
{
public:
    using Exception::Exception;
};

class CannotApplyChangeSet : public Exception
{
public:
    using Exception::Exception;
};

class ChangeSetIsMisused : public Exception
{
public:
    using Exception::Exception;
};

class SchemeChangeError : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class CannotWriteToReadOnlyConnection : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class ProtocolError : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class DatabaseExceedsMaximumFileSize : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class DataTypeMismatch : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class ConnectionIsLocked : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class ExecutionInterrupted : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class DatabaseIsCorrupt : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class CannotOpen : public ExceptionWithMessage
{
public:
    using ExceptionWithMessage::ExceptionWithMessage;
};

class CannotCreateChangeSetIterator : public Exception
{
public:
    using Exception::Exception;
};

class CannotGetChangeSetOperation : public Exception
{
public:
    using Exception::Exception;
};

class ChangeSetTupleIsOutOfRange : public Exception
{
public:
    using Exception::Exception;
};

class ChangeSetTupleIsMisused : public Exception
{
public:
    using Exception::Exception;
};

class UnknownError : public Exception
{
public:
    using Exception::Exception;
};

class DatabaseIsNotLocked : public Exception
{
public:
    using Exception::Exception;
};

class WrongBindingParameterCount : public Exception
{
public:
    using Exception::Exception;
};

class WrongColumnCount : public Exception
{
public:
    using Exception::Exception;
};

} // namespace Sqlite

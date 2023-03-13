// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "sqlitebasestatement.h"

#include "sqlitedatabase.h"
#include "sqlitedatabasebackend.h"
#include "sqliteexception.h"

#include "sqlite.h"

#include <condition_variable>
#include <mutex>

#if defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif

#define CARRAY_INT32 0  /* Data is 32-bit signed integers */
#define CARRAY_INT64 1  /* Data is 64-bit signed integers */
#define CARRAY_DOUBLE 2 /* Data is doubles */
#define CARRAY_TEXT 3   /* Data is char* */

extern "C" int sqlite3_carray_bind(
    sqlite3_stmt *pStmt, int idx, void *aData, int nData, int mFlags, void (*xDestroy)(void *));

namespace Sqlite {

BaseStatement::BaseStatement(Utils::SmallStringView sqlStatement, Database &database)
    : m_compiledStatement(nullptr, deleteCompiledStatement)
    , m_database(database)
{
    prepare(sqlStatement);
}

void BaseStatement::deleteCompiledStatement(sqlite3_stmt *compiledStatement)
{
    sqlite3_finalize(compiledStatement);
}

class UnlockNotification
{
public:
    static void unlockNotifyCallBack(void **arguments, int argumentCount)
    {
        for (int index = 0; index < argumentCount; index++) {
          UnlockNotification *unlockNotification = static_cast<UnlockNotification *>(arguments[index]);
          unlockNotification->wakeupWaitCondition();
        }
    }

    void wakeupWaitCondition()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_fired = 1;
        }
        m_waitCondition.notify_all();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_waitCondition.wait(lock, [&] () { return m_fired; });
    }

private:
    bool m_fired = false;
    std::condition_variable m_waitCondition;
    std::mutex m_mutex;
};

void BaseStatement::waitForUnlockNotify() const
{
    UnlockNotification unlockNotification;
    int resultCode = sqlite3_unlock_notify(sqliteDatabaseHandle(),
                                           UnlockNotification::unlockNotifyCallBack,
                                           &unlockNotification);

    if (resultCode == SQLITE_LOCKED)
        throw DeadLock();

    unlockNotification.wait();
}

void BaseStatement::reset() const noexcept
{
    sqlite3_reset(m_compiledStatement.get());
}

bool BaseStatement::next() const
{
    int resultCode;

    do {
        resultCode = sqlite3_step(m_compiledStatement.get());
        if (resultCode == SQLITE_LOCKED) {
            waitForUnlockNotify();
            sqlite3_reset(m_compiledStatement.get());
        }

    } while (resultCode == SQLITE_LOCKED);

    if (resultCode == SQLITE_ROW)
        return true;
    else if (resultCode == SQLITE_DONE)
        return false;

    checkForStepError(resultCode);
}

void BaseStatement::step() const
{
    next();
}

void BaseStatement::bindNull(int index)
{
    int resultCode = sqlite3_bind_null(m_compiledStatement.get(), index);
    if (resultCode != SQLITE_OK)
        checkForBindingError(resultCode);
}

void BaseStatement::bind(int index, NullValue)
{
    bindNull(index);
}

void BaseStatement::bind(int index, int value)
{
    int resultCode = sqlite3_bind_int(m_compiledStatement.get(), index, value);
    if (resultCode != SQLITE_OK)
        checkForBindingError(resultCode);
}

void BaseStatement::bind(int index, long long value)
{
    int resultCode = sqlite3_bind_int64(m_compiledStatement.get(), index, value);
    if (resultCode != SQLITE_OK)
        checkForBindingError(resultCode);
}

void BaseStatement::bind(int index, double value)
{
    int resultCode = sqlite3_bind_double(m_compiledStatement.get(), index, value);
    if (resultCode != SQLITE_OK)
        checkForBindingError(resultCode);
}

void BaseStatement::bind(int index, void *pointer)
{
    int resultCode = sqlite3_bind_pointer(m_compiledStatement.get(),
                                          index,
                                          pointer,
                                          "carray",
                                          nullptr);
    if (resultCode != SQLITE_OK)
        checkForBindingError(resultCode);
}

void BaseStatement::bind(int index, Utils::span<const int> values)
{
    int resultCode = sqlite3_carray_bind(m_compiledStatement.get(),
                                         index,
                                         const_cast<int *>(values.data()),
                                         static_cast<int>(values.size()),
                                         CARRAY_INT32,
                                         SQLITE_STATIC);
    if (resultCode != SQLITE_OK)
        checkForBindingError(resultCode);
}

void BaseStatement::bind(int index, Utils::span<const long long> values)
{
    int resultCode = sqlite3_carray_bind(m_compiledStatement.get(),
                                         index,
                                         const_cast<long long *>(values.data()),
                                         static_cast<int>(values.size()),
                                         CARRAY_INT64,
                                         SQLITE_STATIC);
    if (resultCode != SQLITE_OK)
        checkForBindingError(resultCode);
}

void BaseStatement::bind(int index, Utils::span<const double> values)
{
    int resultCode = sqlite3_carray_bind(m_compiledStatement.get(),
                                         index,
                                         const_cast<double *>(values.data()),
                                         static_cast<int>(values.size()),
                                         CARRAY_DOUBLE,
                                         SQLITE_STATIC);
    if (resultCode != SQLITE_OK)
        checkForBindingError(resultCode);
}

void BaseStatement::bind(int index, Utils::span<const char *> values)
{
    int resultCode = sqlite3_carray_bind(m_compiledStatement.get(),
                                         index,
                                         values.data(),
                                         static_cast<int>(values.size()),
                                         CARRAY_TEXT,
                                         SQLITE_STATIC);
    if (resultCode != SQLITE_OK)
        checkForBindingError(resultCode);
}

void BaseStatement::bind(int index, Utils::SmallStringView text)
{
    int resultCode = sqlite3_bind_text(m_compiledStatement.get(),
                                       index,
                                       text.data(),
                                       int(text.size()),
                                       SQLITE_STATIC);
    if (resultCode != SQLITE_OK)
        checkForBindingError(resultCode);
}

void BaseStatement::bind(int index, BlobView blobView)
{
    int resultCode = SQLITE_OK;

    if (blobView.empty()) {
        sqlite3_bind_null(m_compiledStatement.get(), index);
    } else {
        resultCode = sqlite3_bind_blob64(m_compiledStatement.get(),
                                         index,
                                         blobView.data(),
                                         blobView.size(),
                                         SQLITE_STATIC);
    }

    if (resultCode != SQLITE_OK)
        checkForBindingError(resultCode);
}

void BaseStatement::bind(int index, const Value &value)
{
    switch (value.type()) {
    case ValueType::Integer:
        bind(index, value.toInteger());
        break;
    case ValueType::Float:
        bind(index, value.toFloat());
        break;
    case ValueType::String:
        bind(index, value.toStringView());
        break;
    case ValueType::Blob:
        bind(index, value.toBlobView());
        break;
    case ValueType::Null:
        bind(index, NullValue{});
        break;
    }
}

void BaseStatement::bind(int index, ValueView value)
{
    switch (value.type()) {
    case ValueType::Integer:
        bind(index, value.toInteger());
        break;
    case ValueType::Float:
        bind(index, value.toFloat());
        break;
    case ValueType::String:
        bind(index, value.toStringView());
        break;
    case ValueType::Blob:
        bind(index, value.toBlobView());
        break;
    case ValueType::Null:
        bind(index, NullValue{});
        break;
    }
}

void BaseStatement::prepare(Utils::SmallStringView sqlStatement)
{
    int resultCode;

    do {
        sqlite3_stmt *sqliteStatement = nullptr;
        resultCode = sqlite3_prepare_v2(sqliteDatabaseHandle(),
                                        sqlStatement.data(),
                                        int(sqlStatement.size()),
                                        &sqliteStatement,
                                        nullptr);
        m_compiledStatement.reset(sqliteStatement);

        if (resultCode == SQLITE_LOCKED)
            waitForUnlockNotify();

    } while (resultCode == SQLITE_LOCKED);


    if (resultCode != SQLITE_OK)
        checkForPrepareError(resultCode);
}

sqlite3 *BaseStatement::sqliteDatabaseHandle() const
{
    return m_database.backend().sqliteDatabaseHandle();
}

void BaseStatement::checkForStepError(int resultCode) const
{
    switch (resultCode) {
    case SQLITE_BUSY_RECOVERY:
        throw StatementIsBusyRecovering(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_BUSY_SNAPSHOT:
        throw StatementIsBusySnapshot(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_BUSY_TIMEOUT:
        throw StatementIsBusyTimeout(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_BUSY:
        throw StatementIsBusy(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_ERROR_MISSING_COLLSEQ:
        throw StatementHasErrorMissingCollatingSequence(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_ERROR_RETRY:
        throw StatementHasErrorRetry(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_ERROR_SNAPSHOT:
        throw StatementHasErrorSnapshot(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_ERROR:
        throw StatementHasError(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_MISUSE:
        throwStatementIsMisused("SqliteStatement::stepStatement: was called inappropriately!");
    case SQLITE_CONSTRAINT_CHECK:
        throw CheckConstraintPreventsModification(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_CONSTRAINT_COMMITHOOK:
        throw CommitHookConstraintPreventsModification(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_CONSTRAINT_DATATYPE:
        throw DataTypeConstraintPreventsModification(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_CONSTRAINT_FOREIGNKEY:
        throw ForeignKeyConstraintPreventsModification(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_CONSTRAINT_FUNCTION:
        throw FunctionConstraintPreventsModification(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_CONSTRAINT_NOTNULL:
        throw NotNullConstraintPreventsModification(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_CONSTRAINT_PINNED:
        throw PinnedConstraintPreventsModification(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_CONSTRAINT_PRIMARYKEY:
        throw PrimaryKeyConstraintPreventsModification(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_CONSTRAINT_ROWID:
        throw RowIdConstraintPreventsModification(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_CONSTRAINT_TRIGGER:
        throw TriggerConstraintPreventsModification(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_CONSTRAINT_UNIQUE:
        throw UniqueConstraintPreventsModification(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_CONSTRAINT_VTAB:
        throw VirtualTableConstraintPreventsModification(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_CONSTRAINT:
        throw ConstraintPreventsModification(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_TOOBIG:
        throwTooBig("SqliteStatement::stepStatement: Some is to bigger than SQLITE_MAX_LENGTH.");
    case SQLITE_SCHEMA:
        throwSchemaChangeError("SqliteStatement::stepStatement: Schema changed but the statement "
                               "cannot be recompiled.");
    case SQLITE_READONLY_CANTINIT:
        throw CannotInitializeReadOnlyConnection(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_READONLY_CANTLOCK:
        throw CannotLockReadOnlyConnection(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_READONLY_DBMOVED:
        throw CannotWriteToMovedDatabase(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_READONLY_DIRECTORY:
        throw CannotCreateLogInReadonlyDirectory(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_READONLY_RECOVERY:
        throw DatabaseNeedsToBeRecovered(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_READONLY_ROLLBACK:
        throw CannotRollbackToReadOnlyConnection(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_READONLY:
        throw CannotWriteToReadOnlyConnection(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_PROTOCOL:
        throwProtocolError(
            "SqliteStatement::stepStatement: Something strang with the file locking happened.");
    case SQLITE_NOMEM:
        throw std::bad_alloc();
    case SQLITE_NOLFS:
        throwDatabaseExceedsMaximumFileSize(
            "SqliteStatement::stepStatement: Database exceeds maximum file size.");
    case SQLITE_MISMATCH:
        throwDataTypeMismatch(
            "SqliteStatement::stepStatement: Most probably you used not an integer for a rowid.");
    case SQLITE_LOCKED_SHAREDCACHE:
        throw ConnectionsSharedCacheIsLocked(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_LOCKED_VTAB:
        throw ConnectionsVirtualTableIsLocked(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_LOCKED:
        throw ConnectionIsLocked(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_IOERR_READ:
        throw InputOutputCannotRead();
    case SQLITE_IOERR_SHORT_READ:
        throw InputOutputCannotShortRead();
    case SQLITE_IOERR_WRITE:
        throw InputOutputCannotWrite();
    case SQLITE_IOERR_FSYNC:
        throw InputOutputCannotSynchronizeFile();
    case SQLITE_IOERR_DIR_FSYNC:
        throw InputOutputCannotSynchronizeDirectory();
    case SQLITE_IOERR_TRUNCATE:
        throw InputOutputCannotTruncate();
    case SQLITE_IOERR_FSTAT:
        throw InputOutputCannotFsStat();
    case SQLITE_IOERR_UNLOCK:
        throw InputOutputCannotUnlock();
    case SQLITE_IOERR_RDLOCK:
        throw InputOutputCannotReadLock();
    case SQLITE_IOERR_DELETE:
        throw InputOutputCannotDelete();
    case SQLITE_IOERR_BLOCKED:
        throw InputOutputBlocked();
    case SQLITE_IOERR_NOMEM:
        throw InputOutputNoMemory();
    case SQLITE_IOERR_ACCESS:
        throw InputOutputCannotAccess();
    case SQLITE_IOERR_CHECKRESERVEDLOCK:
        throw InputOutputCannotCheckReservedLock();
    case SQLITE_IOERR_LOCK:
        throw InputOutputCannotLock();
    case SQLITE_IOERR_CLOSE:
        throw InputOutputCannotClose();
    case SQLITE_IOERR_DIR_CLOSE:
        throw InputOutputCannotCloseDirectory();
    case SQLITE_IOERR_SHMOPEN:
        throw InputOutputCannotOpenSharedMemory();
    case SQLITE_IOERR_SHMSIZE:
        throw InputOutputCannotEnlargeSharedMemory();
    case SQLITE_IOERR_SHMLOCK:
        throw InputOutputCannotLockSharedMemory();
    case SQLITE_IOERR_SHMMAP:
        throw InputOutputCannotMapSharedMemory();
    case SQLITE_IOERR_SEEK:
        throw InputOutputCannotSeek();
    case SQLITE_IOERR_DELETE_NOENT:
        throw InputOutputCannotDeleteNonExistingFile();
    case SQLITE_IOERR_MMAP:
        throw InputOutputCannotMemoryMap();
    case SQLITE_IOERR_GETTEMPPATH:
        throw InputOutputCannotGetTemporaryPath();
    case SQLITE_IOERR_CONVPATH:
        throw InputOutputConvPathFailed();
    case SQLITE_IOERR_VNODE:
        throw InputOutputVNodeError();
    case SQLITE_IOERR_AUTH:
        throw InputOutputCannotAuthenticate();
    case SQLITE_IOERR_BEGIN_ATOMIC:
        throw InputOutputCannotBeginAtomic();
    case SQLITE_IOERR_COMMIT_ATOMIC:
        throw InputOutputCannotCommitAtomic();
    case SQLITE_IOERR_ROLLBACK_ATOMIC:
        throw InputOutputCannotRollbackAtomic();
    case SQLITE_IOERR_DATA:
        throw InputOutputDataError();
    case SQLITE_IOERR_CORRUPTFS:
        throw InputOutputFileSystemIsCorrupt();
    case SQLITE_IOERR:
        throw InputOutputError();
    case SQLITE_INTERRUPT:
        throwExecutionInterrupted("SqliteStatement::stepStatement: Execution was interrupted.");
    case SQLITE_CORRUPT_INDEX:
        throw DatabaseHasCorruptIndex();
    case SQLITE_CORRUPT_SEQUENCE:
        throw DatabaseHasCorruptSequence();
    case SQLITE_CORRUPT_VTAB:
        throw DatabaseHasCorruptVirtualTable();
    case SQLITE_CORRUPT:
        throw DatabaseIsCorrupt();
    case SQLITE_CANTOPEN_CONVPATH:
    case SQLITE_CANTOPEN_DIRTYWAL:
    case SQLITE_CANTOPEN_FULLPATH:
    case SQLITE_CANTOPEN_ISDIR:
    case SQLITE_CANTOPEN_NOTEMPDIR:
    case SQLITE_CANTOPEN_SYMLINK:
    case SQLITE_CANTOPEN:
        throwCannotOpen("SqliteStatement::stepStatement: Cannot open database or temporary file.");
    }

    throwUnknowError("SqliteStatement::stepStatement: unknown error has happened");
}

void BaseStatement::checkForPrepareError(int resultCode) const
{
    switch (resultCode) {
    case SQLITE_BUSY_RECOVERY:
        throw StatementIsBusyRecovering(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_BUSY_SNAPSHOT:
        throw StatementIsBusySnapshot(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_BUSY_TIMEOUT:
        throw StatementIsBusyTimeout(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_BUSY:
        throw StatementIsBusy(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_ERROR_MISSING_COLLSEQ:
        throw StatementHasErrorMissingCollatingSequence(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_ERROR_RETRY:
        throw StatementHasErrorRetry(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_ERROR_SNAPSHOT:
        throw StatementHasErrorSnapshot(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_ERROR:
        throw StatementHasError(sqlite3_errmsg(sqliteDatabaseHandle()));
    case SQLITE_MISUSE:
        throwStatementIsMisused("SqliteStatement::prepareStatement: was called inappropriately!");
    case SQLITE_IOERR_READ:
        throw InputOutputCannotRead();
    case SQLITE_IOERR_SHORT_READ:
        throw InputOutputCannotShortRead();
    case SQLITE_IOERR_WRITE:
        throw InputOutputCannotWrite();
    case SQLITE_IOERR_FSYNC:
        throw InputOutputCannotSynchronizeFile();
    case SQLITE_IOERR_DIR_FSYNC:
        throw InputOutputCannotSynchronizeDirectory();
    case SQLITE_IOERR_TRUNCATE:
        throw InputOutputCannotTruncate();
    case SQLITE_IOERR_FSTAT:
        throw InputOutputCannotFsStat();
    case SQLITE_IOERR_UNLOCK:
        throw InputOutputCannotUnlock();
    case SQLITE_IOERR_RDLOCK:
        throw InputOutputCannotReadLock();
    case SQLITE_IOERR_DELETE:
        throw InputOutputCannotDelete();
    case SQLITE_IOERR_BLOCKED:
        throw InputOutputBlocked();
    case SQLITE_IOERR_NOMEM:
        throw InputOutputNoMemory();
    case SQLITE_IOERR_ACCESS:
        throw InputOutputCannotAccess();
    case SQLITE_IOERR_CHECKRESERVEDLOCK:
        throw InputOutputCannotCheckReservedLock();
    case SQLITE_IOERR_LOCK:
        throw InputOutputCannotLock();
    case SQLITE_IOERR_CLOSE:
        throw InputOutputCannotClose();
    case SQLITE_IOERR_DIR_CLOSE:
        throw InputOutputCannotCloseDirectory();
    case SQLITE_IOERR_SHMOPEN:
        throw InputOutputCannotOpenSharedMemory();
    case SQLITE_IOERR_SHMSIZE:
        throw InputOutputCannotEnlargeSharedMemory();
    case SQLITE_IOERR_SHMLOCK:
        throw InputOutputCannotLockSharedMemory();
    case SQLITE_IOERR_SHMMAP:
        throw InputOutputCannotMapSharedMemory();
    case SQLITE_IOERR_SEEK:
        throw InputOutputCannotSeek();
    case SQLITE_IOERR_DELETE_NOENT:
        throw InputOutputCannotDeleteNonExistingFile();
    case SQLITE_IOERR_MMAP:
        throw InputOutputCannotMemoryMap();
    case SQLITE_IOERR_GETTEMPPATH:
        throw InputOutputCannotGetTemporaryPath();
    case SQLITE_IOERR_CONVPATH:
        throw InputOutputConvPathFailed();
    case SQLITE_IOERR_VNODE:
        throw InputOutputVNodeError();
    case SQLITE_IOERR_AUTH:
        throw InputOutputCannotAuthenticate();
    case SQLITE_IOERR_BEGIN_ATOMIC:
        throw InputOutputCannotBeginAtomic();
    case SQLITE_IOERR_COMMIT_ATOMIC:
        throw InputOutputCannotCommitAtomic();
    case SQLITE_IOERR_ROLLBACK_ATOMIC:
        throw InputOutputCannotRollbackAtomic();
    case SQLITE_IOERR_DATA:
        throw InputOutputDataError();
    case SQLITE_IOERR_CORRUPTFS:
        throw InputOutputFileSystemIsCorrupt();
    case SQLITE_IOERR:
        throw InputOutputError();
    }

    throwUnknowError("SqliteStatement::prepareStatement: unknown error has happened");
}

void BaseStatement::checkForBindingError(int resultCode) const
{
    switch (resultCode) {
        case SQLITE_TOOBIG: throwBingingTooBig("SqliteStatement::bind: string or blob are over size limits(SQLITE_LIMIT_LENGTH)!");
        case SQLITE_RANGE : throwBindingIndexIsOutOfRange("SqliteStatement::bind: binding index is out of range!");
        case SQLITE_NOMEM: throw std::bad_alloc();
        case SQLITE_MISUSE: throwStatementIsMisused("SqliteStatement::bind: was called inappropriately!");
    }

    throwUnknowError("SqliteStatement::bind: unknown error has happened");
}

void BaseStatement::checkBindingParameterCount(int bindingParameterCount) const
{
    if (bindingParameterCount != sqlite3_bind_parameter_count(m_compiledStatement.get()))
        throw WrongBindingParameterCount{};
}

void BaseStatement::checkColumnCount(int columnCount) const
{
    if (columnCount != sqlite3_column_count(m_compiledStatement.get()))
        throw WrongColumnCount{};
}

bool BaseStatement::isReadOnlyStatement() const
{
    return sqlite3_stmt_readonly(m_compiledStatement.get());
}

void BaseStatement::throwStatementIsMisused(const char *) const
{
    throw StatementIsMisused(sqlite3_errmsg(sqliteDatabaseHandle()));
}

void BaseStatement::throwNoValuesToFetch(const char *) const
{
    throw NoValuesToFetch();
}

void BaseStatement::throwBindingIndexIsOutOfRange(const char *) const
{
    throw BindingIndexIsOutOfRange(sqlite3_errmsg(sqliteDatabaseHandle()));
}

void BaseStatement::throwUnknowError(const char *whatHasHappened) const
{
    if (sqliteDatabaseHandle())
        throw UnknowError(sqlite3_errmsg(sqliteDatabaseHandle()));
    else
        throw UnknowError(whatHasHappened);
}

void BaseStatement::throwBingingTooBig(const char *) const
{
    throw BindingTooBig(sqlite3_errmsg(sqliteDatabaseHandle()));
}

void BaseStatement::throwTooBig(const char *) const
{
    throw TooBig{sqlite3_errmsg(sqliteDatabaseHandle())};
}

void BaseStatement::throwSchemaChangeError(const char *) const
{
    throw SchemeChangeError{sqlite3_errmsg(sqliteDatabaseHandle())};
}

void BaseStatement::throwProtocolError(const char *) const
{
    throw ProtocolError{sqlite3_errmsg(sqliteDatabaseHandle())};
}

void BaseStatement::throwDatabaseExceedsMaximumFileSize(const char *) const
{
    throw DatabaseExceedsMaximumFileSize{sqlite3_errmsg(sqliteDatabaseHandle())};
}

void BaseStatement::throwDataTypeMismatch(const char *) const
{
    throw DataTypeMismatch{sqlite3_errmsg(sqliteDatabaseHandle())};
}

void BaseStatement::throwExecutionInterrupted(const char *) const
{
    throw ExecutionInterrupted{};
}

void BaseStatement::throwCannotOpen(const char *) const
{
    throw CannotOpen{};
}

QString BaseStatement::columnName(int column) const
{
    return QString::fromUtf8(sqlite3_column_name(m_compiledStatement.get(), column));
}

Database &BaseStatement::database() const
{
    return m_database;
}

namespace {
template<typename StringType>
StringType textForColumn(sqlite3_stmt *sqlStatment, int column)
{
    const char *text =  reinterpret_cast<const char*>(sqlite3_column_text(sqlStatment, column));
    std::size_t size = std::size_t(sqlite3_column_bytes(sqlStatment, column));

    return StringType(text, size);
}

BlobView blobForColumn(sqlite3_stmt *sqlStatment, int column)
{
    const std::byte *blob = reinterpret_cast<const std::byte *>(
        sqlite3_column_blob(sqlStatment, column));
    std::size_t size = std::size_t(sqlite3_column_bytes(sqlStatment, column));

    return {blob, size};
}

BlobView convertToBlobForColumn(sqlite3_stmt *sqlStatment, int column)
{
    int dataType = sqlite3_column_type(sqlStatment, column);
    if (dataType == SQLITE_BLOB)
        return blobForColumn(sqlStatment, column);

    return {};
}

template<typename StringType>
StringType convertToTextForColumn(sqlite3_stmt *sqlStatment, int column)
{
    int dataType = sqlite3_column_type(sqlStatment, column);
    switch (dataType) {
    case SQLITE_INTEGER:
    case SQLITE_FLOAT:
    case SQLITE3_TEXT:
        return textForColumn<StringType>(sqlStatment, column);
    case SQLITE_BLOB:
    case SQLITE_NULL:
        break;
    }

    return {};
}
} // namespace

Type BaseStatement::fetchType(int column) const
{
    auto dataType = sqlite3_column_type(m_compiledStatement.get(), column);

    switch (dataType) {
    case SQLITE_INTEGER:
        return Type::Integer;
    case SQLITE_FLOAT:
        return Type::Float;
    case SQLITE3_TEXT:
        return Type::Text;
    case SQLITE_BLOB:
        return Type::Blob;
    case SQLITE_NULL:
        return Type::Null;
    }

    return Type::Invalid;
}

int BaseStatement::fetchIntValue(int column) const
{
    return sqlite3_column_int(m_compiledStatement.get(), column);
}

template<>
int BaseStatement::fetchValue<int>(int column) const
{
    return fetchIntValue(column);
}

long BaseStatement::fetchLongValue(int column) const
{
    return long(fetchValue<long long>(column));
}

template<>
long BaseStatement::fetchValue<long>(int column) const
{
    return fetchLongValue(column);
}

long long BaseStatement::fetchLongLongValue(int column) const
{
    return sqlite3_column_int64(m_compiledStatement.get(), column);
}

template<>
long long BaseStatement::fetchValue<long long>(int column) const
{
    return fetchLongLongValue(column);
}

double BaseStatement::fetchDoubleValue(int column) const
{
    return sqlite3_column_double(m_compiledStatement.get(), column);
}

BlobView BaseStatement::fetchBlobValue(int column) const
{
    return convertToBlobForColumn(m_compiledStatement.get(), column);
}

template<>
double BaseStatement::fetchValue<double>(int column) const
{
    return fetchDoubleValue(column);
}

template<typename StringType>
StringType BaseStatement::fetchValue(int column) const
{
    return convertToTextForColumn<StringType>(m_compiledStatement.get(), column);
}

template SQLITE_EXPORT Utils::SmallStringView BaseStatement::fetchValue<Utils::SmallStringView>(
    int column) const;
template SQLITE_EXPORT Utils::SmallString BaseStatement::fetchValue<Utils::SmallString>(
    int column) const;
template SQLITE_EXPORT Utils::PathString BaseStatement::fetchValue<Utils::PathString>(
    int column) const;

Utils::SmallStringView BaseStatement::fetchSmallStringViewValue(int column) const
{
    return fetchValue<Utils::SmallStringView>(column);
}

ValueView BaseStatement::fetchValueView(int column) const
{
    int dataType = sqlite3_column_type(m_compiledStatement.get(), column);
    switch (dataType) {
    case SQLITE_NULL:
        return ValueView::create(NullValue{});
    case SQLITE_INTEGER:
        return ValueView::create(fetchLongLongValue(column));
    case SQLITE_FLOAT:
        return ValueView::create(fetchDoubleValue(column));
    case SQLITE3_TEXT:
        return ValueView::create(fetchValue<Utils::SmallStringView>(column));
    case SQLITE_BLOB:
        break;
    }

    return ValueView::create(NullValue{});
}

} // namespace Sqlite

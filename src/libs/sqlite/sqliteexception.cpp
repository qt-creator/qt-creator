// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sqliteexception.h"

#include <utils/smallstringio.h>

#include <sqlite.h>

#include <QDebug>

namespace Sqlite {

const char *Exception::what() const noexcept
{
    return "Sqlite::Exception";
}

const char *ExceptionWithMessage::what() const noexcept
{
    return "Sqlite::ExceptionWithMessage";
}

void ExceptionWithMessage::printWarning() const
{
    qWarning() << what() << m_sqliteErrorMessage;
}

const char *StatementIsBusy::what() const noexcept
{
    return "Sqlite::StatementIsBusy";
}

const char *DatabaseIsBusy::what() const noexcept
{
    return "Sqlite::DatabaseIsBusy";
}

const char *StatementHasError::what() const noexcept
{
    return "Sqlite::StatementHasError";
}

const char *StatementIsMisused::what() const noexcept
{
    return "Sqlite::StatementIsMisused";
}

const char *InputOutputError::what() const noexcept
{
    return "Sqlite::InputOutputError";
}

const char *ConstraintPreventsModification::what() const noexcept
{
    return "Sqlite::ConstraintPreventsModification";
}

const char *NoValuesToFetch::what() const noexcept
{
    return "Sqlite::NoValuesToFetch";
}

const char *BindingIndexIsOutOfRange::what() const noexcept
{
    return "Sqlite::BindingIndexIsOutOfRange";
}

const char *WrongBindingName::what() const noexcept
{
    return "Sqlite::WrongBindingName";
}

const char *DatabaseIsNotOpen::what() const noexcept
{
    return "Sqlite::DatabaseIsNotOpen";
}

const char *DatabaseCannotBeOpened::what() const noexcept
{
    return "Sqlite::DatabaseCannotBeOpened";
}

const char *DatabaseFilePathIsEmpty::what() const noexcept
{
    return "Sqlite::DatabaseFilePathIsEmpty";
}

const char *DatabaseIsAlreadyOpen::what() const noexcept
{
    return "Sqlite::DatabaseIsAlreadyOpen";
}

const char *DatabaseCannotBeClosed::what() const noexcept
{
    return "Sqlite::DatabaseCannotBeClosed";
}

const char *DatabaseIsAlreadyClosed::what() const noexcept
{
    return "Sqlite::DatabaseIsAlreadyClosed";
}

const char *WrongFilePath::what() const noexcept
{
    return "Sqlite::WrongFilePath";
}

const char *PragmaValueNotSet::what() const noexcept
{
    return "Sqlite::PragmaValueNotSet";
}

const char *NotReadOnlySqlStatement::what() const noexcept
{
    return "Sqlite::NotReadOnlySqlStatement";
}

const char *NotWriteSqlStatement::what() const noexcept
{
    return "Sqlite::NotWriteSqlStatement";
}

const char *DeadLock::what() const noexcept
{
    return "Sqlite::DeadLock";
}

const char *UnknowError::what() const noexcept
{
    return "Sqlite::UnknowError";
}

const char *BindingTooBig::what() const noexcept
{
    return "Sqlite::BindingTooBig";
}

const char *TooBig::what() const noexcept
{
    return "Sqlite::TooBig";
}

const char *CannotConvert::what() const noexcept
{
    return "Sqlite::CannotConvert";
}

const char *ForeignKeyColumnIsNotUnique::what() const noexcept
{
    return "Sqlite::ForeignKeyColumnIsNotUnique";
}

const char *CannotApplyChangeSet::what() const noexcept
{
    return "Sqlite::CannotApplyChangeSet";
}

const char *ChangeSetIsMisused::what() const noexcept
{
    return "Sqlite::ChangeSetIsMisused";
}

const char *SchemeChangeError::what() const noexcept
{
    return "Sqlite::SchemeChangeError";
}

const char *CannotWriteToReadOnlyConnection::what() const noexcept
{
    return "Sqlite::CannotWriteToReadOnlyConnection";
}

const char *ProtocolError::what() const noexcept
{
    return "Sqlite::ProtocolError";
}

const char *DatabaseExceedsMaximumFileSize::what() const noexcept
{
    return "Sqlite::DatabaseExceedsMaximumFileSize";
}

const char *DataTypeMismatch::what() const noexcept
{
    return "Sqlite::DataTypeMismatch";
}

const char *ConnectionIsLocked::what() const noexcept
{
    return "Sqlite::ConnectionIsLocked";
}

const char *ExecutionInterrupted::what() const noexcept
{
    return "Sqlite::ExecutionInterrupted";
}

const char *DatabaseIsCorrupt::what() const noexcept
{
    return "Sqlite::DatabaseIsCorrupt";
}

const char *CannotOpen::what() const noexcept
{
    return "Sqlite::CannotOpen";
}

const char *CannotCreateChangeSetIterator::what() const noexcept
{
    return "Sqlite::CannotCreateChangeSetIterator";
}

const char *CannotGetChangeSetOperation::what() const noexcept
{
    return "Sqlite::CannotGetChangeSetOperation";
}

const char *ChangeSetTupleIsOutOfRange::what() const noexcept
{
    return "Sqlite::ChangeSetTupleIsOutOfRange";
}

const char *ChangeSetTupleIsMisused::what() const noexcept
{
    return "Sqlite::ChangeSetTupleIsMisused";
}

const char *UnknownError::what() const noexcept
{
    return "Sqlite::UnknownError";
}

const char *DatabaseIsNotLocked::what() const noexcept
{
    return "Sqlite::DatabaseIsNotLocked";
}

const char *WrongBindingParameterCount::what() const noexcept
{
    return "Sqlite::WrongBindingParameterCount";
}

const char *WrongColumnCount::what() const noexcept
{
    return "Sqlite::WrongColumnCount";
}

const char *IndexHasNoTableName::what() const noexcept
{
    return "Sqlite::IndexHasNoTableName";
}

const char *IndexHasNoColumns::what() const noexcept
{
    return "Sqlite::IndexHasNoColumns";
}

const char *MultiTheadingCannotBeActivated::what() const noexcept
{
    return "Sqlite::MultiTheadingCannotBeActivated";
}

const char *LoggingCannotBeActivated::what() const noexcept
{
    return "Sqlite::LoggingCannotBeActivated";
}

const char *MemoryMappingCannotBeChanged::what() const noexcept
{
    return "Sqlite::MemoryMappingCannotBeChanged";
}

const char *LibraryCannotBeInitialized::what() const noexcept
{
    return "Sqlite::LibraryCannotBeInitialized";
}

const char *LibraryCannotBeShutdown::what() const noexcept
{
    return "Sqlite::LibraryCannotBeShutdown";
}

const char *LogCannotBeCheckpointed::what() const noexcept
{
    return "Sqlite::LogCannotBeCheckPointed";
}

const char *BusyTimerCannotBeSet::what() const noexcept
{
    return "Sqlite::BusyTimerCannotBeSet";
}

const char *PragmaValueCannotBeTransformed::what() const noexcept
{
    return "Sqlite::PragmaValueCannotBeTransformed";
}

const char *CheckpointIsMisused::what() const noexcept
{
    return "Sqlite::CheckpointIsMisused";
}

const char *InputOutputCannotAuthenticate::what() const noexcept
{
    return "Sqlite::InputOutputCannotAuthenticate";
}

const char *InputOutputCannotBeginAtomic::what() const noexcept
{
    return "Sqlite::InputOutputCannotBeginAtomic";
}

const char *InputOutputBlocked::what() const noexcept
{
    return "Sqlite::InputOutputBlocked";
}

const char *InputOutputCannotCommitAtomic::what() const noexcept
{
    return "Sqlite::InputOutputCannotCommitAtomic";
}

const char *InputOutputCannotRollbackAtomic::what() const noexcept
{
    return "Sqlite::InputOutputCannotRollbackAtomic";
}

const char *InputOutputDataError::what() const noexcept
{
    return "Sqlite::InputOutputDataError";
}

const char *InputOutputFileSystemIsCorrupt::what() const noexcept
{
    return "Sqlite::InputOutputFileSystemIsCorrupt";
}

const char *InputOutputVNodeError::what() const noexcept
{
    return "Sqlite::InputOutputVNodeError";
}

const char *InputOutputConvPathFailed::what() const noexcept
{
    return "Sqlite::InputOutputConvPathFailed";
}

const char *InputOutputCannotGetTemporaryPath::what() const noexcept
{
    return "Sqlite::InputOutputCannotGetTemporaryPath";
}

const char *InputOutputCannotMemoryMap::what() const noexcept
{
    return "Sqlite::InputOutputCannotMemoryMap";
}

const char *InputOutputCannotDeleteNonExistingFile::what() const noexcept
{
    return "Sqlite::InputOutputCannotDeleteNonExistingFile";
}

const char *InputOutputCannotSeek::what() const noexcept
{
    return "Sqlite::InputOutputCannotSeek";
}

const char *InputOutputCannotMapSharedMemory::what() const noexcept
{
    return "Sqlite::InputOutputCannotMapSharedMemory";
}

const char *InputOutputCannotLockSharedMemory::what() const noexcept
{
    return "Sqlite::InputOutputCannotLockSharedMemory";
}

const char *InputOutputCannotEnlargeSharedMemory::what() const noexcept
{
    return "Sqlite::InputOutputCannotEnlargeSharedMemory";
}

const char *InputOutputCannotOpenSharedMemory::what() const noexcept
{
    return "Sqlite::InputOutputCannotOpenSharedMemory";
}

const char *InputOutputCannotCloseDirectory::what() const noexcept
{
    return "Sqlite::InputOutputCannotCloseDirectory";
}

const char *InputOutputCannotClose::what() const noexcept
{
    return "Sqlite::InputOutputCannotClose";
}

const char *InputOutputCannotLock::what() const noexcept
{
    return "Sqlite::InputOutputCannotLock";
}

const char *InputOutputCannotCheckReservedLock::what() const noexcept
{
    return "Sqlite::InputOutputCannotCheckReservedLock";
}

const char *InputOutputCannotAccess::what() const noexcept
{
    return "Sqlite::InputOutputCannotAccess";
}

const char *InputOutputNoMemory::what() const noexcept
{
    return "Sqlite::InputOutputNoMemory";
}

const char *InputOutputCannotDelete::what() const noexcept
{
    return "Sqlite::InputOutputCannotDelete";
}

const char *InputOutputCannotReadLock::what() const noexcept
{
    return "Sqlite::InputOutputCannotReadLock";
}

const char *InputOutputCannotUnlock::what() const noexcept
{
    return "Sqlite::InputOutputCannotUnlock";
}

const char *InputOutputCannotFsStat::what() const noexcept
{
    return "Sqlite::InputOutputCannotFsStat";
}

const char *InputOutputCannotTruncate::what() const noexcept
{
    return "Sqlite::InputOutputCannotTruncate";
}

const char *InputOutputCannotSynchronizeDirectory::what() const noexcept
{
    return "Sqlite::InputOutputCannotFsyncDirectory";
}

const char *InputOutputCannotSynchronizeFile::what() const noexcept
{
    return "Sqlite::InputOutputCannotSynchronizeFile";
}

const char *InputOutputCannotWrite::what() const noexcept
{
    return "Sqlite::InputOutputCannotWrite";
}

const char *InputOutputCannotShortRead::what() const noexcept
{
    return "Sqlite::InputOutputCannotShortRead";
}

const char *InputOutputCannotRead::what() const noexcept
{
    return "Sqlite::InputOutputCannotRead";
}

const char *StatementIsBusyRecovering::what() const noexcept
{
    return "Sqlite::StatementIsBusyRecovering";
}

const char *StatementIsBusySnapshot::what() const noexcept
{
    return "Sqlite::StatementIsBusySnapshot";
}

const char *StatementIsBusyTimeout::what() const noexcept
{
    return "Sqlite::StatementIsBusyTimeout";
}

const char *DatabaseIsBusyRecovering::what() const noexcept
{
    return "Sqlite::DatabaseIsBusyRecovering";
}

const char *DatabaseIsBusySnapshot::what() const noexcept
{
    return "Sqlite::DatabaseIsBusySnapshot";
}

const char *DatabaseIsBusyTimeout::what() const noexcept
{
    return "Sqlite::DatabaseIsBusyTimeout";
}

const char *StatementHasErrorMissingCollatingSequence::what() const noexcept
{
    return "Sqlite::StatementHasErrorMissingCollatingSequence";
}

const char *StatementHasErrorRetry::what() const noexcept
{
    return "Sqlite::StatementHasErrorRetry";
}

const char *StatementHasErrorSnapshot::what() const noexcept
{
    return "Sqlite::StatementHasErrorSnapshot";
}

const char *CheckConstraintPreventsModification::what() const noexcept
{
    return "Sqlite::CheckConstraintPreventsModification";
}

const char *CommitHookConstraintPreventsModification::what() const noexcept
{
    return "Sqlite::CommitHookConstraintPreventsModification";
}

const char *DataTypeConstraintPreventsModification::what() const noexcept
{
    return "Sqlite::DataTypeConstraintPreventsModification";
}

const char *ForeignKeyConstraintPreventsModification::what() const noexcept
{
    return "Sqlite::ForeignKeyConstraintPreventsModification";
}

const char *FunctionConstraintPreventsModification::what() const noexcept
{
    return "Sqlite::FunctionConstraintPreventsModification";
}

const char *NotNullConstraintPreventsModification::what() const noexcept
{
    return "Sqlite::NotNullConstraintPreventsModification";
}

const char *PinnedConstraintPreventsModification::what() const noexcept
{
    return "Sqlite::PinnedConstraintPreventsModification";
}

const char *PrimaryKeyConstraintPreventsModification::what() const noexcept
{
    return "Sqlite::PrimaryKeyConstraintPreventsModification";
}

const char *RowIdConstraintPreventsModification::what() const noexcept
{
    return "Sqlite::RowIdConstraintPreventsModification";
}

const char *TriggerConstraintPreventsModification::what() const noexcept
{
    return "Sqlite::TriggerConstraintPreventsModification";
}

const char *UniqueConstraintPreventsModification::what() const noexcept
{
    return "Sqlite::UniqueConstraintPreventsModification";
}

const char *VirtualTableConstraintPreventsModification::what() const noexcept
{
    return "Sqlite::VirtualTableConstraintPreventsModification";
}

const char *DatabaseHasCorruptIndex::what() const noexcept
{
    return "Sqlite::DatabaseHasCorruptIndex";
}

const char *DatabaseHasCorruptSequence::what() const noexcept
{
    return "Sqlite::DatabaseHasCorruptSequence";
}

const char *DatabaseHasCorruptVirtualTable::what() const noexcept
{
    return "Sqlite::DatabaseHasCorruptVirtualTable";
}

const char *CannotInitializeReadOnlyConnection::what() const noexcept
{
    return "Sqlite::CannotInitializeReadOnlyConnection";
}

const char *CannotLockReadOnlyConnection::what() const noexcept
{
    return "Sqlite::CannotLockReadOnlyConnection";
}

const char *CannotWriteToMovedDatabase::what() const noexcept
{
    return "Sqlite::CannotWriteToMovedDatabase";
}

const char *CannotCreateLogInReadonlyDirectory::what() const noexcept
{
    return "Sqlite::CannotCreateLogInReadonlyDirectory";
}

const char *DatabaseNeedsToBeRecovered::what() const noexcept
{
    return "Sqlite::DatabaseNeedsToBeRecovered";
}

const char *CannotRollbackToReadOnlyConnection::what() const noexcept
{
    return "Sqlite::CannotRollbackToReadOnlyConnection";
}

const char *ConnectionsSharedCacheIsLocked::what() const noexcept
{
    return "Sqlite::ConnectionsSharedCacheIsLocked";
}

const char *ConnectionsVirtualTableIsLocked::what() const noexcept
{
    return "Sqlite::ConnectionsVirtualTableIsLocked";
}

const char *CannotOpenConvPath::what() const noexcept
{
    return "Sqlite::CannotOpenConvPath";
}

const char *CannotOpenDirtyWal::what() const noexcept
{
    return "Sqlite::CannotOpenDirtyWal";
}

const char *CannotCovertToFullPath::what() const noexcept
{
    return "Sqlite::CannotCovertToFullPath";
}

const char *CannotOpenDirectoryPath::what() const noexcept
{
    return "Sqlite::CannotOpenDirectoryPath";
}

const char *CannotOpenNoTempDir::what() const noexcept
{
    return "Sqlite::CannotOpenNoTempDir";
}

const char *CannotOpenSynbolicLink::what() const noexcept
{
    return "Sqlite::CannotOpenSynbolicLink";
}

void throwError(int resultCode, sqlite3 *sqliteHandle)
{
    switch (resultCode) {
    case SQLITE_BUSY_RECOVERY:
        throw StatementIsBusyRecovering(sqlite3_errmsg(sqliteHandle));
    case SQLITE_BUSY_SNAPSHOT:
        throw StatementIsBusySnapshot(sqlite3_errmsg(sqliteHandle));
    case SQLITE_BUSY_TIMEOUT:
        throw StatementIsBusyTimeout(sqlite3_errmsg(sqliteHandle));
    case SQLITE_BUSY:
        throw StatementIsBusy(sqlite3_errmsg(sqliteHandle));
    case SQLITE_ERROR_MISSING_COLLSEQ:
        throw StatementHasErrorMissingCollatingSequence(sqlite3_errmsg(sqliteHandle));
    case SQLITE_ERROR_RETRY:
        throw StatementHasErrorRetry(sqlite3_errmsg(sqliteHandle));
    case SQLITE_ERROR_SNAPSHOT:
        throw StatementHasErrorSnapshot(sqlite3_errmsg(sqliteHandle));
    case SQLITE_ERROR:
        throw StatementHasError(sqlite3_errmsg(sqliteHandle));
    case SQLITE_MISUSE:
        throw StatementIsMisused(sqlite3_errmsg(sqliteHandle));
    case SQLITE_CONSTRAINT_CHECK:
        throw CheckConstraintPreventsModification(sqlite3_errmsg(sqliteHandle));
    case SQLITE_CONSTRAINT_COMMITHOOK:
        throw CommitHookConstraintPreventsModification(sqlite3_errmsg(sqliteHandle));
    case SQLITE_CONSTRAINT_DATATYPE:
        throw DataTypeConstraintPreventsModification(sqlite3_errmsg(sqliteHandle));
    case SQLITE_CONSTRAINT_FOREIGNKEY:
        throw ForeignKeyConstraintPreventsModification(sqlite3_errmsg(sqliteHandle));
    case SQLITE_CONSTRAINT_FUNCTION:
        throw FunctionConstraintPreventsModification(sqlite3_errmsg(sqliteHandle));
    case SQLITE_CONSTRAINT_NOTNULL:
        throw NotNullConstraintPreventsModification(sqlite3_errmsg(sqliteHandle));
    case SQLITE_CONSTRAINT_PINNED:
        throw PinnedConstraintPreventsModification(sqlite3_errmsg(sqliteHandle));
    case SQLITE_CONSTRAINT_PRIMARYKEY:
        throw PrimaryKeyConstraintPreventsModification(sqlite3_errmsg(sqliteHandle));
    case SQLITE_CONSTRAINT_ROWID:
        throw RowIdConstraintPreventsModification(sqlite3_errmsg(sqliteHandle));
    case SQLITE_CONSTRAINT_TRIGGER:
        throw TriggerConstraintPreventsModification(sqlite3_errmsg(sqliteHandle));
    case SQLITE_CONSTRAINT_UNIQUE:
        throw UniqueConstraintPreventsModification(sqlite3_errmsg(sqliteHandle));
    case SQLITE_CONSTRAINT_VTAB:
        throw VirtualTableConstraintPreventsModification(sqlite3_errmsg(sqliteHandle));
    case SQLITE_CONSTRAINT:
        throw ConstraintPreventsModification(sqlite3_errmsg(sqliteHandle));
    case SQLITE_TOOBIG:
        throw TooBig(sqlite3_errmsg(sqliteHandle));
    case SQLITE_SCHEMA:
        throw SchemeChangeError(sqlite3_errmsg(sqliteHandle));
    case SQLITE_READONLY_CANTINIT:
        throw CannotInitializeReadOnlyConnection(sqlite3_errmsg(sqliteHandle));
    case SQLITE_READONLY_CANTLOCK:
        throw CannotLockReadOnlyConnection(sqlite3_errmsg(sqliteHandle));
    case SQLITE_READONLY_DBMOVED:
        throw CannotWriteToMovedDatabase(sqlite3_errmsg(sqliteHandle));
    case SQLITE_READONLY_DIRECTORY:
        throw CannotCreateLogInReadonlyDirectory(sqlite3_errmsg(sqliteHandle));
    case SQLITE_READONLY_RECOVERY:
        throw DatabaseNeedsToBeRecovered(sqlite3_errmsg(sqliteHandle));
    case SQLITE_READONLY_ROLLBACK:
        throw CannotRollbackToReadOnlyConnection(sqlite3_errmsg(sqliteHandle));
    case SQLITE_READONLY:
        throw CannotWriteToReadOnlyConnection(sqlite3_errmsg(sqliteHandle));
    case SQLITE_PROTOCOL:
        throw ProtocolError(sqlite3_errmsg(sqliteHandle));
    case SQLITE_NOMEM:
        throw std::bad_alloc();
    case SQLITE_NOLFS:
        throw DatabaseExceedsMaximumFileSize(sqlite3_errmsg(sqliteHandle));
    case SQLITE_MISMATCH:
        throw DataTypeMismatch(sqlite3_errmsg(sqliteHandle));
    case SQLITE_LOCKED_SHAREDCACHE:
        throw ConnectionsSharedCacheIsLocked(sqlite3_errmsg(sqliteHandle));
    case SQLITE_LOCKED_VTAB:
        throw ConnectionsVirtualTableIsLocked(sqlite3_errmsg(sqliteHandle));
    case SQLITE_LOCKED:
        throw ConnectionIsLocked(sqlite3_errmsg(sqliteHandle));
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
        throw ExecutionInterrupted();
    case SQLITE_CORRUPT_INDEX:
        throw DatabaseHasCorruptIndex();
    case SQLITE_CORRUPT_SEQUENCE:
        throw DatabaseHasCorruptSequence();
    case SQLITE_CORRUPT_VTAB:
        throw DatabaseHasCorruptVirtualTable();
    case SQLITE_CORRUPT:
        throw DatabaseIsCorrupt();
    case SQLITE_CANTOPEN_CONVPATH:
        throw CannotOpenConvPath();
    case SQLITE_CANTOPEN_DIRTYWAL:
        throw CannotOpenDirtyWal();
    case SQLITE_CANTOPEN_FULLPATH:
        throw CannotCovertToFullPath();
    case SQLITE_CANTOPEN_ISDIR:
        throw CannotOpenDirectoryPath();
    case SQLITE_CANTOPEN_NOTEMPDIR:
        throw CannotOpenNoTempDir();
    case SQLITE_CANTOPEN_SYMLINK:
        throw CannotOpenSynbolicLink();
    case SQLITE_CANTOPEN:
        throw CannotOpen();
    case SQLITE_RANGE:
        throw BindingIndexIsOutOfRange(sqlite3_errmsg(sqliteHandle));
    }

    if (sqliteHandle)
        throw UnknowError(sqlite3_errmsg(sqliteHandle));
    else
        throw UnknowError();
}

} // namespace Sqlite

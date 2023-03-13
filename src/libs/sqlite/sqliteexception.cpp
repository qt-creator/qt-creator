// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "sqliteexception.h"

#include <utils/smallstringio.h>

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

} // namespace Sqlite

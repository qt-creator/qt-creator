// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "savefile.h"

#include "filepath.h"
#include "hostosinfo.h"
#include "qtcassert.h"

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <io.h>
#else
#  include <unistd.h>
#  include <sys/stat.h>
#endif

namespace Utils {

static QFile::Permissions m_umask;

SaveFile::SaveFile(const FilePath &filePath) :
    m_finalFilePath(filePath)
{
}

SaveFile::~SaveFile()
{
    QTC_ASSERT(m_finalized, rollback());
}

bool SaveFile::open(OpenMode flags)
{
    QTC_ASSERT(!m_finalFilePath.isEmpty(), return false);
    QTC_ASSERT(!m_tempFile, return false);

    QFile ofi(m_finalFilePath.toFSPathString());
    // Check whether the existing file is writable
    if (ofi.exists() && !ofi.open(QIODevice::ReadWrite)) {
        setErrorString(ofi.errorString());
        return false;
    }

    auto tmpPath = TemporaryFilePath::create(m_finalFilePath);
    if (!tmpPath) {
        setErrorString(tmpPath.error());
        return false;
    }
    m_tempFile = std::move(*tmpPath);
    m_tempFile->setAutoRemove(false);

    setFileName(m_tempFile->filePath().toFSPathString());

    if (!QFile::open(flags))
        return false;

    m_finalized = false; // needs clean up in the end
    if (ofi.exists()) {
        setPermissions(ofi.permissions()); // Ignore errors
    } else {
        Permissions permAll = QFile::ReadOwner
                | QFile::ReadGroup
                | QFile::ReadOther
                | QFile::WriteOwner
                | QFile::WriteGroup
                | QFile::WriteOther;

        // set permissions with respect to the current umask
        setPermissions(permAll & ~m_umask);
    }

    return true;
}

void SaveFile::rollback()
{
    close();
    if (m_tempFile)
        m_tempFile->filePath().removeFile();
    m_finalized = true;
}

#ifndef Q_OS_WIN
// Declare what we need from Windows API so we can compile without #ifdef
constexpr int REPLACEFILE_IGNORE_MERGE_ERRORS = 0;
constexpr int ERROR_UNABLE_TO_REMOVE_REPLACED = 0;
constexpr int MOVEFILE_COPY_ALLOWED = 0;
constexpr int MOVEFILE_REPLACE_EXISTING = 0;
constexpr int MOVEFILE_WRITE_THROUGH = 0;
constexpr int FORMAT_MESSAGE_FROM_SYSTEM = 0;
constexpr int FORMAT_MESSAGE_IGNORE_INSERTS = 0;
constexpr int LANG_NEUTRAL = 0;
constexpr int SUBLANG_DEFAULT = 0;

using DWORD = unsigned long;

bool MoveFileEx(const wchar_t *, const wchar_t *, DWORD);
bool ReplaceFile(const wchar_t *, const wchar_t *, const wchar_t *, DWORD, void *, void *);
DWORD GetLastError();
DWORD MAKELANGID(int, int);
DWORD FormatMessageW(DWORD, void *, DWORD, DWORD, const wchar_t *, DWORD, void *);
#endif

bool SaveFile::commit()
{
    QTC_ASSERT(!m_finalized && m_tempFile, return false;);
    m_finalized = true;

    if (!flush()) {
        close();
        m_tempFile->filePath().removeFile();
        return false;
    }
#ifdef Q_OS_WIN
    FlushFileBuffers(reinterpret_cast<HANDLE>(_get_osfhandle(handle())));
#elif _POSIX_SYNCHRONIZED_IO > 0
    fdatasync(handle());
#else
    fsync(handle());
#endif

    close();
    if (error() != NoError) {
        m_tempFile->filePath().removeFile();
        return false;
    }

    FilePath finalFileName = m_finalFilePath.resolveSymlinks();

    if constexpr (HostOsInfo::isWindowsHost()) {
        static const bool disableWinSpecialCode = !qEnvironmentVariableIsEmpty(
            "QTC_DISABLE_SPECIAL_WIN_SAVEFILE");
        if (m_finalFilePath.isLocal() && !disableWinSpecialCode) {
            // Release the file lock
            m_tempFile.reset();
            bool result = ReplaceFile(
                finalFileName.nativePath().toStdWString().data(),
                fileName().toStdWString().data(),
                nullptr,
                REPLACEFILE_IGNORE_MERGE_ERRORS,
                nullptr,
                nullptr);
            if (!result) {
                DWORD replaceErrorCode = GetLastError();
                QString errorStr;
                if (!finalFileName.exists()) {
                    // Replace failed because finalFileName does not exist, try rename.
                    if (!(result = rename(finalFileName.toFSPathString())))
                        errorStr = errorString();
                } else {
                    if (replaceErrorCode == ERROR_UNABLE_TO_REMOVE_REPLACED) {
                        // If we do not get the rights to remove the original final file we still might try
                        // to replace the file contents
                        result = MoveFileEx(
                            fileName().toStdWString().data(),
                            finalFileName.nativePath().toStdWString().data(),
                            MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING
                                | MOVEFILE_WRITE_THROUGH);
                        if (!result)
                            replaceErrorCode = GetLastError();
                    }
                    if (!result) {
                        wchar_t messageBuffer[256];
                        FormatMessageW(
                            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                            nullptr,
                            replaceErrorCode,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            messageBuffer,
                            sizeof(messageBuffer),
                            nullptr);
                        errorStr = QString::fromWCharArray(messageBuffer);
                    }
                }
                if (!result) {
                    remove();
                    setErrorString(errorStr);
                }
            }

            return result;
        }
    }

    const QString backupName = finalFileName.toFSPathString() + '~';

    // Back up current file.
    // If it's opened by another application, the lock follows the move.
    if (finalFileName.exists()) {
        // Kill old backup. Might be useful if creator crashed before removing backup.
        QFile::remove(backupName);
        QFile finalFile(finalFileName.toFSPathString());
        if (!finalFile.rename(backupName)) {
            m_tempFile->filePath().removeFile();
            setErrorString(finalFile.errorString());
            return false;
        }
    }

    Result<> renameResult = m_tempFile->filePath().renameFile(finalFileName);
    if (!renameResult) {
        // The case when someone else was able to create finalFileName after we've renamed it.
        // Higher level call may try to save this file again but here we do nothing and
        // return false while keeping the error string from last rename call.
        m_tempFile->filePath().removeFile();
        setErrorString(renameResult.error());
        QFile::rename(backupName, finalFileName.toFSPathString()); // rollback to backup if possible ...
        return false; // ... or keep the backup copy at least
    }

    QFile::remove(backupName);

    return true;
}

void SaveFile::initializeUmask()
{
#ifdef Q_OS_WIN
    m_umask = QFile::WriteGroup | QFile::WriteOther;
#else
    // Get the current process' file creation mask (umask)
    // umask() is not thread safe so this has to be done by single threaded
    // application initialization
    mode_t mask = umask(0); // get current umask
    umask(mask); // set it back

    m_umask = ((mask & S_IRUSR) ? QFile::ReadOwner  : QFlags<QFile::Permission>())
            | ((mask & S_IWUSR) ? QFile::WriteOwner : QFlags<QFile::Permission>())
            | ((mask & S_IXUSR) ? QFile::ExeOwner   : QFlags<QFile::Permission>())
            | ((mask & S_IRGRP) ? QFile::ReadGroup  : QFlags<QFile::Permission>())
            | ((mask & S_IWGRP) ? QFile::WriteGroup : QFlags<QFile::Permission>())
            | ((mask & S_IXGRP) ? QFile::ExeGroup   : QFlags<QFile::Permission>())
            | ((mask & S_IROTH) ? QFile::ReadOther  : QFlags<QFile::Permission>())
            | ((mask & S_IWOTH) ? QFile::WriteOther : QFlags<QFile::Permission>())
            | ((mask & S_IXOTH) ? QFile::ExeOther   : QFlags<QFile::Permission>());
#endif
}

} //  Utils

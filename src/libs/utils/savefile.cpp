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

#include "savefile.h"
#include "qtcassert.h"
#include "fileutils.h"
#ifdef Q_OS_WIN
#  include <windows.h>
#  include <io.h>
#else
#  include <unistd.h>
#  include <sys/stat.h>
#endif

namespace Utils {

static QFile::Permissions m_umask;

SaveFile::SaveFile(const QString &filename) :
    m_finalFileName(filename)
{
}

SaveFile::~SaveFile()
{
    QTC_ASSERT(m_finalized, rollback());
}

bool SaveFile::open(OpenMode flags)
{
    QTC_ASSERT(!m_finalFileName.isEmpty(), return false);

    QFile ofi(m_finalFileName);
    // Check whether the existing file is writable
    if (ofi.exists() && !ofi.open(QIODevice::ReadWrite)) {
        setErrorString(ofi.errorString());
        return false;
    }

    m_tempFile = std::make_unique<QTemporaryFile>(m_finalFileName);
    m_tempFile->setAutoRemove(false);
    if (!m_tempFile->open())
        return false;
    setFileName(m_tempFile->fileName());

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
        m_tempFile->remove();
    m_finalized = true;
}

bool SaveFile::commit()
{
    QTC_ASSERT(!m_finalized && m_tempFile, return false;);
    m_finalized = true;

    if (!flush()) {
        close();
        m_tempFile->remove();
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
    m_tempFile->close();
    if (error() != NoError) {
        m_tempFile->remove();
        return false;
    }

    QString finalFileName
            = FileUtils::resolveSymlinks(FilePath::fromString(m_finalFileName)).toString();

#ifdef Q_OS_WIN
    // Release the file lock
    m_tempFile.reset();
    bool result = ReplaceFile(finalFileName.toStdWString().data(),
                              fileName().toStdWString().data(),
                              nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr);
    if (!result) {
        DWORD replaceErrorCode = GetLastError();
        QString errorStr;
        if (!QFile::exists(finalFileName)) {
            // Replace failed because finalFileName does not exist, try rename.
            if (!(result = rename(finalFileName)))
                errorStr = errorString();
        } else {
            if (replaceErrorCode == ERROR_UNABLE_TO_REMOVE_REPLACED) {
                // If we do not get the rights to remove the original final file we still might try
                // to replace the file contents
                result = MoveFileEx(fileName().toStdWString().data(),
                                    finalFileName.toStdWString().data(),
                                    MOVEFILE_COPY_ALLOWED
                                    | MOVEFILE_REPLACE_EXISTING
                                    | MOVEFILE_WRITE_THROUGH);
                if (!result)
                    replaceErrorCode = GetLastError();
            }
            if (!result) {
                wchar_t messageBuffer[256];
                FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                               nullptr, replaceErrorCode,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               messageBuffer, sizeof(messageBuffer), nullptr);
                errorStr = QString::fromWCharArray(messageBuffer);
            }
        }
        if (!result) {
            remove();
            setErrorString(errorStr);
        }
    }

    return result;
#else
    const QString backupName = finalFileName + '~';

    // Back up current file.
    // If it's opened by another application, the lock follows the move.
    if (QFile::exists(finalFileName)) {
        // Kill old backup. Might be useful if creator crashed before removing backup.
        QFile::remove(backupName);
        QFile finalFile(finalFileName);
        if (!finalFile.rename(backupName)) {
            m_tempFile->remove();
            setErrorString(finalFile.errorString());
            return false;
        }
    }

    bool result = true;
    if (!m_tempFile->rename(finalFileName)) {
        // The case when someone else was able to create finalFileName after we've renamed it.
        // Higher level call may try to save this file again but here we do nothing and
        // return false while keeping the error string from last rename call.
        const QString &renameError = m_tempFile->errorString();
        m_tempFile->remove();
        setErrorString(renameError);
        QFile::rename(backupName, finalFileName); // rollback to backup if possible ...
        return false; // ... or keep the backup copy at least
    }

    QFile::remove(backupName);

    return result;
#endif
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

} // namespace Utils

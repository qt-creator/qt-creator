/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "savefile.h"
#include "qtcassert.h"
#include "fileutils.h"
#ifdef Q_OS_WIN
#  include <windows.h>
#else
#  include <unistd.h>
#endif

namespace Utils {

SaveFile::SaveFile(const QString &filename) :
    m_finalFileName(filename), m_finalized(true), m_backup(false)
{
}

SaveFile::~SaveFile()
{
    QTC_ASSERT(m_finalized, rollback());
}

bool SaveFile::open(OpenMode flags)
{
    QTC_ASSERT(!m_finalFileName.isEmpty() && fileName().isEmpty(), return false);

    QFile ofi(m_finalFileName);
    // Check whether the existing file is writable
    if (ofi.exists() && !ofi.open(QIODevice::ReadWrite)) {
        setErrorString(ofi.errorString());
        return false;
    }

    setAutoRemove(false);
    setFileTemplate(m_finalFileName);
    if (!QTemporaryFile::open(flags))
        return false;

    m_finalized = false; // needs clean up in the end
    if (ofi.exists())
        setPermissions(ofi.permissions()); // Ignore errors

    return true;
}

void SaveFile::rollback()
{
    remove();
    m_finalized = true;
}

bool SaveFile::commit()
{
    QTC_ASSERT(!m_finalized, return false);
    m_finalized = true;

    if (!flush()) {
        remove();
        return false;
    }
#ifdef Q_OS_WIN
    FlushFileBuffers(handle());
#else
    fdatasync(handle());
#endif
    close();
    if (error() != NoError) {
        remove();
        return false;
    }

    QString finalFileName = Utils::FileUtils::resolveSymlinks(m_finalFileName);
    QString bakname = finalFileName + QLatin1Char('~');
    QFile::remove(bakname); // Kill old backup
    QFile::rename(finalFileName, bakname); // Backup current file
    if (!rename(finalFileName)) { // Replace current file
        QFile::rename(bakname, finalFileName); // Rollback to current file
        return false;
    }
    if (!m_backup)
        QFile::remove(bakname);

    return true;
}

} // namespace Utils

/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef QTLOCKEDFILE_H
#define QTLOCKEDFILE_H

#include <QtCore/QFile>

#if defined(Q_WS_WIN)
#  if !defined(QT_QTLOCKEDFILE_EXPORT) && !defined(QT_QTLOCKEDFILE_IMPORT)
#    define QT_QTLOCKEDFILE_EXPORT
#  elif defined(QT_QTLOCKEDFILE_IMPORT)
#    if defined(QT_QTLOCKEDFILE_EXPORT)
#      undef QT_QTLOCKEDFILE_EXPORT
#    endif
#    define QT_QTLOCKEDFILE_EXPORT __declspec(dllimport)
#  elif defined(QT_QTLOCKEDFILE_EXPORT)
#    undef QT_QTLOCKEDFILE_EXPORT
#    define QT_QTLOCKEDFILE_EXPORT __declspec(dllexport)
#  endif
#else
#  define QT_QTLOCKEDFILE_EXPORT
#endif

namespace SharedTools {

class QT_QTLOCKEDFILE_EXPORT QtLockedFile : public QFile
{
public:    
    enum LockMode { NoLock = 0, ReadLock, WriteLock };

    QtLockedFile();
    QtLockedFile(const QString &name);
    ~QtLockedFile();
    
    bool lock(LockMode mode, bool block = true);
    bool unlock();
    bool isLocked() const;
    LockMode lockMode() const;
    
private:
#ifdef Q_OS_WIN
    Qt::HANDLE m_semaphore_hnd;
    Qt::HANDLE m_mutex_hnd;
#endif
    LockMode m_lock_mode;
};

} // namespace SharedTools

#endif // QTLOCKEDFILE_H

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

#include "qmake_global.h"

#include <qiodevice.h>
#include <qhash.h>
#include <qstring.h>
#ifdef PROEVALUATOR_THREAD_SAFE
# include <qmutex.h>
#endif

#ifdef PROEVALUATOR_DUAL_VFS
# ifndef PROEVALUATOR_CUMULATIVE
#  error PROEVALUATOR_DUAL_VFS requires PROEVALUATOR_CUMULATIVE
# endif
#endif

QT_BEGIN_NAMESPACE

class QMAKE_EXPORT QMakeVfs
{
public:
    enum ReadResult {
        ReadOk,
        ReadNotFound,
        ReadOtherError
    };

    enum VfsFlag {
        VfsExecutable = 1,
        VfsExact = 0,
#ifdef PROEVALUATOR_DUAL_VFS
        VfsCumulative = 2,
        VfsCreate = 4,
        VfsCreatedOnly = 8,
#else
        VfsCumulative = 0,
        VfsCreate = 0,
        VfsCreatedOnly = 0,
#endif
        VfsAccessedOnly = 16
    };
    Q_DECLARE_FLAGS(VfsFlags, VfsFlag)

    QMakeVfs();
    ~QMakeVfs();

    static void ref();
    static void deref();

    int idForFileName(const QString &fn, VfsFlags flags);
    QString fileNameForId(int id);
    bool writeFile(int id, QIODevice::OpenMode mode, VfsFlags flags, const QString &contents, QString *errStr);
    ReadResult readFile(int id, QString *contents, QString *errStr);
    bool exists(const QString &fn, QMakeVfs::VfsFlags flags);

#ifndef PROEVALUATOR_FULL
    void invalidateCache();
    void invalidateContents();
#endif

private:
#ifdef PROEVALUATOR_THREAD_SAFE
    static QMutex s_mutex;
#endif
    static int s_refCount;
    static QAtomicInt s_fileIdCounter;
    // Qt Creator's ProFile cache is a singleton to maximize its cross-project
    // effectiveness (shared prf files from QtVersions).
    // For this to actually work, real files need a global mapping.
    // This is fine, because the namespace of real files is indeed global.
    static QHash<QString, int> s_fileIdMap;
    static QHash<int, QString> s_idFileMap;
#ifdef PROEVALUATOR_DUAL_VFS
# ifdef PROEVALUATOR_THREAD_SAFE
    // The simple way to avoid recursing m_mutex.
    QMutex m_vmutex;
# endif
    // Virtual files are bound to the project context they were created in,
    // so their ids need to be local as well.
    // We violate that rule in lupdate (which has a non-dual VFS), but that
    // does not matter, because it has only one project context anyway.
    QHash<QString, int> m_virtualFileIdMap[2];  // Exact and cumulative
    QHash<int, QString> m_virtualIdFileMap;  // Only one map, as ids are unique across realms.
#endif

#ifndef PROEVALUATOR_FULL
# ifdef PROEVALUATOR_THREAD_SAFE
    QMutex m_mutex;
# endif
    QHash<int, QString> m_files;
    QString m_magicMissing;
    QString m_magicExisting;
#endif
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMakeVfs::VfsFlags)

QT_END_NAMESPACE

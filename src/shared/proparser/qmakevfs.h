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

# include <qiodevice.h>
#ifndef PROEVALUATOR_FULL
# include <qhash.h>
# include <qstring.h>
# ifdef PROEVALUATOR_THREAD_SAFE
#  include <qmutex.h>
# endif
#endif

#ifndef QT_NO_TEXTCODEC
QT_FORWARD_DECLARE_CLASS(QTextCodec)
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
        VfsNoVirtual = 4
#else
        VfsCumulative = 0,
        VfsNoVirtual = 0
#endif
    };
    Q_DECLARE_FLAGS(VfsFlags, VfsFlag)

    QMakeVfs();

    bool writeFile(const QString &fn, QIODevice::OpenMode mode, VfsFlags flags, const QString &contents, QString *errStr);
    ReadResult readFile(const QString &fn, VfsFlags flags, QString *contents, QString *errStr);
    bool exists(const QString &fn, VfsFlags flags);

#ifndef PROEVALUATOR_FULL
    bool readVirtualFile(const QString &fn, VfsFlags flags, QString *contents);

    void invalidateCache();
    void invalidateContents();
#endif

#ifndef QT_NO_TEXTCODEC
    void setTextCodec(const QTextCodec *textCodec);
#endif

private:
#ifndef PROEVALUATOR_FULL
# ifdef PROEVALUATOR_THREAD_SAFE
    QMutex m_mutex;
# endif
    QHash<QString, QString> m_files;
    QString m_magicMissing;
    QString m_magicExisting;
#endif
#ifndef QT_NO_TEXTCODEC
    const QTextCodec *m_textCodec;
#endif
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMakeVfs::VfsFlags)

QT_END_NAMESPACE

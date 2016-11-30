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

#include "qmakevfs.h"

#include "ioutils.h"
using namespace QMakeInternal;

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>

#ifndef QT_NO_TEXTCODEC
#include <qtextcodec.h>
#endif

#define fL1S(s) QString::fromLatin1(s)

QT_BEGIN_NAMESPACE

QMakeVfs::QMakeVfs()
#ifndef PROEVALUATOR_FULL
    : m_magicMissing(fL1S("missing"))
    , m_magicExisting(fL1S("existing"))
#endif
{
#ifndef QT_NO_TEXTCODEC
    m_textCodec = 0;
#endif
}

bool QMakeVfs::writeFile(const QString &fn, QIODevice::OpenMode mode, VfsFlags flags,
                         const QString &contents, QString *errStr)
{
#ifndef PROEVALUATOR_FULL
# ifdef PROEVALUATOR_THREAD_SAFE
    QMutexLocker locker(&m_mutex);
# endif
#ifdef PROEVALUATOR_DUAL_VFS
    QString *cont = &m_files[((flags & VfsCumulative) ? '-' : '+') + fn];
#else
    QString *cont = &m_files[fn];
    Q_UNUSED(flags)
#endif
    if (mode & QIODevice::Append)
        *cont += contents;
    else
        *cont = contents;
    Q_UNUSED(errStr)
    return true;
#else
    QFileInfo qfi(fn);
    if (!QDir::current().mkpath(qfi.path())) {
        *errStr = fL1S("Cannot create parent directory");
        return false;
    }
    QByteArray bytes = contents.toLocal8Bit();
    QFile cfile(fn);
    if (!(mode & QIODevice::Append) && cfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (cfile.readAll() == bytes) {
            if (flags & VfsExecutable) {
                cfile.setPermissions(cfile.permissions()
                                     | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);
            } else {
                cfile.setPermissions(cfile.permissions()
                                     & ~(QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther));
            }
            return true;
        }
        cfile.close();
    }
    if (!cfile.open(mode | QIODevice::WriteOnly | QIODevice::Text)) {
        *errStr = cfile.errorString();
        return false;
    }
    cfile.write(bytes);
    cfile.close();
    if (cfile.error() != QFile::NoError) {
        *errStr = cfile.errorString();
        return false;
    }
    if (flags & VfsExecutable)
        cfile.setPermissions(cfile.permissions()
                             | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);
    return true;
#endif
}

#ifndef PROEVALUATOR_FULL
bool QMakeVfs::readVirtualFile(const QString &fn, VfsFlags flags, QString *contents)
{
# ifdef PROEVALUATOR_THREAD_SAFE
    QMutexLocker locker(&m_mutex);
# endif
    QHash<QString, QString>::ConstIterator it;
# ifdef PROEVALUATOR_DUAL_VFS
    it = m_files.constFind(((flags & VfsCumulative) ? '-' : '+') + fn);
    if (it != m_files.constEnd()) {
        *contents = *it;
        return true;
    }
# else
    it = m_files.constFind(fn);
    if (it != m_files.constEnd()
        && it->constData() != m_magicMissing.constData()
        && it->constData() != m_magicExisting.constData()) {
        *contents = *it;
        return true;
    }
    Q_UNUSED(flags)
# endif
    return false;
}
#endif

QMakeVfs::ReadResult QMakeVfs::readFile(
        const QString &fn, VfsFlags flags, QString *contents, QString *errStr)
{
#ifndef PROEVALUATOR_FULL
# ifdef PROEVALUATOR_THREAD_SAFE
    QMutexLocker locker(&m_mutex);
# endif
    QHash<QString, QString>::ConstIterator it;
# ifdef PROEVALUATOR_DUAL_VFS
    if (!(flags & VfsNoVirtual)) {
        it = m_files.constFind(((flags & VfsCumulative) ? '-' : '+') + fn);
        if (it != m_files.constEnd()) {
            *contents = *it;
            return ReadOk;
        }
    }
# else
    Q_UNUSED(flags)
# endif
    it = m_files.constFind(fn);
    if (it != m_files.constEnd()) {
        if (it->constData() == m_magicMissing.constData()) {
            *errStr = fL1S("No such file or directory");
            return ReadNotFound;
        }
        if (it->constData() != m_magicExisting.constData()) {
            *contents = *it;
            return ReadOk;
        }
    }
#else
    Q_UNUSED(flags)
#endif

    QFile file(fn);
    if (!file.open(QIODevice::ReadOnly)) {
        if (!file.exists()) {
#ifndef PROEVALUATOR_FULL
            m_files[fn] = m_magicMissing;
#endif
            *errStr = fL1S("No such file or directory");
            return ReadNotFound;
        }
        *errStr = file.errorString();
        return ReadOtherError;
    }
#ifndef PROEVALUATOR_FULL
    m_files[fn] = m_magicExisting;
#endif

    QByteArray bcont = file.readAll();
    if (bcont.startsWith("\xef\xbb\xbf")) {
        // UTF-8 BOM will cause subtle errors
        *errStr = fL1S("Unexpected UTF-8 BOM");
        return ReadOtherError;
    }
    *contents =
#ifndef QT_NO_TEXTCODEC
        m_textCodec ? m_textCodec->toUnicode(bcont) :
#endif
        QString::fromLocal8Bit(bcont);
    return ReadOk;
}

bool QMakeVfs::exists(const QString &fn, VfsFlags flags)
{
#ifndef PROEVALUATOR_FULL
# ifdef PROEVALUATOR_THREAD_SAFE
    QMutexLocker locker(&m_mutex);
# endif
    QHash<QString, QString>::ConstIterator it;
# ifdef PROEVALUATOR_DUAL_VFS
    it = m_files.constFind(((flags & VfsCumulative) ? '-' : '+') + fn);
    if (it != m_files.constEnd())
        return true;
# else
    Q_UNUSED(flags)
# endif
    it = m_files.constFind(fn);
    if (it != m_files.constEnd())
        return it->constData() != m_magicMissing.constData();
#else
    Q_UNUSED(flags)
#endif
    bool ex = IoUtils::fileType(fn) == IoUtils::FileIsRegular;
#ifndef PROEVALUATOR_FULL
    m_files[fn] = ex ? m_magicExisting : m_magicMissing;
#endif
    return ex;
}

#ifndef PROEVALUATOR_FULL
// This should be called when the sources may have changed (e.g., VCS update).
void QMakeVfs::invalidateCache()
{
# ifdef PROEVALUATOR_THREAD_SAFE
    QMutexLocker locker(&m_mutex);
# endif
    QHash<QString, QString>::Iterator it = m_files.begin(), eit = m_files.end();
    while (it != eit) {
        if (it->constData() == m_magicMissing.constData()
                ||it->constData() == m_magicExisting.constData())
            it = m_files.erase(it);
        else
            ++it;
    }
}

// This should be called when generated files may have changed (e.g., actual build).
void QMakeVfs::invalidateContents()
{
# ifdef PROEVALUATOR_THREAD_SAFE
    QMutexLocker locker(&m_mutex);
# endif
    m_files.clear();
}
#endif

#ifndef QT_NO_TEXTCODEC
void QMakeVfs::setTextCodec(const QTextCodec *textCodec)
{
    m_textCodec = textCodec;
}
#endif

QT_END_NAMESPACE

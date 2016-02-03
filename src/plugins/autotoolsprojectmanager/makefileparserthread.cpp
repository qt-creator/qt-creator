/****************************************************************************
**
** Copyright (C) 2016 Openismus GmbH.
** Author: Peter Penz (ppenz@openismus.com)
** Author: Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#include "makefileparserthread.h"

#include <QMutexLocker>

using namespace AutotoolsProjectManager::Internal;

MakefileParserThread::MakefileParserThread(const QString &makefile) : m_parser(makefile)
{
    connect(&m_parser, &MakefileParser::status,
            this, &MakefileParserThread::status);
}

QStringList MakefileParserThread::sources() const
{
    QMutexLocker locker(&m_mutex);
    return m_sources;
}

QStringList MakefileParserThread::makefiles() const
{
    QMutexLocker locker(&m_mutex);
    return m_makefiles;
}

QString MakefileParserThread::executable() const
{
    QMutexLocker locker(&m_mutex);
    return m_executable;
}

QStringList MakefileParserThread::includePaths() const
{
    QMutexLocker locker(&m_mutex);
    return m_includePaths;
}

QByteArray MakefileParserThread::defines() const
{
    QMutexLocker locker(&m_mutex);
    return m_defines;
}

QStringList MakefileParserThread::cflags() const
{
    QMutexLocker locker(&m_mutex);
    return m_cflags;
}

QStringList MakefileParserThread::cxxflags() const
{
    QMutexLocker locker(&m_mutex);
    return m_cxxflags;
}

bool MakefileParserThread::hasError() const
{
    QMutexLocker locker(&m_mutex);
    return m_hasError;
}

bool MakefileParserThread::isCanceled() const
{
    // MakefileParser::isCanceled() is thread-safe
    return m_parser.isCanceled();
}

void MakefileParserThread::cancel()
{
    m_parser.cancel();
}

void MakefileParserThread::run()
{
    const bool success = m_parser.parse();

    // Important: Start locking the mutex _after_ the parsing has been finished, as
    // this prevents long locks if the caller reads a value before the signal
    // finished() has been emitted.
    QMutexLocker locker(&m_mutex);
    m_hasError = !success;
    m_executable = m_parser.executable();
    m_sources = m_parser.sources();
    m_makefiles = m_parser.makefiles();
    m_includePaths = m_parser.includePaths();
    m_defines = m_parser.defines();
    m_cflags = m_parser.cflags();
    m_cxxflags = m_parser.cxxflags();
}

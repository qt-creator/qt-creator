// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "makefileparserthread.h"

#include <QMutexLocker>

namespace AutotoolsProjectManager::Internal {

MakefileParserThread::MakefileParserThread(ProjectExplorer::BuildSystem *bs)
    : m_parser(bs->projectFilePath().toString()),
      m_guard(bs->guardParsingRun())
{
    connect(&m_parser, &MakefileParser::status, this, &MakefileParserThread::status);
    connect(this, &QThread::finished, this, &MakefileParserThread::done, Qt::QueuedConnection);
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

ProjectExplorer::Macros MakefileParserThread::macros() const
{
    QMutexLocker locker(&m_mutex);
    return m_macros;
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
    return !m_guard.isSuccess();
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
    if (success)
        m_guard.markAsSuccess();
    m_executable = m_parser.executable();
    m_sources = m_parser.sources();
    m_makefiles = m_parser.makefiles();
    m_includePaths = m_parser.includePaths();
    m_macros = m_parser.macros();
    m_cflags = m_parser.cflags();
    m_cxxflags = m_parser.cxxflags();
}

} // AutotoolsProjectManager::Internal

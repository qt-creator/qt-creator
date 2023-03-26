// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "makefileparser.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectmacro.h>

#include <QMutex>
#include <QStringList>
#include <QThread>
#include <QVector>

namespace AutotoolsProjectManager::Internal {

/**
 * @brief Executes the makefile parser in the thread.
 *
 * After the finished() signal has been emitted, the makefile
 * parser output can be read by sources(), makefiles() and executable().
 * A parsing error can be checked by hasError().
 */
class MakefileParserThread : public QThread
{
    Q_OBJECT

    using Macros = ProjectExplorer::Macros;

public:
    explicit MakefileParserThread(ProjectExplorer::BuildSystem *bs);

    /** @see QThread::run() */
    void run() override;

    /**
     * @return List of sources that are set for the _SOURCES target.
     *         Sources in sub directorties contain the sub directory as
     *         prefix. Should be invoked, after the signal finished()
     *         has been emitted.
     */
    QStringList sources() const;

    /**
     * @return List of Makefile.am files from the current directory and
     *         all sub directories. The values for sub directories contain
     *         the sub directory as prefix. Should be invoked, after the
     *         signal finished() has been emitted.
     */
    QStringList makefiles() const;

    /**
     * @return File name of the executable. Should be invoked, after the
     *         signal finished() has been emitted.
     */
    QString executable() const;

    /**
     * @return List of include paths. Should be invoked, after the signal
     *         finished() has been emitted.
     */
    QStringList includePaths() const;

    /**
     * @return Concatenated macros. Should be invoked, after the signal
     *         finished() has been emitted.
     */
    Macros macros() const;

    /**
     * @return List of compiler flags for C. Should be invoked, after the signal
     *         finished() has been emitted.
     */
    QStringList cflags() const;

    /**
     * @return List of compiler flags for C++. Should be invoked, after the
     *         signal finished() has been emitted.
     */
    QStringList cxxflags() const;

    /**
     * @return True, if an error occurred during the parsing. Should be invoked,
     *         after the signal finished() has been emitted.
     */
    bool hasError() const;

    /**
     * @return True, if the parsing has been cancelled by MakefileParserThread::cancel().
     */
    bool isCanceled() const;

    /**
     * Cancels the parsing of the makefile. MakefileParser::hasError() will
     * return true in this case.
     */
    void cancel();

signals:
    /**
     * Is emitted periodically during parsing the Makefile.am files
     * and the sub directories. \p status provides a translated
     * string, that can be shown to indicate the current state
     * of the parsing.
     */
    void status(const QString &status);

    /**
      * Similar to finished, but emitted from MakefileParserThread thread, i.e. from the
      * thread where the MakefileParserThread lives in, not the tread that it creates.
      * This helps to avoid race condition when connecting to finished() signal.
      */
    void done();

private:
    MakefileParser m_parser;    ///< Is not accessible outside the thread

    mutable QMutex m_mutex;
    QString m_executable;       ///< Return value for MakefileParserThread::executable()
    QStringList m_sources;      ///< Return value for MakefileParserThread::sources()
    QStringList m_makefiles;    ///< Return value for MakefileParserThread::makefiles()
    QStringList m_includePaths; ///< Return value for MakefileParserThread::includePaths()
    Macros m_macros;            ///< Return value for MakefileParserThread::macros()
    QStringList m_cflags;       ///< Return value for MakefileParserThread::cflags()
    QStringList m_cxxflags;     ///< Return value for MakefileParserThread::cxxflags()

    ProjectExplorer::BuildSystem::ParseGuard m_guard;
};

} // AutotoolsProjectManager::Internal

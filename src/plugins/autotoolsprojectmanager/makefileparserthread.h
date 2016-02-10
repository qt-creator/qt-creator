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

#pragma once

#include "makefileparser.h"

#include <QMutex>
#include <QStringList>
#include <QThread>

namespace AutotoolsProjectManager {
namespace Internal {

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

public:
    MakefileParserThread(const QString &makefile);

    /** @see QThread::run() */
    void run();

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
     * @return Concatenated defines. Should be invoked, after the signal
     *         finished() has been emitted.
     */
    QByteArray defines() const;

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

private:
    MakefileParser m_parser;    ///< Is not accessible outside the thread

    mutable QMutex m_mutex;
    bool m_hasError = false;    ///< Return value for MakefileParserThread::hasError()
    QString m_executable;       ///< Return value for MakefileParserThread::executable()
    QStringList m_sources;      ///< Return value for MakefileParserThread::sources()
    QStringList m_makefiles;    ///< Return value for MakefileParserThread::makefiles()
    QStringList m_includePaths; ///< Return value for MakefileParserThread::includePaths()
    QByteArray m_defines;       ///< Return value for MakefileParserThread::defines()
    QStringList m_cflags;       ///< Return value for MakefileParserThread::cflags()
    QStringList m_cxxflags;     ///< Return value for MakefileParserThread::cxxflags()
};

} // namespace Internal
} // namespace AutotoolsProjectManager

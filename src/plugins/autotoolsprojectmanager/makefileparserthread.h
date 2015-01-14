/**************************************************************************
**
** Copyright (C) 2015 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MAKEFILEPARSERTHREAD_H
#define MAKEFILEPARSERTHREAD_H

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

public slots:
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
    bool m_hasError;            ///< Return value for MakefileParserThread::hasError()
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

#endif // MAKEFILEPARSERTHREAD_H



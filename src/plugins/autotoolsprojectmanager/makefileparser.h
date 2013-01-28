/**************************************************************************
**
** Copyright (C) 2013 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MAKEFILEPARSER_H
#define MAKEFILEPARSER_H

#include <QMutex>
#include <QStringList>
#include <QTextStream>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QDir)

namespace AutotoolsProjectManager {
namespace Internal {

/**
 * @brief Parses the autotools makefile Makefile.am.
 *
 * The parser returns the sources, makefiles and executable.
 * Variables like $(test) are not evaluated. If such a variable
 * is part of a SOURCES target, a fallback will be done and all
 * sub directories get parsed for C- and C++ files.
 */
class MakefileParser : public QObject
{
    Q_OBJECT

public:
    /**
     * @param makefile  Filename including path of the autotools
     *                  makefile that should be parsed.
     */
    MakefileParser(const QString &makefile);

    ~MakefileParser();

    /**
     * Parses the makefile. Must be invoked at least once, otherwise
     * the getter methods of MakefileParser will return empty values.
     * @return True, if the parsing was successful. If false is returned,
     *         the makefile could not be opened.
     */
    bool parse();

    /**
     * @return List of sources that are set for the _SOURCES target.
     *         Sources in sub directorties contain the sub directory as
     *         prefix.
     */
    QStringList sources() const;

    /**
     * @return List of Makefile.am files from the current directory and
     *         all sub directories. The values for sub directories contain
     *         the sub directory as prefix.
     */
    QStringList makefiles() const;

    /**
     * @return File name of the executable.
     */
    QString executable() const;

    /**
     * @return List of include paths. Should be invoked, after the signal
     *         finished() has been emitted.
     */
    QStringList includePaths() const;

    /**
     * Cancels the parsing. Calling this method only makes sense, if the
     * parser runs in a different thread than the caller of this method.
     * The method is thread-safe.
     */
    void cancel();

    /**
     * @return True, if the parser has been cancelled by MakefileParser::cancel().
     *         The method is thread-safe.
     */
    bool isCanceled() const;

signals:
    /**
     * Is emitted periodically during parsing the Makefile.am files
     * and the sub directories. \p status provides a translated
     * string, that can be shown to indicate the current state
     * of the parsing.
     */
    void status(const QString &status);

private:
    enum TopTarget {
        Undefined,
        AmDefaultSourceExt,
        BinPrograms,
        BuiltSources,
        Sources,
        SubDirs
    };

    TopTarget topTarget() const;

    /**
     * Parses the bin_PROGRAM target and stores it in m_executable.
     */
    void parseBinPrograms();

    /**
     * Parses all values from a _SOURCE target and appends them to
     * the m_sources list.
     */
    void parseSources();

    /**
     * Parses all sub directories for files having the extension
     * specified by 'AM_DEFAULT_SOURCE_EXT ='. The result will be
     * append to the m_sources list. Corresponding header files
     * will automatically be attached too.
     */
    void parseDefaultSourceExtensions();

    /**
     * Parses all sub directories specified by the SUBDIRS target and
     * adds the found sources to the m_sources list. The found makefiles
     * get added to the m_makefiles list.
     */
    void parseSubDirs();

    /**
     * Helper method for parseDefaultExtensions(). Returns recursively all sources
     * inside the directory \p directory that match with the extension \p extension.
     */
    QStringList directorySources(const QString &directory,
                                 const QStringList &extensions);

    /**
     * Helper method for all parse-methods. Returns each value of a target as string in
     * the stringlist. The current line m_line is used as starting point and increased
     * if the current line ends with a \.
     *
     * Example: For the text
     * \code
     * my_SOURCES = a.cpp\
     *              b.cpp c.cpp\
     *              d.cpp
     * \endcode
     * the string list contains all 4 *.cpp files. m_line is positioned to d.cpp afterwards.
     * Variables like $(test) are skipped and not part of the return value.
     *
     * @param hasVariables Optional output parameter. Is set to true, if the target values
     *                     contained a variable like $(test). Note that all variables are not
     *                     part of the return value, as they cannot get interpreted currently.
     */
    QStringList targetValues(bool *hasVariables = 0);

    /**
     * Adds recursively all sources of the current folder to m_sources and removes
     * all duplicates. The Makefile.am is not parsed, only the folders and files are
     * handled. This method should only be called, if the sources parsing in the Makefile.am
     * failed because variables (e.g. $(test)) have been used.
     */
    void addAllSources();

    /**
     * Adds all include paths to m_includePaths. TODO: Currently this is done
     * by parsing the generated Makefile. It might be more efficient and reliable
     * to parse the Makefile.am instead.
     */
    void parseIncludePaths();

    /**
     * Helper method for MakefileParser::directorySources(). Appends the name of the headerfile
     * to \p list, if the header could be found in the directory specified by \p dir.
     * The headerfile base name is defined by \p fileName.
     */
    static void appendHeader(QStringList &list, const QDir &dir, const QString &fileName);

private:
    bool m_success;             ///< Return value for MakefileParser::parse().

    bool m_cancel;              ///< True, if the parsing should be cancelled.
    mutable QMutex m_mutex;     ///< Mutex to protect m_cancel.

    QString m_makefile;         ///< Filename of the makefile
    QString m_executable;       ///< Return value for MakefileParser::executable()
    QStringList m_sources;      ///< Return value for MakefileParser::sources()
    QStringList m_makefiles;    ///< Return value for MakefileParser::makefiles()
    QStringList m_includePaths; ///< Return value for MakefileParser::includePaths()

    QString m_line;             ///< Current line of the makefile
    QTextStream m_textStream;   ///< Textstream that represents the makefile
};

} // namespace Internal
} // namespace AutotoolsProjectManager

#endif // MAKEFILEPARSER_H


// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "makefileparser.h"

#include "autotoolsprojectmanagertr.h"

#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

using namespace ProjectExplorer;
using namespace Utils;

namespace AutotoolsProjectManager::Internal {

/**
 * @brief Parses the autotools makefile Makefile.am.
 *
 * The parser returns the sources, makefiles and executable.
 * Variables like $(test) are not evaluated. If such a variable
 * is part of a SOURCES target, a fallback will be done and all
 * sub directories get parsed for C- and C++ files.
 */
class MakefileParser final
{
public:
    /**
     * @param makefile  Filename including path of the autotools
     *                  makefile that should be parsed.
     */
    MakefileParser(const QString &makefile) : m_makefile(makefile) {}

    /**
     * Parses the makefile. Must be invoked at least once, otherwise
     * the getter functions of MakefileParser will return empty values.
     * @return True, if the parsing was successful. If false is returned,
     *         the makefile could not be opened.
     */
    bool parse(const QFuture<void> &future);

    MakefileParserOutputData outputData() const { return m_outputData; }

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
    void parseBinPrograms(QTextStream *textStream);

    /**
     * Parses all values from a _SOURCE target and appends them to
     * the m_sources list.
     */
    void parseSources(QTextStream *textStream);

    /**
     * Parses all sub directories for files having the extension
     * specified by 'AM_DEFAULT_SOURCE_EXT ='. The result will be
     * append to the m_sources list. Corresponding header files
     * will automatically be attached too.
     */
    void parseDefaultSourceExtensions(QTextStream *textStream);

    /**
     * Parses all sub directories specified by the SUBDIRS target and
     * adds the found sources to the m_sources list. The found makefiles
     * get added to the m_makefiles list.
     */
    void parseSubDirs(QTextStream *textStream);

    /**
     * Helper function for parseDefaultExtensions(). Returns recursively all sources
     * inside the directory \p directory that match with the extension \p extension.
     */
    QStringList directorySources(const QString &directory, const QStringList &extensions);

    /**
     * Helper function for all parse-functions. Returns each value of a target as string in
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
    QStringList targetValues(QTextStream *textStream, bool *hasVariables = nullptr);

    /**
     * Adds recursively all sources of the current folder to m_sources and removes
     * all duplicates. The Makefile.am is not parsed, only the folders and files are
     * handled. This function should only be called, if the sources parsing in the Makefile.am
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
     * Helper function for MakefileParser::directorySources(). Appends the name of the headerfile
     * to \p list, if the header could be found in the directory specified by \p dir.
     * The headerfile base name is defined by \p fileName.
     */
    static void appendHeader(QStringList &list, const QDir &dir, const QString &fileName);

    /**
     * If line starts with identifier and = goes next, return identifier.
     * Identifier is valid target name and it matches regexp [a-zA-Z1-9_]+
     */
    static QString parseIdentifierBeforeAssign(const QString &line);

    /**
     * Parses list of space-separated terms after "="
     */
    static QStringList parseTermsAfterAssign(const QString &line);

    /**
     * If term is compiler flag -D<macro>, adds macro to defines and returns true.
     */
    bool maybeParseDefine(const QString &term);

    /**
     * If term is compiler flag -I<path>, adds path to includes and returns true.
     * @param term Term itself
     * @param dirName Directory where Makefile placed
     */
    bool maybeParseInclude(const QString &term, const QString &dirName);

    /**
     * If term is compiler flag -<flag>, adds it to cflags and returns true.
     */
    bool maybeParseCFlag(const QString &term);

    /**
     * If term is compiler flag -<flag>, adds it to cxxflags and returns true.
     */
    bool maybeParseCXXFlag(const QString &term);

    /**
     * If term is compiler flag -<flag>, adds it to cppflags and returns true.
     */
    bool maybeParseCPPFlag(const QString &term);

    bool m_success = true;      ///< Return value for MakefileParser::parse().
    bool m_subDirsEmpty = false;///< States if last subdirs var was empty

    QFuture<void> m_future;     ///< For periodic checking of cancelled state.

    QString m_makefile;         ///< Filename of the makefile
    MakefileParserOutputData m_outputData;
    QStringList m_cppflags;     ///< The cpp flags, which will be part of both cflags and cxxflags

    QString m_line;             ///< Current line of the makefile
};

bool MakefileParser::parse(const QFuture<void> &future)
{
    m_future = future;

    QFile file(m_makefile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("%s: %s", qPrintable(m_makefile), qPrintable(file.errorString()));
        return false;
    }

    QFileInfo info(m_makefile);
    m_outputData.m_makefiles.append(info.fileName());

    QTextStream textStream(&file);

    do {
        m_line = textStream.readLine();
        switch (topTarget()) {
        case AmDefaultSourceExt: parseDefaultSourceExtensions(&textStream); break;
        case BinPrograms: parseBinPrograms(&textStream); break;
        case BuiltSources: break; // TODO: Add to m_sources?
        case Sources: parseSources(&textStream); break;
        case SubDirs: parseSubDirs(&textStream); break;
        case Undefined:
        default: break;
        }
    } while (!m_line.isNull());

    parseIncludePaths();

    if (m_subDirsEmpty)
        m_success = false;

    return m_success;
}

MakefileParser::TopTarget MakefileParser::topTarget() const
{
    const QString line = m_line.simplified();

    if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
        return Undefined;

    const QString id = parseIdentifierBeforeAssign(line);
    if (id.isEmpty())
        return Undefined;

    if (id == QLatin1String("AM_DEFAULT_SOURCE_EXT"))
        return AmDefaultSourceExt;
    if (id == QLatin1String("bin_PROGRAMS"))
        return BinPrograms;
    if (id == QLatin1String("BUILT_SOURCES"))
        return BuiltSources;
    if (id == QLatin1String("SUBDIRS") || id == QLatin1String("DIST_SUBDIRS"))
        return SubDirs;
    if (id.endsWith("_SOURCES") || id.endsWith("_HEADERS"))
        return Sources;

    return Undefined;
}

void MakefileParser::parseBinPrograms(QTextStream *textStream)
{
    QTC_ASSERT(m_line.contains(QLatin1String("bin_PROGRAMS")), return);
    const QStringList binPrograms = targetValues(textStream);

    // TODO: are multiple values possible?
    if (binPrograms.size() == 1) {
        QFileInfo info(binPrograms.first());
        m_outputData.m_executable = info.fileName();
    }
}

void MakefileParser::parseSources(QTextStream *textStream)
{
    QTC_ASSERT(m_line.contains("_SOURCES") || m_line.contains("_HEADERS"), return);

    bool hasVariables = false;
    m_outputData.m_sources.append(targetValues(textStream, &hasVariables));

    // Skip parsing of Makefile.am for getting the sub directories,
    // as variables have been used. As fallback all sources will be added.
    if (hasVariables)
        addAllSources();

    // Duplicates might be possible in combination with 'AM_DEFAULT_SOURCE_EXT ='
    m_outputData.m_sources.removeDuplicates();

    // TODO: Definitions like "SOURCES = ../src.cpp" are ignored currently.
    // This case must be handled correctly in MakefileParser::parseSubDirs(),
    // where the current sub directory must be shortened.
    QStringList::iterator it = m_outputData.m_sources.begin();
    while (it != m_outputData.m_sources.end()) {
        if ((*it).startsWith(QLatin1String("..")))
            it = m_outputData.m_sources.erase(it);
        else
            ++it;
    }
}

void MakefileParser::parseDefaultSourceExtensions(QTextStream *textStream)
{
    QTC_ASSERT(m_line.contains(QLatin1String("AM_DEFAULT_SOURCE_EXT")), return);
    const QStringList extensions = targetValues(textStream);
    if (extensions.isEmpty()) {
        m_success = false;
        return;
    }

    QFileInfo info(m_makefile);
    const QString dirName = info.absolutePath();
    m_outputData.m_sources.append(directorySources(dirName, extensions));

    // Duplicates might be possible in combination with '_SOURCES ='
    m_outputData.m_sources.removeDuplicates();
}

void MakefileParser::parseSubDirs(QTextStream *textStream)
{
    QTC_ASSERT(m_line.contains(QLatin1String("SUBDIRS")), return);
    if (m_future.isCanceled()) {
        m_success = false;
        return;
    }

    QFileInfo info(m_makefile);
    const QString path = info.absolutePath();
    const QString makefileName = info.fileName();

    bool hasVariables = false;
    QStringList subDirs = targetValues(textStream, &hasVariables);
    if (hasVariables) {
        // Skip parsing of Makefile.am for getting the sub directories,
        // as variables have been used. As fallback all sources will be added.
        addAllSources();
        return;
    }

    // If the SUBDIRS values contain a '.' or a variable like $(test),
    // all the sub directories of the current folder must get parsed.
    bool hasDotSubDir = false;
    QStringList::iterator it = subDirs.begin();
    while (it != subDirs.end()) {
        // Erase all entries that represent a '.'
        if ((*it) == QLatin1String(".")) {
            hasDotSubDir = true;
            it = subDirs.erase(it);
        } else {
            ++it;
        }
    }
    if (hasDotSubDir) {
        // Add all sub directories of the current folder
        QDir dir(path);
        dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
        const QFileInfoList infoList = dir.entryInfoList();
        for (const QFileInfo &info : infoList)
            subDirs.append(info.fileName());
    }
    subDirs.removeDuplicates();

    // Delegate the parsing of all sub directories to a local
    // makefile parser and merge the results
    for (const QString &subDir : std::as_const(subDirs)) {
        const QChar slash = QLatin1Char('/');
        const QString subDirMakefile = path + slash + subDir
                                       + slash + makefileName;

        // Parse sub directory
        QFile file(subDirMakefile);

        // Don't try to parse a file, that might not exist (e. g.
        // if SUBDIRS specifies a 'po' directory).
        if (!file.exists())
            continue;

        MakefileParser parser(subDirMakefile);
        const bool success = parser.parse(m_future);

        // Don't return, try to parse as many sub directories
        // as possible
        if (!success)
            m_success = false;

        const MakefileParserOutputData result = parser.outputData();

        m_outputData.m_makefiles.append(subDir + slash + makefileName);

        // Append the sources of the sub directory to the current sources
        for (const QString &source : result.m_sources)
            m_outputData.m_sources.append(subDir + slash + source);

        // Append the include paths of the sub directory
        m_outputData.m_includePaths.append(result.m_includePaths);

        // Append the flags of the sub directory
        m_outputData.m_cflags.append(result.m_cflags);
        m_outputData.m_cxxflags.append(result.m_cxxflags);

        // Append the macros of the sub directory
        for (const Macro &macro : result.m_macros) {
            if (!m_outputData.m_macros.contains(macro))
                m_outputData.m_macros.append(macro);
        }
    }

    // Duplicates might be possible in combination with several
    // "..._SUBDIRS" targets
    m_outputData.m_makefiles.removeDuplicates();
    m_outputData.m_sources.removeDuplicates();

    m_subDirsEmpty = subDirs.isEmpty();
}

QStringList MakefileParser::directorySources(const QString &directory,
                                             const QStringList &extensions)
{
    if (m_future.isCanceled()) {
        m_success = false;
        return {};
    }

    QStringList list; // return value

    QDir dir(directory);
    dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    const QFileInfoList infos = dir.entryInfoList();
    for (const QFileInfo &info : infos) {
        if (info.isDir()) {
            // Append recursively sources from the sub directory
            const QStringList subDirSources = directorySources(info.absoluteFilePath(),
                                                               extensions);
            const QString dirPath = info.fileName();
            for (const QString &subDirSource : subDirSources)
                list.append(dirPath + QLatin1Char('/') + subDirSource);
        } else {
            // Check whether the file matches to an extension
            for (const QString &extension : extensions) {
                if (info.fileName().endsWith(extension)) {
                    list.append(info.fileName());
                    appendHeader(list, dir, info.baseName());
                    break;
                }
            }
        }
    }

    return list;
}

QStringList MakefileParser::targetValues(QTextStream *textStream, bool *hasVariables)
{
    QStringList values;
    if (hasVariables)
        *hasVariables = false;

    const int index = m_line.indexOf(QLatin1Char('='));
    if (index < 0) {
        m_success = false;
        return {};
    }

    m_line.remove(0, index + 1); // remove the 'target = ' prefix

    bool endReached = false;
    do {
        m_line = m_line.simplified();

        // Get all values of a line separated by spaces.
        // Values representing a variable like $(value) get
        // removed currently.
        QStringList lineValues = m_line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        QStringList::iterator it = lineValues.begin();
        while (it != lineValues.end()) {
            if ((*it).startsWith(QLatin1String("$("))) {
                it = lineValues.erase(it);
                if (hasVariables)
                    *hasVariables = true;
            } else {
                ++it;
            }
        }

        endReached = lineValues.isEmpty();
        if (!endReached) {
            const QChar backSlash = QLatin1Char('\\');
            QString last = lineValues.last();
            if (last.endsWith(backSlash)) {
                // The last value contains a backslash. Remove the
                // backslash and replace the last value.
                lineValues.pop_back();
                last.remove(backSlash);
                if (!last.isEmpty())
                    lineValues.push_back(last);

                values.append(lineValues);
                m_line = textStream->readLine();
                endReached = m_line.isNull();
            } else {
                values.append(lineValues);
                endReached = true;
            }
        }
    } while (!endReached);

    return values;
}

void MakefileParser::appendHeader(QStringList &list,  const QDir &dir, const QString &fileName)
{
    const char *const headerExtensions[] = {".h", ".hh", ".hg", ".hxx", ".hpp", nullptr};
    int i = 0;
    while (headerExtensions[i]) {
        const QString headerFile = fileName + QLatin1String(headerExtensions[i]);
        QFileInfo fileInfo(dir, headerFile);
        if (fileInfo.exists())
            list.append(headerFile);
        ++i;
    }
}

QString MakefileParser::parseIdentifierBeforeAssign(const QString &line)
{
    int end = 0;
    for (; end < line.size(); ++end)
        if (!line[end].isLetterOrNumber() && line[end] != '_')
            break;

    QString ret = line.left(end);
    while (end < line.size() && line[end].isSpace())
        ++end;
    return ((end < line.size() && line[end] == '=') ||
            (end < line.size() - 1 && line[end] == '+' && line[end + 1] == '=')) ? ret : QString();
}

QStringList MakefileParser::parseTermsAfterAssign(const QString &line)
{
    int assignPos = line.indexOf(QLatin1Char('=')) + 1;
    if (assignPos <= 0 || assignPos >= line.size())
        return {};

    const QStringList parts = ProcessArgs::splitArgs(line.mid(assignPos), HostOsInfo::hostOs());
    QStringList result;
    for (int i = 0; i < parts.count(); ++i) {
        const QString cur = parts.at(i);
        const QString next = (i == parts.count() - 1) ? QString() : parts.at(i + 1);
        if (cur == QLatin1String("-D") || cur == QLatin1String("-U") || cur == QLatin1String("-I")) {
            result << cur + next;
            ++i;
        } else {
            result << cur;
        }
    }
    return result;
}

bool MakefileParser::maybeParseDefine(const QString &term)
{
    if (term.startsWith(QLatin1String("-D"))) {
        QString def = term.mid(2); // remove the "-D"
        m_outputData.m_macros += Macro::fromKeyValue(def);
        return true;
    }
    return false;
}

bool MakefileParser::maybeParseInclude(const QString &term, const QString &dirName)
{
    if (term.startsWith(QLatin1String("-I"))) {
        QString includePath = term.mid(2); // remove the "-I"
        if (includePath == QLatin1String("."))
            includePath = dirName;
        if (!includePath.isEmpty())
            m_outputData.m_includePaths += includePath;
        return true;
    }
    return false;
}

bool MakefileParser::maybeParseCFlag(const QString &term)
{
    if (term.startsWith(QLatin1Char('-'))) {
        m_outputData.m_cflags += term;
        return true;
    }
    return false;
}

bool MakefileParser::maybeParseCXXFlag(const QString &term)
{
    if (term.startsWith(QLatin1Char('-'))) {
        m_outputData.m_cxxflags += term;
        return true;
    }
    return false;
}

bool MakefileParser::maybeParseCPPFlag(const QString &term)
{
    if (term.startsWith(QLatin1Char('-'))) {
        m_cppflags += term;
        return true;
    }
    return false;
}

static QStringList extensions()
{
    static const QStringList extList = {".c", ".cpp", ".cc", ".cxx", ".ccg"};
    return extList;
}

void MakefileParser::addAllSources()
{
    const QFileInfo info(m_makefile);
    m_outputData.m_sources.append(directorySources(info.absolutePath(), extensions()));
    m_outputData.m_sources.removeDuplicates();
}

void MakefileParser::parseIncludePaths()
{
    QFileInfo info(m_makefile);
    const QString dirName = info.absolutePath();

    QFile file(dirName + QLatin1String("/Makefile"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    // TODO: Targets are ignored at this moment.
    // Whether it is worth to improve this, depends on whether
    // we want to parse the generated Makefile at all or whether we want to
    // improve the Makefile.am parsing to be aware of variables.
    QTextStream textStream(&file);
    QString line;
    do {
        line = textStream.readLine();
        while (line.endsWith(QLatin1Char('\\'))) {
            line.chop(1);
            QString next = textStream.readLine();
            line.append(next);
        }

        const QString varName = parseIdentifierBeforeAssign(line);
        if (varName.isEmpty())
            continue;

        const QStringList termList = parseTermsAfterAssign(line);
        if (varName == QLatin1String("DEFS")) {
            for (const QString &term : termList)
                maybeParseDefine(term);
        } else if (varName.endsWith(QLatin1String("INCLUDES"))) {
            for (const QString &term : termList)
                maybeParseInclude(term, dirName);
        } else if (varName.endsWith(QLatin1String("CFLAGS"))) {
            for (const QString &term : termList)
                maybeParseDefine(term) || maybeParseInclude(term, dirName)
                        || maybeParseCFlag(term);
        } else if (varName.endsWith(QLatin1String("CXXFLAGS"))) {
            for (const QString &term : termList)
                maybeParseDefine(term) || maybeParseInclude(term, dirName)
                        || maybeParseCXXFlag(term);
        } else if (varName.endsWith(QLatin1String("CPPFLAGS"))) {
            for (const QString &term : termList)
                maybeParseDefine(term) || maybeParseInclude(term, dirName)
                        || maybeParseCPPFlag(term);
        }
    } while (!line.isNull());

    m_outputData.m_includePaths.removeDuplicates();
    m_outputData.m_cflags.removeDuplicates();
    m_outputData.m_cxxflags.removeDuplicates();
}

std::optional<MakefileParserOutputData> parseMakefile(const QString &makefile,
                                                      const QFuture<void> &future)
{
    MakefileParser parser(makefile);
    if (parser.parse(future))
        return parser.outputData();
    return {};
}

} // AutotoolsProjectManager::Internal

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

#include "makefileparser.h"

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QFile>
#include <QDir>
#include <QFileInfoList>
#include <QMutexLocker>

using namespace AutotoolsProjectManager::Internal;

MakefileParser::MakefileParser(const QString &makefile) : m_makefile(makefile)
{ }

MakefileParser::~MakefileParser()
{
    delete m_textStream.device();
}

bool MakefileParser::parse()
{
    m_mutex.lock();
    m_cancel = false;
    m_mutex.unlock(),

    m_success = true;
    m_executable.clear();
    m_sources.clear();
    m_makefiles.clear();

    QFile *file = new QFile(m_makefile);
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("%s: %s", qPrintable(m_makefile), qPrintable(file->errorString()));
        delete file;
        return false;
    }

    QFileInfo info(m_makefile);
    m_makefiles.append(info.fileName());

    emit status(tr("Parsing %1 in directory %2").arg(info.fileName()).arg(info.absolutePath()));

    m_textStream.setDevice(file);

    do {
        m_line = m_textStream.readLine();
        switch (topTarget()) {
        case AmDefaultSourceExt: parseDefaultSourceExtensions(); break;
        case BinPrograms: parseBinPrograms(); break;
        case BuiltSources: break; // TODO: Add to m_sources?
        case Sources: parseSources(); break;
        case SubDirs: parseSubDirs(); break;
        case Undefined:
        default: break;
        }
    } while (!m_line.isNull());

    parseIncludePaths();

    return m_success;
}

QStringList MakefileParser::sources() const
{
    return m_sources;
}

QStringList MakefileParser::makefiles() const
{
    return m_makefiles;
}

QString MakefileParser::executable() const
{
    return m_executable;
}

QStringList MakefileParser::includePaths() const
{
    return m_includePaths;
}

QByteArray MakefileParser::defines() const
{
    return m_defines;
}

QStringList MakefileParser::cflags() const
{
    return m_cppflags + m_cflags;
}

QStringList MakefileParser::cxxflags() const
{
    return m_cppflags + m_cxxflags;
}

void MakefileParser::cancel()
{
    QMutexLocker locker(&m_mutex);
    m_cancel = true;
}

bool MakefileParser::isCanceled() const
{
    QMutexLocker locker(&m_mutex);
    return m_cancel;
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
    if (id.endsWith(QLatin1String("_SOURCES")))
        return Sources;

    return Undefined;
}

void MakefileParser::parseBinPrograms()
{
    QTC_ASSERT(m_line.contains(QLatin1String("bin_PROGRAMS")), return);
    const QStringList binPrograms = targetValues();

    // TODO: are multiple values possible?
    if (binPrograms.size() == 1) {
        QFileInfo info(binPrograms.first());
        m_executable = info.fileName();
    }
}

void MakefileParser::parseSources()
{
    QTC_ASSERT(m_line.contains(QLatin1String("_SOURCES")), return);

    bool hasVariables = false;
    m_sources.append(targetValues(&hasVariables));

    // Skip parsing of Makefile.am for getting the sub directories,
    // as variables have been used. As fallback all sources will be added.
    if (hasVariables)
        addAllSources();

    // Duplicates might be possible in combination with 'AM_DEFAULT_SOURCE_EXT ='
    m_sources.removeDuplicates();

    // TODO: Definitions like "SOURCES = ../src.cpp" are ignored currently.
    // This case must be handled correctly in MakefileParser::parseSubDirs(),
    // where the current sub directory must be shortened.
    QStringList::iterator it = m_sources.begin();
    while (it != m_sources.end()) {
        if ((*it).startsWith(QLatin1String("..")))
            it = m_sources.erase(it);
        else
            ++it;
    }
}

void MakefileParser::parseDefaultSourceExtensions()
{
    QTC_ASSERT(m_line.contains(QLatin1String("AM_DEFAULT_SOURCE_EXT")), return);
    const QStringList extensions = targetValues();
    if (extensions.isEmpty()) {
        m_success = false;
        return;
    }

    QFileInfo info(m_makefile);
    const QString dirName = info.absolutePath();
    m_sources.append(directorySources(dirName, extensions));

    // Duplicates might be possible in combination with '_SOURCES ='
    m_sources.removeDuplicates();
}

void MakefileParser::parseSubDirs()
{
    QTC_ASSERT(m_line.contains(QLatin1String("SUBDIRS")), return);
    if (isCanceled()) {
        m_success = false;
        return;
    }

    QFileInfo info(m_makefile);
    const QString path = info.absolutePath();
    const QString makefileName = info.fileName();

    bool hasVariables = false;
    QStringList subDirs = targetValues(&hasVariables);
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
        foreach (const QFileInfo& info, dir.entryInfoList()) {
            subDirs.append(info.fileName());
        }
    }
    subDirs.removeDuplicates();

    // Delegate the parsing of all sub directories to a local
    // makefile parser and merge the results
    foreach (const QString& subDir, subDirs) {
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
        connect(&parser, &MakefileParser::status, this, &MakefileParser::status);
        const bool success = parser.parse();

        // Don't return, try to parse as many sub directories
        // as possible
        if (!success)
            m_success = false;

        m_makefiles.append(subDir + slash + makefileName);

        // Append the sources of the sub directory to the
        // current sources
        foreach (const QString& source, parser.sources())
            m_sources.append(subDir + slash + source);

        // Duplicates might be possible in combination with several
        // "..._SUBDIRS" targets
        m_makefiles.removeDuplicates();
        m_sources.removeDuplicates();
    }

    if (subDirs.isEmpty())
        m_success = false;
}

QStringList MakefileParser::directorySources(const QString &directory,
                                             const QStringList &extensions)
{
    if (isCanceled()) {
        m_success = false;
        return QStringList();
    }

    emit status(tr("Parsing directory %1").arg(directory));

    QStringList list; // return value

    QDir dir(directory);
    dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    const QFileInfoList infos = dir.entryInfoList();
    foreach (const QFileInfo& info, infos) {
        if (info.isDir()) {
            // Append recursively sources from the sub directory
            const QStringList subDirSources = directorySources(info.absoluteFilePath(),
                                                               extensions);
            const QString dirPath = info.fileName();
            foreach (const QString& subDirSource, subDirSources)
                list.append(dirPath + QLatin1Char('/') + subDirSource);
        } else {
            // Check whether the file matches to an extension
            foreach (const QString& extension, extensions) {
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

QStringList MakefileParser::targetValues(bool *hasVariables)
{
    QStringList values;
    if (hasVariables != 0)
        *hasVariables = false;

    const int index = m_line.indexOf(QLatin1Char('='));
    if (index < 0) {
        m_success = false;
        return QStringList();
    }

    m_line.remove(0, index + 1); // remove the 'target = ' prefix

    bool endReached = false;
    do {
        m_line = m_line.simplified();

        // Get all values of a line separated by spaces.
        // Values representing a variable like $(value) get
        // removed currently.
        QStringList lineValues = m_line.split(QLatin1Char(' '), QString::SkipEmptyParts);
        QStringList::iterator it = lineValues.begin();
        while (it != lineValues.end()) {
            if ((*it).startsWith(QLatin1String("$("))) {
                it = lineValues.erase(it);
                if (hasVariables != 0)
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
                m_line = m_textStream.readLine();
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
    const char *const headerExtensions[] = {".h", ".hh", ".hg", ".hxx", ".hpp", 0};
    int i = 0;
    while (headerExtensions[i] != 0) {
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
        if (!line[end].isLetterOrNumber() && line[end] != QLatin1Char('_'))
            break;

    QString ret = line.left(end);
    while (end < line.size() && line[end].isSpace())
        ++end;
    return (end < line.size() && line[end] == QLatin1Char('=')) ? ret : QString();
}

QStringList MakefileParser::parseTermsAfterAssign(const QString &line)
{
    int assignPos = line.indexOf(QLatin1Char('=')) + 1;
    if (assignPos <= 0 || assignPos >= line.size())
        return QStringList();

    const QStringList parts = Utils::QtcProcess::splitArgs(line.mid(assignPos));
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
        QByteArray data = def.toUtf8();
        int pos = data.indexOf('=');
        if (pos >= 0)
            data[pos] = ' ';
        m_defines += (QByteArray("#define ") + data + '\n');
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
            m_includePaths += includePath;
        return true;
    }
    return false;
}

bool MakefileParser::maybeParseCFlag(const QString &term)
{
    if (term.startsWith(QLatin1Char('-'))) {
        m_cflags += term;
        return true;
    }
    return false;
}

bool MakefileParser::maybeParseCXXFlag(const QString &term)
{
    if (term.startsWith(QLatin1Char('-'))) {
        m_cxxflags += term;
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

void MakefileParser::addAllSources()
{
    QStringList extensions;
    extensions << QLatin1String(".c")
               << QLatin1String(".cpp")
               << QLatin1String(".cc")
               << QLatin1String(".cxx")
               << QLatin1String(".ccg");
    QFileInfo info(m_makefile);
    m_sources.append(directorySources(info.absolutePath(), extensions));
    m_sources.removeDuplicates();
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

        if (varName == QLatin1String("DEFS")) {
            foreach (const QString &term, parseTermsAfterAssign(line))
                maybeParseDefine(term);
        } else if (varName.endsWith(QLatin1String("INCLUDES"))) {
            foreach (const QString &term, parseTermsAfterAssign(line))
                maybeParseInclude(term, dirName);
        } else if (varName.endsWith(QLatin1String("CFLAGS"))) {
            foreach (const QString &term, parseTermsAfterAssign(line))
                maybeParseDefine(term) || maybeParseInclude(term, dirName)
                        || maybeParseCFlag(term);
        } else if (varName.endsWith(QLatin1String("CXXFLAGS"))) {
            foreach (const QString &term, parseTermsAfterAssign(line))
                maybeParseDefine(term) || maybeParseInclude(term, dirName)
                        || maybeParseCXXFlag(term);
        } else if (varName.endsWith(QLatin1String("CPPFLAGS"))) {
            foreach (const QString &term, parseTermsAfterAssign(line))
                maybeParseDefine(term) || maybeParseInclude(term, dirName)
                        || maybeParseCPPFlag(term);
        }
    } while (!line.isNull());

    m_includePaths.removeDuplicates();
    m_cflags.removeDuplicates();
    m_cxxflags.removeDuplicates();
}

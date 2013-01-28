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

#include "makefileparser.h"

#include <utils/qtcassert.h>

#include <QFile>
#include <QDir>
#include <QFileInfoList>
#include <QMutexLocker>

using namespace AutotoolsProjectManager::Internal;

MakefileParser::MakefileParser(const QString &makefile) :
    QObject(),
    m_success(false),
    m_cancel(false),
    m_mutex(),
    m_makefile(makefile),
    m_executable(),
    m_sources(),
    m_makefiles(),
    m_includePaths(),
    m_line(),
    m_textStream()
{
}

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
    TopTarget topTarget = Undefined;

    const QString line = m_line.simplified();
    if (!line.isEmpty() && !line.startsWith(QLatin1Char('#'))) {
        // TODO: Check how many fixed strings like AM_DEFAULT_SOURCE_EXT will
        // be needed vs. variable strings like _SOURCES. Dependent on this a
        // more clever way than this (expensive) if-cascading might be done.
        if (line.startsWith(QLatin1String("AM_DEFAULT_SOURCE_EXT =")))
            topTarget = AmDefaultSourceExt;
        else if (line.startsWith(QLatin1String("bin_PROGRAMS =")))
            topTarget = BinPrograms;
        else if (line.startsWith(QLatin1String("BUILT_SOURCES =")))
            topTarget = BuiltSources;
        else if (line.contains(QLatin1String("SUBDIRS =")))
            topTarget = SubDirs;
        else if (line.contains(QLatin1String("_SOURCES =")))
            topTarget = Sources;
    }

    return topTarget;
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
        connect(&parser, SIGNAL(status(QString)), this, SIGNAL(status(QString)));
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
    const char *const headerExtensions[] = { ".h", ".hh", ".hg", ".hxx", ".hpp", 0 };
    int i = 0;
    while (headerExtensions[i] != 0) {
        const QString headerFile = fileName + QLatin1String(headerExtensions[i]);
        QFileInfo fileInfo(dir, headerFile);
        if (fileInfo.exists())
            list.append(headerFile);
        ++i;
    }
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

    // TODO: The parsing is done very poor. Comments are ignored and targets
    // are ignored too. Whether it is worth to improve this, depends on whether
    // we want to parse the generated Makefile at all or whether we want to
    // improve the Makefile.am parsing to be aware of variables.
    QTextStream textStream(&file);
    QString line;
    do {
        line = textStream.readLine();
        QStringList terms = line.split(QLatin1Char(' '), QString::SkipEmptyParts);
        foreach (const QString &term, terms) {
            if (term.startsWith(QLatin1String("-I"))) {
                QString includePath = term.right(term.length() - 2); // remove the "-I"
                if (includePath == QLatin1String("."))
                    includePath = dirName;
                if (!includePath.isEmpty())
                    m_includePaths += includePath;
            }
        }
    } while (!line.isNull());

    m_includePaths.removeDuplicates();
}

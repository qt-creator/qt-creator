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

#include "ioutputparser.h"
#include "task.h"

#include <utils/algorithm.h>
#include <utils/fileinprojectfinder.h>
#include <utils/synchronousprocess.h>

#include <QDir>
#include <QFileInfo>
#include <QPointer>


/*!
    \class ProjectExplorer::OutputTaskParser

    \brief The OutputTaskParser class provides an interface for an output parser
    that emits issues (tasks).

    \sa ProjectExplorer::Task
*/

/*!
   \fn ProjectExplorer::OutputTaskParser::Status ProjectExplorer::OutputTaskParser::handleLine(const QString &line, Utils::OutputFormat type)

   Called once for each line of standard output or standard error to parse.
*/

/*!
   \fn bool ProjectExplorer::OutputTaskParser::hasFatalErrors() const

   This is mainly a Symbian specific quirk.
*/

/*!
   \fn void ProjectExplorer::OutputTaskParser::addTask(const ProjectExplorer::Task &task)

   Should be emitted for each task seen in the output.
*/

/*!
   \fn void ProjectExplorer::OutputTaskParser::flush()

   Instructs a parser to flush its state.
   Parsers may have state (for example, because they need to aggregate several
   lines into one task). This
   function is called when this state needs to be flushed out to be visible.
*/

namespace ProjectExplorer {

class OutputTaskParser::Private
{
public:
    Utils::FilePaths searchDirs;
    Utils::FileInProjectFinder *fileFinder = nullptr;
    QPointer<const OutputTaskParser> redirectionDetector;
    bool skipFileExistsCheck = false;
};

OutputTaskParser::OutputTaskParser() : d(new Private)
{
}

OutputTaskParser::~OutputTaskParser() { delete d; }

void OutputTaskParser::addSearchDir(const Utils::FilePath &dir)
{
    d->searchDirs << dir;
}

void OutputTaskParser::dropSearchDir(const Utils::FilePath &dir)
{
    const int idx = d->searchDirs.lastIndexOf(dir);

    // TODO: This apparently triggers. Find out why and either remove the assertion (if it's legit)
    //       or fix the culprit.
    QTC_ASSERT(idx != -1, return);

    d->searchDirs.removeAt(idx);
}

const Utils::FilePaths OutputTaskParser::searchDirectories() const
{
    return d->searchDirs;
}

// The redirection mechanism is needed for broken build tools (e.g. xcodebuild) that get invoked
// indirectly as part of the build process and redirect their child processes' stderr output
// to stdout. A parser might be able to detect this condition and inform interested
// other parsers that they need to interpret stdout data as stderr.
void OutputTaskParser::setRedirectionDetector(const OutputTaskParser *detector)
{
    d->redirectionDetector = detector;
}

bool OutputTaskParser::needsRedirection() const
{
    return d->redirectionDetector && (d->redirectionDetector->hasDetectedRedirection()
                                      || d->redirectionDetector->needsRedirection());
}

void OutputTaskParser::setFileFinder(Utils::FileInProjectFinder *finder)
{
    d->fileFinder = finder;
}

Utils::FilePath OutputTaskParser::absoluteFilePath(const Utils::FilePath &filePath)
{
    if (filePath.isEmpty() || filePath.toFileInfo().isAbsolute())
        return filePath;
    Utils::FilePaths candidates;
    for (const Utils::FilePath &dir : searchDirectories()) {
        const Utils::FilePath candidate = dir.pathAppended(filePath.toString());
        if (candidate.exists() || d->skipFileExistsCheck)
            candidates << candidate;
    }
    if (candidates.count() == 1)
        return Utils::FilePath::fromString(QDir::cleanPath(candidates.first().toString()));

    QString fp = filePath.toString();
    while (fp.startsWith("../"))
        fp.remove(0, 3);
    bool found = false;
    candidates = d->fileFinder->findFile(QUrl::fromLocalFile(fp), &found);
    if (found && candidates.size() == 1)
        return candidates.first();

    return filePath;
}

QString OutputTaskParser::rightTrimmed(const QString &in)
{
    int pos = in.length();
    for (; pos > 0; --pos) {
        if (!in.at(pos - 1).isSpace())
            break;
    }
    return in.mid(0, pos);
}

#ifdef WITH_TESTS
void OutputTaskParser::skipFileExistsCheck()
{
    d->skipFileExistsCheck = true;
}
#endif

class IOutputParser::OutputChannelState
{
public:
    using LineHandler = void (IOutputParser::*)(const QString &line);

    OutputChannelState(IOutputParser *parser, Utils::OutputFormat type)
        : parser(parser), type(type) {}

    void handleData(const QString &newData)
    {
        pendingData += newData;
        pendingData = Utils::SynchronousProcess::normalizeNewlines(pendingData);
        while (true) {
            const int eolPos = pendingData.indexOf('\n');
            if (eolPos == -1)
                break;
            const QString line = pendingData.left(eolPos + 1);
            pendingData.remove(0, eolPos + 1);
            parser->handleLine(line, type);
        }
    }

    void flush()
    {
        if (!pendingData.isEmpty()) {
            parser->handleLine(pendingData, type);
            pendingData.clear();
        }
    }

    IOutputParser * const parser;
    const Utils::OutputFormat type;
    QString pendingData;
};

class IOutputParser::IOutputParserPrivate
{
public:
    IOutputParserPrivate(IOutputParser *parser)
        : stdoutState(parser, Utils::StdOutFormat),
          stderrState(parser, Utils::StdErrFormat)
    {}

    QList<OutputTaskParser *> lineParsers;
    OutputTaskParser *nextParser = nullptr;
    QList<Filter> filters;
    Utils::FileInProjectFinder fileFinder;
    OutputChannelState stdoutState;
    OutputChannelState stderrState;
};

IOutputParser::IOutputParser() : d(new IOutputParserPrivate(this))
{
}

IOutputParser::~IOutputParser()
{
    clear();
    delete d;
}

void IOutputParser::handleStdout(const QString &data)
{
    d->stdoutState.handleData(data);
}

void IOutputParser::handleStderr(const QString &data)
{
    d->stderrState.handleData(data);
}

void IOutputParser::handleLine(const QString &line, Utils::OutputFormat type)
{
    const QString cleanLine = filteredLine(line);
    if (d->nextParser) {
        switch (d->nextParser->handleLine(cleanLine, outputTypeForParser(d->nextParser, type))) {
        case OutputTaskParser::Status::Done:
            d->nextParser = nullptr;
            return;
        case OutputTaskParser::Status::InProgress:
            return;
        case OutputTaskParser::Status::NotHandled:
            d->nextParser = nullptr;
            break;
        }
    }
    QTC_CHECK(!d->nextParser);
    for (OutputTaskParser * const lineParser : d->lineParsers) {
        switch (lineParser->handleLine(cleanLine, outputTypeForParser(lineParser, type))) {
        case OutputTaskParser::Status::Done:
            return;
        case OutputTaskParser::Status::InProgress:
            d->nextParser = lineParser;
            return;
        case OutputTaskParser::Status::NotHandled:
            break;
        }
    }
}

QString IOutputParser::filteredLine(const QString &line) const
{
    QString l = line;
    for (const IOutputParser::Filter &f : qAsConst(d->filters))
        l = f(l);
    return l;
}

void IOutputParser::setupLineParser(OutputTaskParser *parser)
{
    parser->setFileFinder(&d->fileFinder);
    connect(parser, &OutputTaskParser::addTask, this, &IOutputParser::addTask);
    connect(parser, &OutputTaskParser::newSearchDir, this, &IOutputParser::addSearchDir);
    connect(parser, &OutputTaskParser::searchDirExpired, this, &IOutputParser::dropSearchDir);
}

bool IOutputParser::hasFatalErrors() const
{
    return Utils::anyOf(d->lineParsers, [](const OutputTaskParser *p) {
        return p->hasFatalErrors();
    });
}

void IOutputParser::flush()
{
    d->stdoutState.flush();
    d->stderrState.flush();
    for (OutputTaskParser * const p : qAsConst(d->lineParsers))
        p->flush();
}

void IOutputParser::clear()
{
    d->nextParser = nullptr;
    d->filters.clear();
    qDeleteAll(d->lineParsers);
    d->lineParsers.clear();
    d->stdoutState.pendingData.clear();
    d->stderrState.pendingData.clear();
    d->fileFinder = Utils::FileInProjectFinder();
}

void IOutputParser::addLineParser(OutputTaskParser *parser)
{
    setupLineParser(parser);
    d->lineParsers << parser;
}

void IOutputParser::addLineParsers(const QList<OutputTaskParser *> &parsers)
{
    for (OutputTaskParser * const p : qAsConst(parsers))
        addLineParser(p);
}

void IOutputParser::setLineParsers(const QList<OutputTaskParser *> &parsers)
{
    qDeleteAll(d->lineParsers);
    d->lineParsers.clear();
    addLineParsers(parsers);
}

void IOutputParser::setFileFinder(const Utils::FileInProjectFinder &finder)
{
    d->fileFinder = finder;
}

#ifdef WITH_TESTS
QList<OutputTaskParser *> IOutputParser::lineParsers() const
{
    return d->lineParsers;
}
#endif // WITH_TESTS

void IOutputParser::addFilter(const Filter &filter)
{
    d->filters << filter;
}

void IOutputParser::addSearchDir(const Utils::FilePath &dir)
{
    for (OutputTaskParser * const p : qAsConst(d->lineParsers))
        p->addSearchDir(dir);
}

void IOutputParser::dropSearchDir(const Utils::FilePath &dir)
{
    for (OutputTaskParser * const p : qAsConst(d->lineParsers))
        p->dropSearchDir(dir);
}

Utils::OutputFormat IOutputParser::outputTypeForParser(const OutputTaskParser *parser,
                                                       Utils::OutputFormat type) const
{
    if (type == Utils::StdOutFormat && parser->needsRedirection())
        return Utils::StdErrFormat;
    return type;
}

} // namespace ProjectExplorer

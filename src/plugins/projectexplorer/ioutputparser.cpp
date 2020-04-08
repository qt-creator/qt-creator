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
#include <utils/synchronousprocess.h>

#include <QDir>
#include <QFileInfo>
#include <QPointer>

/*!
    \class ProjectExplorer::IOutputParser

    \brief The IOutputParser class provides an interface for an output parser
    that emits issues (tasks).

    \sa ProjectExplorer::Task
*/

/*!
   \fn ProjectExplorer::IOutputParser::Status ProjectExplorer::IOutputParser::doHandleLine(const QString &line, Utils::OutputFormat type)

   Called once for each line of standard output or standard error to parse.
*/

/*!
   \fn bool ProjectExplorer::IOutputParser::hasFatalErrors() const

   This is mainly a Symbian specific quirk.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::addTask(const ProjectExplorer::Task &task)

   Should be emitted for each task seen in the output.
*/

/*!
   \fn void ProjectExplorer::IOutputParser::doFlush()

   Instructs a parser to flush its state.
   Parsers may have state (for example, because they need to aggregate several
   lines into one task). This
   function is called when this state needs to be flushed out to be visible.

   doFlush() is called by flush(). flush() is called on child parsers
   whenever a new task is added.
   It is also called once when all input has been parsed.
*/

namespace ProjectExplorer {

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

    QList<IOutputParser *> lineParsers;
    IOutputParser *nextParser = nullptr;
    QList<Filter> filters;
    Utils::FilePaths searchDirs;
    OutputChannelState stdoutState;
    OutputChannelState stderrState;
    QPointer<const IOutputParser> redirectionDetector;
    bool skipFileExistsCheck = false;
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

IOutputParser::Status IOutputParser::doHandleLine(const QString &line, Utils::OutputFormat type)
{
    Q_UNUSED(line);
    Q_UNUSED(type);
    return Status::NotHandled;
}

void IOutputParser::doFlush() { }

void IOutputParser::handleLine(const QString &line, Utils::OutputFormat type)
{
    const QString cleanLine = filteredLine(line);
    if (d->nextParser) {
        switch (d->nextParser->doHandleLine(cleanLine, outputTypeForParser(d->nextParser, type))) {
        case Status::Done:
            d->nextParser = nullptr;
            return;
        case Status::InProgress:
            return;
        case Status::NotHandled:
            d->nextParser = nullptr;
            break;
        }
    }
    QTC_CHECK(!d->nextParser);
    for (IOutputParser * const lineParser : d->lineParsers) {
        switch (lineParser->doHandleLine(cleanLine, outputTypeForParser(lineParser, type))) {
        case Status::Done:
            return;
        case Status::InProgress:
            d->nextParser = lineParser;
            return;
        case Status::NotHandled:
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

void IOutputParser::connectLineParser(IOutputParser *parser)
{
    connect(parser, &IOutputParser::addTask, this, &IOutputParser::addTask);
    connect(parser, &IOutputParser::searchDirIn, this, &IOutputParser::addSearchDir);
    connect(parser, &IOutputParser::searchDirOut, this, &IOutputParser::dropSearchDir);
}

bool IOutputParser::hasFatalErrors() const
{
    return Utils::anyOf(d->lineParsers, [](const IOutputParser *p) { return p->hasFatalErrors(); });
}

void IOutputParser::flush()
{
    d->stdoutState.flush();
    d->stderrState.flush();
    doFlush();
    for (IOutputParser * const p : qAsConst(d->lineParsers))
        p->doFlush();
}

void IOutputParser::clear()
{
    d->nextParser = nullptr;
    d->redirectionDetector.clear();
    d->filters.clear();
    d->searchDirs.clear();
    qDeleteAll(d->lineParsers);
    d->lineParsers.clear();
    d->stdoutState.pendingData.clear();
    d->stderrState.pendingData.clear();
}

void IOutputParser::addLineParser(IOutputParser *parser)
{
    connectLineParser(parser);
    d->lineParsers << parser;
}

void IOutputParser::addLineParsers(const QList<IOutputParser *> &parsers)
{
    for (IOutputParser * const p : qAsConst(parsers))
        addLineParser(p);
}

void IOutputParser::setLineParsers(const QList<IOutputParser *> &parsers)
{
    qDeleteAll(d->lineParsers);
    d->lineParsers.clear();
    addLineParsers(parsers);
}

#ifdef WITH_TESTS
QList<IOutputParser *> IOutputParser::lineParsers() const
{
    return d->lineParsers;
}

void IOutputParser::skipFileExistsCheck()
{
    d->skipFileExistsCheck = true;
}
#endif // WITH_TESTS

QString IOutputParser::rightTrimmed(const QString &in)
{
    int pos = in.length();
    for (; pos > 0; --pos) {
        if (!in.at(pos - 1).isSpace())
            break;
    }
    return in.mid(0, pos);
}

void IOutputParser::addFilter(const Filter &filter)
{
    d->filters << filter;
}

void IOutputParser::addSearchDir(const Utils::FilePath &dir)
{
    d->searchDirs << dir;
    for (IOutputParser * const p : qAsConst(d->lineParsers))
        p->addSearchDir(dir);
}

void IOutputParser::dropSearchDir(const Utils::FilePath &dir)
{
    const int idx = d->searchDirs.lastIndexOf(dir);
    QTC_ASSERT(idx != -1, return);
    d->searchDirs.removeAt(idx);
    for (IOutputParser * const p : qAsConst(d->lineParsers))
        p->dropSearchDir(dir);
}

const Utils::FilePaths IOutputParser::searchDirectories() const
{
    return d->searchDirs;
}

Utils::FilePath IOutputParser::absoluteFilePath(const Utils::FilePath &filePath)
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
    return filePath;
}

// The redirection mechanism is needed for broken build tools (e.g. xcodebuild) that get invoked
// indirectly as part of the build process and redirect their child processes' stderr output
// to stdout. A parser might be able to detect this condition and inform interested
// other parsers that they need to interpret stdout data as stderr.
void IOutputParser::setRedirectionDetector(const IOutputParser *detector)
{
    d->redirectionDetector = detector;
}

bool IOutputParser::needsRedirection() const
{
    return d->redirectionDetector && (d->redirectionDetector->hasDetectedRedirection()
                                      || d->redirectionDetector->needsRedirection());
}

Utils::OutputFormat IOutputParser::outputTypeForParser(const IOutputParser *parser,
                                                       Utils::OutputFormat type) const
{
    if (type == Utils::StdOutFormat && parser->needsRedirection())
        return Utils::StdErrFormat;
    return type;
}

} // namespace ProjectExplorer

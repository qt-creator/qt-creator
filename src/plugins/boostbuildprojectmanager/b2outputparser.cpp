//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the  GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#include "b2outputparser.h"
#include "b2utility.h"
#include "external/projectexplorer/gccparser.h"
#include "external/projectexplorer/ldparser.h"
#include "external/projectexplorer/clangparser.h"
// Qt Creator
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>
#include <utils/qtcassert.h>
// Qt
#include <QRegExp>
#include <QString>
#include <QStringRef>

namespace BoostBuildProjectManager {
namespace Internal {

namespace {
char const* const RxToolsetFromCommand = "^([\\w-]+)(?:\\.)([\\w-]+).+$";
char const* const RxToolsetFromWarning = "^warning\\:.+toolset.+\\\"([\\w-]+)\\\".*$";
// TODO: replace filename with full path? ^\*\*passed\*\*\s(.+\.test)
char const* const RxTestPassed = "^\\*\\*passed\\*\\*\\s+(.+\\.test)\\s*$";
char const* const RxTestFailed = "^\\.\\.\\.failed\\s+testing\\.capture-output\\s+(.+\\.run)\\.\\.\\.$";
char const* const RxTestFailedAsExpected = "^\\(failed-as-expected\\)\\s+(.+\\.o)\\s*$";
char const* const RxTestFileLineN = "(\\.[ch][px]*)\\(([0-9]+)\\)"; // replace \\1:\\2
char const* const RxTestFileObj = "\\W(\\w+)(\\.o)";
}

BoostBuildParser::BoostBuildParser()
    : rxToolsetNameCommand_(QLatin1String(RxToolsetFromCommand))
    , rxToolsetNameWarning_(QLatin1String(RxToolsetFromWarning))
    , rxTestPassed_(QLatin1String(RxTestPassed))
    , rxTestFailed_(QLatin1String(RxTestFailed))
    , rxTestFailedAsExpected_(QLatin1String(RxTestFailedAsExpected))
    , rxTestFileLineN_(QLatin1String(RxTestFileLineN))
    , rxTestFileObj_(QLatin1String(RxTestFileObj))
    , lineMode_(Common)
{
    rxToolsetNameCommand_.setMinimal(true);
    QTC_CHECK(rxToolsetNameCommand_.isValid());

    rxToolsetNameWarning_.setMinimal(true);
    QTC_CHECK(rxToolsetNameWarning_.isValid());

    rxTestPassed_.setMinimal(true);
    QTC_CHECK(rxTestPassed_.isValid());

    rxTestFailed_.setMinimal(true);
    QTC_CHECK(rxTestFailed_.isValid());

    rxTestFailedAsExpected_.setMinimal(true);
    QTC_CHECK(rxTestFailedAsExpected_.isValid());

    rxTestFileLineN_.setMinimal(true);
    QTC_CHECK(rxTestFileLineN_.isValid());
}

QString BoostBuildParser::findToolset(QString const& line) const
{
    QString name;
    if (rxToolsetNameWarning_.indexIn(line) > -1)
        name = rxToolsetNameWarning_.cap(1);
    else if (rxToolsetNameCommand_.indexIn(line) > -1)
        name = rxToolsetNameCommand_.cap(1);
    return name;
}

void BoostBuildParser::setToolsetParser(QString const& toolsetName)
{
    if (!toolsetName.isEmpty() && toolsetName_.isEmpty()) {
        if (QStringRef(&toolsetName, 0, 3) == QLatin1String("gcc")) {
            appendOutputParser(new ProjectExplorer::GccParser());
            toolsetName_ = toolsetName;
        } else if (QStringRef(&toolsetName, 0, 5) == QLatin1String("clang")) {
            // clang-xxx found (e.g. clang-linux)
            appendOutputParser(new ProjectExplorer::ClangParser());
            toolsetName_ = toolsetName;
        } else {
            // TODO: Add more toolsets (Intel, VC++, etc.)
        }
    }
}

void BoostBuildParser::stdOutput(QString const& rawLine)
{
    setToolsetParser(findToolset(rawLine));

    QString const line
        = rightTrimmed(rawLine).replace(rxTestFileLineN_, QLatin1String("\\1:\\2"));
    if (!toolsetName_.isEmpty() && line.startsWith(toolsetName_))
        lineMode_ = Toolset;
    else if (line.startsWith(QLatin1String("testing"))
            || line.startsWith(QLatin1String("(failed-as-expected)")))
        lineMode_ = Testing;
    else if (line.startsWith(QLatin1String("common")))
        lineMode_ = Common;

    // TODO: Why forwarding stdOutput to ProjectExplorer::IOutputParser::stdError?
    // Because of a bug (or feature?) in Boost.Build:
    // stdout and stderr not forwarded to respective channels
    // https://svn.boost.org/trac/boost/ticket/9485

    if (lineMode_ == Toolset) {
        ProjectExplorer::IOutputParser::stdError(line);
    } else if (lineMode_ == Testing) {
        if (rxTestPassed_.indexIn(line) > -1) {
            BBPM_QDEBUG(rxTestPassed_.capturedTexts());
            // TODO: issue #3
            ProjectExplorer::Task task(ProjectExplorer::Task::Unknown
                , rxTestPassed_.cap(0)
                , Utils::FileName::fromString(rxTestPassed_.cap(1))
                , -1 // line
                , ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
            setTask(task);
            lineMode_ = Common;
        } else if (rxTestFailed_.indexIn(line) > -1) {
            BBPM_QDEBUG(rxTestFailed_.capturedTexts());

            // Report summary task for "...failed testing.capture-output /myfile.run"
            ProjectExplorer::Task task(ProjectExplorer::Task::Error
                , rxTestFailed_.cap(0)
                , Utils::FileName::fromString(rxTestFailed_.cap(1))
                , -1 // line
                , ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
            setTask(task);

            lineMode_ = Common;
        } else if (rxTestFailedAsExpected_.indexIn(line) > -1) {
            BBPM_QDEBUG(rxTestFailedAsExpected_.capturedTexts());

            // TODO: Handling of "(failed-as-expected)" is not great, might be confusing.
            // Boost.Build spits out compile command first, so compilation errors arrive
            // and are parsed immediately (issue tasks are created)
            // due to lineMode_==Toolset.
            // Then, "(failed-as-expected)" status arrives and there seem to be no way to
            // look back and clear all the issue tasks created for compilation errors.
            // TODO: Ask Volodya if b2 could announce "testing." before compile command
            // for a compile-time test.

            QString fileName(rxTestFailedAsExpected_.cap(1));
            if (rxTestFileObj_.indexIn(fileName))
                fileName = rxTestFileObj_.cap(1) + QLatin1String(".cpp");// FIXME:hardcoded ext

            // ATM, we can only indicate in UI that test failed-as-expected
            ProjectExplorer::Task task(ProjectExplorer::Task::Error
                , rxTestFailedAsExpected_.cap(0)
                , Utils::FileName::fromString(fileName)
                , -1 // line
                , ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
            setTask(task);

            lineMode_ = Common;
        } else {
            // Parses compilation errors of run-time tests, creates issue tasks
            ProjectExplorer::IOutputParser::stdError(line);
        }
    } else {
        doFlush();
        ProjectExplorer::IOutputParser::stdOutput(line);
    }
}

void BoostBuildParser::stdError(QString const& line)
{
    setToolsetParser(findToolset(line));
    ProjectExplorer::IOutputParser::stdError(line);
}

void BoostBuildParser::doFlush()
{
    if (!lastTask_.isNull()) {
        ProjectExplorer::Task t = lastTask_;
        lastTask_.clear();
        emit addTask(t);
    }
}

void BoostBuildParser::setTask(ProjectExplorer::Task const& task)
{
    doFlush();
    lastTask_ = task;
}

} // namespace Internal
} // namespace BoostBuildProjectManager

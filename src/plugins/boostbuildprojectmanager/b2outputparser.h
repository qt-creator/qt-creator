//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#ifndef BBOUTPUTPARSER_HPP
#define BBOUTPUTPARSER_HPP

// Qt Creator
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/task.h>
// Qt
#include <QPointer>
#include <QString>

namespace BoostBuildProjectManager {
namespace Internal {

class BoostBuildParser : public ProjectExplorer::IOutputParser
{
    Q_OBJECT

public:
    BoostBuildParser();

    void stdOutput(QString const& line);
    void stdError(QString const& line);

protected:
    void doFlush();

private:

    QString findToolset(QString const& line) const;
    void setToolsetParser(QString const& toolsetName);
    void setTask(ProjectExplorer::Task const& task);

    QRegExp rxToolsetNameCommand_; // ".compile." command line
    QRegExp rxToolsetNameWarning_; // "warning: " status line
    QRegExp rxTestPassed_; // "**passed**" status line
    QRegExp rxTestFailed_; // "...failed testing" status line
    QRegExp rxTestFailedAsExpected_; // "(failed-as-expected)" status line
    QRegExp rxTestFileLineN_; // file.cpp(XX) => file.cpp:XX
    QRegExp rxTestFileObj_; // file.o => file.cpp
    QString toolsetName_;

    // Boost.Build command mode relates to first command token in line.
    enum LineMode { Common, Toolset, Testing };
    LineMode lineMode_;

    ProjectExplorer::Task lastTask_;
    QPointer<ProjectExplorer::IOutputParser> parser_;
};

} // namespace Internal
} // namespace BoostBuildProjectManager

#endif // BBOUTPUTPARSER_HPP

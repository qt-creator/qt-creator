// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#if defined(WITH_TESTS)

#include "projectexplorer_export.h"

#include "ioutputparser.h"
#include "task.h"

namespace ProjectExplorer {

class TestTerminator;

class PROJECTEXPLORER_EXPORT OutputParserTester : public Utils::OutputFormatter
{
    Q_OBJECT

public:
    enum Channel {
        STDOUT,
        STDERR
    };

    OutputParserTester();
    ~OutputParserTester();

    // test functions:
    void testParsing(const QString &lines, Channel inputChannel,
                     Tasks tasks,
                     const QString &childStdOutLines,
                     const QString &childStdErrLines,
                     const QString &outputLines);

    void setDebugEnabled(bool);

signals:
    void aboutToDeleteParser();

private:
    void reset();

    bool m_debug = false;

    QString m_receivedStdErrChildLine;
    QString m_receivedStdOutChildLine;
    Tasks m_receivedTasks;
    QString m_receivedOutput;

    friend class TestTerminator;
};

class TestTerminator : public OutputTaskParser
{
    Q_OBJECT

public:
    explicit TestTerminator(OutputParserTester *t);

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;

    OutputParserTester *m_tester = nullptr;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::OutputParserTester::Channel)

#endif

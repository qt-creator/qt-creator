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
    TestTerminator(OutputParserTester *t);

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;

    OutputParserTester *m_tester = nullptr;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::OutputParserTester::Channel)

#endif

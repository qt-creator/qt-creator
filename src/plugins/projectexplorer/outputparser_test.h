/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef OUTPUTPARSER_TESTER_H
#define OUTPUTPARSER_TESTER_H

#if defined(WITH_TESTS)

#include "projectexplorer_export.h"
#include "metatypedeclarations.h"
#include "ioutputparser.h"

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT OutputParserTester : public IOutputParser
{
    Q_OBJECT

public:
    enum Channel {
        STDOUT,
        STDERR
    };

    OutputParserTester();
    ~OutputParserTester();

    // test methods:
    void testParsing(const QString &lines, Channel inputChannel,
                     QList<Task> tasks,
                     const QString &childStdOutLines,
                     const QString &childStdErrLines,
                     const QString &outputLines);
    void testTaskMangling(const Task input,
                          const Task output);
    void testOutputMangling(const QString &input,
                            const QString &output);

    void setDebugEnabled(bool);

    // IOutputParser interface:
    void stdOutput(const QString &line);
    void stdError(const QString &line);

    void appendOutputParser(IOutputParser *parser);

signals:
    void aboutToDeleteParser();

private slots:
    void outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format);
    void taskAdded(const ProjectExplorer::Task &task);

private:
    void reset();

    bool m_debug;

    QString m_receivedStdErrChildLine;
    QString m_receivedStdOutChildLine;
    QList<Task> m_receivedTasks;
    QString m_receivedOutput;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::OutputParserTester::Channel)

#endif

#endif // OUTPUTPARSER_TESTER_H

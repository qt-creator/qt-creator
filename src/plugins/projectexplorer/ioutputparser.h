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

#include "projectexplorer_export.h"
#include "buildstep.h"

#include <QString>

namespace ProjectExplorer {
class Task;

// Documentation inside.
class PROJECTEXPLORER_EXPORT IOutputParser : public QObject
{
    Q_OBJECT
public:
    IOutputParser() = default;
    ~IOutputParser() override;

    virtual void appendOutputParser(IOutputParser *parser);

    IOutputParser *takeOutputParserChain();

    IOutputParser *childParser() const;
    void setChildParser(IOutputParser *parser);

    virtual void stdOutput(const QString &line);
    virtual void stdError(const QString &line);

    virtual bool hasFatalErrors() const;
    // For GnuMakeParser
    virtual void setWorkingDirectory(const QString &workingDirectory);

    void flush(); // flush out pending tasks

    static QString rightTrimmed(const QString &in);

signals:
    void addOutput(const QString &string, ProjectExplorer::BuildStep::OutputFormat format);
    void addTask(const ProjectExplorer::Task &task, int linkedOutputLines = 0, int skipLines = 0);

public slots:
    virtual void outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format);
    virtual void taskAdded(const ProjectExplorer::Task &task, int linkedOutputLines = 0, int skipLines = 0);

private:
    virtual void doFlush();

    IOutputParser *m_parser = nullptr;
};

} // namespace ProjectExplorer

/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef IOUTPUTPARSER_H
#define IOUTPUTPARSER_H

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
    IOutputParser();
    virtual ~IOutputParser();

    virtual void appendOutputParser(IOutputParser *parser);

    IOutputParser *takeOutputParserChain();

    IOutputParser *childParser() const;
    void setChildParser(IOutputParser *parser);

    virtual void stdOutput(const QString &line);
    virtual void stdError(const QString &line);

    virtual bool hasFatalErrors() const;
    // For GnuMakeParser
    virtual void setWorkingDirectory(const QString &workingDirectory);

signals:
    void addOutput(const QString &string, ProjectExplorer::BuildStep::OutputFormat format);
    void addTask(const ProjectExplorer::Task &task);

public slots:
    virtual void outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format);
    virtual void taskAdded(const ProjectExplorer::Task &task);

private:
    IOutputParser *m_parser;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::IOutputParser*)

#endif // IOUTPUTPARSER_H

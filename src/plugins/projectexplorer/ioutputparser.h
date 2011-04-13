/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef IOUTPUTPARSER_H
#define IOUTPUTPARSER_H

#include "projectexplorer_export.h"
#include "buildstep.h"

#include <QtCore/QString>

namespace ProjectExplorer {
class Task;

class PROJECTEXPLORER_EXPORT IOutputParser : public QObject
{
    Q_OBJECT
public:
    IOutputParser();
    virtual ~IOutputParser();

    /// Append a subparser to this parser.
    /// IOutputParser will take ownership.
    virtual void appendOutputParser(IOutputParser *parser);

    /// Remove the appended outputparser chain frm this parser.
    /// This method transferes ownership of the parser chain to the caller!
    IOutputParser *takeOutputParserChain();

    /// Return the head of this parsers output parser children
    /// IOutputParser keeps ownership!
    IOutputParser *childParser() const;
    void setChildParser(IOutputParser *parser);

    /// Called once for each line if standard output to parse.
    virtual void stdOutput(const QString &line);
    /// Called once for each line if standard error to parse.
    virtual void stdError(const QString &line);

    // This is mainly a symbian specific quirk
    virtual bool hasFatalErrors() const;
    // For GnuMakeParser
    virtual void setWorkingDirectory(const QString &workingDirectory);

signals:
    /// Should be emitted whenever some additional information should be
    /// added to the output.
    /// Note: This is additional information. There is no need to add each
    /// line!
    void addOutput(const QString &string, ProjectExplorer::BuildStep::OutputFormat format);
    /// Should be emitted for each task seen in the output.
    void addTask(const ProjectExplorer::Task &task);

public slots:
    /// Subparsers have their addOutput signal connected to this slot.
    /// This method can be overwritten to change the string.
    virtual void outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format);
    /// Subparsers have their addTask signal connected to this slot.
    /// This method can be overwritten to change the task.
    virtual void taskAdded(const ProjectExplorer::Task &task);

private:
    IOutputParser *m_parser;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::IOutputParser*)

#endif // IOUTPUTPARSER_H

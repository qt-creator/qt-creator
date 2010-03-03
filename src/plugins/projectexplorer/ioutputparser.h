/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef IOUTPUTPARSER_H
#define IOUTPUTPARSER_H

#include "projectexplorer_export.h"
#include "taskwindow.h"

#include <QtCore/QObject>
#include <QtCore/QString>

namespace ProjectExplorer {

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

    /// Called once for each line if standard output to parse.
    virtual void stdOutput(const QString &line);
    /// Called once for each line if standard error to parse.
    virtual void stdError(const QString &line);

signals:
    /// Should be emitted whenever some additional information should be
    /// added to the output.
    /// Note: This is additional information. There is no need to add each
    /// line!
    void addOutput(const QString &string);
    /// Should be emitted for each task seen in the output.
    void addTask(const ProjectExplorer::TaskWindow::Task &task);

public slots:
    /// Subparsers have their addOutput signal connected to this slot.
    /// This method can be overwritten to change the string.
    virtual void outputAdded(const QString &string);
    /// Subparsers have their addTask signal connected to this slot.
    /// This method can be overwritten to change the task.
    virtual void taskAdded(const ProjectExplorer::TaskWindow::Task &task);

private:
    IOutputParser *m_parser;
};

} // namespace ProjectExplorer

#endif // IOUTPUTPARSER_H

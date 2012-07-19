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

#ifndef SBSV2PARSER_H
#define SBSV2PARSER_H

#include <projectexplorer/ioutputparser.h>

#include <QDir>
#include <QXmlStreamReader>

namespace ProjectExplorer {
class TaskHub;
}

namespace Qt4ProjectManager {

class SbsV2Parser : public ProjectExplorer::IOutputParser
{
    Q_OBJECT

public:
    SbsV2Parser();

    virtual void stdOutput(const QString & line);
    virtual void stdError(const QString & line);

public slots:
    virtual void taskAdded(const ProjectExplorer::Task &task);

private:
    void parseLogFile(const QString &file);
    void readBuildLog();
    void readError();
    void readWarning();
    void readRecipe();

    QXmlStreamReader m_log;
    QDir m_currentSource;
    QDir m_currentTarget;
    ProjectExplorer::TaskHub *m_hub;
};

} // namespace Qt4ProjectExplorer

#endif // SBSV2PARSER_H

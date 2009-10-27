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

#ifndef ABSTRACTMAKESTEP_H
#define ABSTRACTMAKESTEP_H

#include "projectexplorer_export.h"
#include "abstractprocessstep.h"
#include "taskwindow.h"

namespace ProjectExplorer {
class BuildStep;
class IBuildStepFactory;
class Project;
}

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT AbstractMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    AbstractMakeStep(Project * project, BuildConfiguration *bc);
    AbstractMakeStep(AbstractMakeStep *bs, BuildConfiguration *bc);
    ~AbstractMakeStep();
    virtual bool init();
    virtual void run(QFutureInterface<bool> &);

protected:
    // derived classes needs to call these functions
    virtual void stdOut(const QString &line);
    virtual void stdError(const QString &line);

    // derived classes needs to call this function
    void setBuildParser(const QString &parser);
    QString buildParser() const;
private slots:
    void slotAddToTaskWindow(const ProjectExplorer::TaskWindow::Task &task);
    void addDirectory(const QString &dir);
    void removeDirectory(const QString &dir);
private:
    QString m_buildParserName;
    ProjectExplorer::IBuildParser *m_buildParser;
    QString m_buildConfiguration;
    QSet<QString> m_openDirectories;
};

} // ProjectExplorer

#endif // ABSTRACTMAKESTEP_H

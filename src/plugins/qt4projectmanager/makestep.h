/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef MAKESTEP_H
#define MAKESTEP_H

#include "qtversionmanager.h"
#include "ui_makestep.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/projectexplorer.h>

namespace Qt4ProjectManager {

class Qt4Project;

// NBS move this class to an own plugin? So that there can be a make project at a future time
class MakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    MakeStep(Qt4Project * project);
    ~MakeStep();
    virtual bool init(const QString & name);
    virtual void run(QFutureInterface<bool> &);
    virtual QString name();
    virtual QString displayName();
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;
protected:
    virtual void stdOut(const QString &line);
    virtual void stdError(const QString &line);
private slots:
    void slotAddToTaskWindow(const QString & fn, int type, int linenumber, const QString & description);
    void addDirectory(const QString &dir);
    void removeDirectory(const QString &dir);
private:
    ProjectExplorer::BuildParserInterface *buildParser(const Internal::QtVersion * const version);
    Qt4Project *m_project;
    ProjectExplorer::BuildParserInterface *m_buildParser;
    bool m_skipMakeClean;
    QString m_buildConfiguration;
    QSet<QString> m_openDirectories;
};

class MakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    MakeStepConfigWidget(MakeStep *makeStep);
    QString displayName() const;
    void init(const QString &buildConfiguration);
private slots:
    void makeLineEditTextEdited();
    void makeArgumentsLineEditTextEdited();
private:
    QString m_buildConfiguration;
    Ui::MakeStep m_ui;
    MakeStep *m_makeStep;
};

} // Qt4ProjectManager

#endif // MAKESTEP_H

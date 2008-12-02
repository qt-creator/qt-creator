/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#ifndef MAKESTEP_H
#define MAKESTEP_H

#include <projectexplorer/ProjectExplorerInterfaces>
#include <QDebug>

#include "qtversionmanager.h"

#include "ui_makestep.h"

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

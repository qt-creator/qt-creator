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
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#ifndef QMAKESTEP_H
#define QMAKESTEP_H

#include <projectexplorer/ProjectExplorerInterfaces>
#include <QStringList>

#include "ui_qmakestep.h"

namespace Qt4ProjectManager {

class Qt4Project;

class QMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    QMakeStep(Qt4Project * project);
    ~QMakeStep();
    virtual bool init(const QString & name);
    virtual void run(QFutureInterface<bool> &);
    virtual QString name();
    virtual QString displayName();
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;
    QStringList arguments(const QString &buildConfiguration);
    void setForced(bool b);
    bool forced();
protected:
    virtual void processStartupFailed();
    virtual bool processFinished(int exitCode, QProcess::ExitStatus status);

private:
    Qt4Project *m_pro;
    // last values
    QString m_buildConfiguration;
    QStringList m_lastEnv;
    QString m_lastWorkingDirectory;
    QStringList m_lastArguments;
    QString m_lastProgram;
    bool m_forced;
};


class QMakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    QMakeStepConfigWidget(QMakeStep *step);
    QString displayName() const;
    void init(const QString &buildConfiguration);
private slots:
    void qmakeArgumentsLineEditTextEdited();
    void buildConfigurationChanged();
private:
    QString m_buildConfiguration;
    Ui::QMakeStep m_ui;
    QMakeStep *m_step;
};

} // namespace Qt4ProjectManager

#endif // QMAKESTEP_H

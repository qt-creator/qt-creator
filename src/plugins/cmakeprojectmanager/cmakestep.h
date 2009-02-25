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

#ifndef CMAKESTEP_H
#define CMAKESTEP_H

#include <projectexplorer/buildstep.h>
#include <projectexplorer/abstractprocessstep.h>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace CMakeProjectManager {
namespace Internal {

class CMakeProject;

class CMakeBuildStepConfigWidget;

class CMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    CMakeStep(CMakeProject *pro);
    ~CMakeStep();
    virtual bool init(const QString &buildConfiguration);

    virtual void run(QFutureInterface<bool> &fi);

    virtual QString name();
    virtual QString displayName();
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;

    void setUserArguments(const QString &buildConfiguration, const QStringList &arguments);
    QStringList userArguments(const QString &buildConfiguration) const;
private:
    CMakeProject *m_pro;
};

class CMakeBuildStepConfigWidget :public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    CMakeBuildStepConfigWidget(CMakeStep *cmakeStep);
    virtual QString displayName() const;
    virtual void init(const QString &buildConfiguration);
private slots:
    void argumentsLineEditChanged();
private:
    CMakeStep *m_cmakeStep;
    QString m_buildConfiguration;
    QLineEdit *m_arguments;
};

class CMakeBuildStepFactory : public ProjectExplorer::IBuildStepFactory
{
    virtual bool canCreate(const QString &name) const;
    virtual ProjectExplorer::BuildStep *create(ProjectExplorer::Project *pro, const QString &name) const;
    virtual QStringList canCreateForProject(ProjectExplorer::Project *pro) const;
    virtual QString displayNameForName(const QString &name) const;
};


}
}
#endif // CMAKESTEP_H

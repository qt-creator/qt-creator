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

#ifndef GDBMACROSBUILDSTEP_H
#define GDBMACROSBUILDSTEP_H

#include <projectexplorer/buildstep.h>

namespace Qt4ProjectManager {
class Qt4Project;
namespace Internal {
class GdbMacrosBuildStepConfigWidget;

class GdbMacrosBuildStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
public:
    GdbMacrosBuildStep(Qt4Project * project);
    virtual ~GdbMacrosBuildStep();
    virtual bool init(const QString &buildConfiguration);
    virtual void run(QFutureInterface<bool> &);
    virtual QString name();
    virtual QString displayName();
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;
private:
    Qt4Project *m_project;
    QString m_buildDirectory;
    QString m_qmake;
    QString m_buildConfiguration;
};

class GdbMacrosBuildStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT
public:
    virtual bool canCreate(const QString &name) const;
    virtual ProjectExplorer::BuildStep *create(ProjectExplorer::Project *pro, const QString &name) const;
    virtual QStringList canCreateForProject(ProjectExplorer::Project *pro) const;
    virtual QString displayNameForName(const QString &name) const;

};

class GdbMacrosBuildStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    virtual QString displayName() const;
    virtual void init(const QString &buildConfiguration);
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // GDBMACROSBUILDSTEP_H

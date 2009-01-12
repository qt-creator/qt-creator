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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef CMAKERUNCONFIGURATION_H
#define CMAKERUNCONFIGURATION_H

#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/environment.h>
#include <projectexplorer/persistentsettings.h>

namespace CMakeProjectManager {
namespace Internal {

class CMakeProject;

class CMakeRunConfiguration : public ProjectExplorer::ApplicationRunConfiguration
{
public:
    CMakeRunConfiguration(CMakeProject *pro, const QString &target, const QString &workingDirectory);
    virtual ~CMakeRunConfiguration();
    virtual QString type() const;
    virtual QString executable() const;
    virtual RunMode runMode() const;
    virtual QString workingDirectory() const;
    virtual QStringList commandLineArguments() const;
    virtual ProjectExplorer::Environment environment() const;
    virtual QWidget *configurationWidget();

    virtual void save(ProjectExplorer::PersistentSettingsWriter &writer) const;
    virtual void restore(const ProjectExplorer::PersistentSettingsReader &reader);
private:
    QString m_target;
    QString m_workingDirectory;
};

/* The run configuration factory is used for restoring run configurations from
 * settings. And used to create new runconfigurations in the "Run Settings" Dialog.
 * For the first case bool canCreate(const QString &type) and
 * QSharedPointer<RunConfiguration> create(Project *project, QString type) are used.
 * For the second type the functions QStringList canCreate(Project *pro) and
 * QString nameForType(const QString&) are used to generate a list of creatable
 * RunConfigurations, and create(..) is used to create it.
 */
class CMakeRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT;
public:
    CMakeRunConfigurationFactory();
    virtual ~CMakeRunConfigurationFactory();
    // used to recreate the runConfigurations when restoring settings
    virtual bool canCreate(const QString &type) const;
    // used to show the list of possible additons to a project, returns a list of types
    virtual QStringList canCreate(ProjectExplorer::Project *pro) const;
    // used to translate the types to names to display to the user
    virtual QString nameForType(const QString &type) const;
    virtual QSharedPointer<ProjectExplorer::RunConfiguration> create(ProjectExplorer::Project *project, const QString &type);
};


}
}

#endif // CMAKERUNCONFIGURATION_H

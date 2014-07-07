/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef WINRTRUNCONFIGURATIONFACTORY_H
#define WINRTRUNCONFIGURATIONFACTORY_H

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/devicesupport/idevicefactory.h>
#include <projectexplorer/deployconfiguration.h>

namespace WinRt {
namespace Internal {

class WinRtRunConfigurationFactory  : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT
public:
    WinRtRunConfigurationFactory();

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const;
    QString displayNameForId(Core::Id id) const;
    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *product) const;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent,
                                             ProjectExplorer::RunConfiguration *product);

private:
    virtual ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent, Core::Id id);
    virtual ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent, const QVariantMap &map);
};

class WinRtRunControlFactory : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT
public:
    WinRtRunControlFactory();
    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration,
                ProjectExplorer::RunMode mode) const;
    ProjectExplorer::RunControl *create(ProjectExplorer::RunConfiguration *runConfiguration,
                       ProjectExplorer::RunMode mode, QString *errorMessage);
    QString displayName() const;
};

} // namespace Internal
} // namespace WinRt

#endif // WINRTRUNCONFIGURATIONFACTORY_H

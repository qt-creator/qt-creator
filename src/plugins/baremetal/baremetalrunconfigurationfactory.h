/****************************************************************************
**
** Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef BAREMETALRUNCONFIGURATIONFACTORY_H
#define BAREMETALRUNCONFIGURATIONFACTORY_H

#include <projectexplorer/runconfiguration.h>

namespace BareMetal {
namespace Internal {

class BareMetalRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit BareMetalRunConfigurationFactory(QObject *parent = 0);
    ~BareMetalRunConfigurationFactory();

    QString displayNameForId(Core::Id id) const;
    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const;

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const;

    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;

    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent,
                                             ProjectExplorer::RunConfiguration *source);
private:
    bool canHandle(const ProjectExplorer::Target *target) const;

    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent, Core::Id id);
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map);
};

} // namespace Internal
} // namespace RemoteLinux

#endif // BAREMETALRUNCONFIGURATIONFACTORY_H

/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MAEMORUNFACTORIES_H
#define MAEMORUNFACTORIES_H

#include <projectexplorer/runconfiguration.h>

namespace ProjectExplorer {
    class RunConfiguration;
    class RunControl;
    class Target;
}
using ProjectExplorer::IRunConfigurationFactory;
using ProjectExplorer::IRunControlFactory;
using ProjectExplorer::RunConfiguration;
using ProjectExplorer::RunConfigWidget;
using ProjectExplorer::RunControl;
using ProjectExplorer::Target;

namespace Madde {
namespace Internal {

class MaemoRunConfigurationFactory : public IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit MaemoRunConfigurationFactory(QObject *parent = 0);
    ~MaemoRunConfigurationFactory();

    QString displayNameForId(const QString &id) const;
    QStringList availableCreationIds(Target *parent) const;

    bool canCreate(Target *parent, const QString &id) const;
    RunConfiguration *create(Target *parent, const QString &id);

    bool canRestore(Target *parent, const QVariantMap &map) const;
    RunConfiguration *restore(Target *parent, const QVariantMap &map);

    bool canClone(Target *parent, RunConfiguration *source) const;
    RunConfiguration *clone(Target *parent, RunConfiguration *source);
};

class MaemoRunControlFactory : public IRunControlFactory
{
    Q_OBJECT
public:
    explicit MaemoRunControlFactory(QObject *parent = 0);
    ~MaemoRunControlFactory();

    QString displayName() const;
    RunConfigWidget *createConfigurationWidget(RunConfiguration *runConfiguration);

    bool canRun(RunConfiguration *runConfiguration, const QString &mode) const;
    RunControl *create(RunConfiguration *runConfiguration, const QString &mode);
};

    } // namespace Internal
} // namespace Madde

#endif  // MAEMORUNFACTORIES_H

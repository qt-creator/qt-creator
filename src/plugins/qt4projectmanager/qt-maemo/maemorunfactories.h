/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef MAEMORUNFACTORIES_H
#define MAEMORUNFACTORIES_H

#include <projectexplorer/runconfiguration.h>

namespace Qt4ProjectManager {
    namespace Internal {

class MaemoRunConfiguration;
using namespace ProjectExplorer;

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

private slots:
    void projectAdded(ProjectExplorer::Project *project);
    void projectRemoved(ProjectExplorer::Project *project);

    void targetAdded(ProjectExplorer::Target *target);
    void targetRemoved(ProjectExplorer::Target *target);

    void currentProjectChanged(ProjectExplorer::Project *project);

private:
    void setupRunConfiguration(MaemoRunConfiguration *rc);
};

class MaemoRunControlFactory : public IRunControlFactory
{
    Q_OBJECT
public:
    explicit MaemoRunControlFactory(QObject *parent = 0);
    ~MaemoRunControlFactory();

    QString displayName() const;
    QWidget *configurationWidget(RunConfiguration *runConfiguration);

    bool canRun(RunConfiguration *runConfiguration, const QString &mode) const;
    RunControl* create(RunConfiguration *runConfiguration, const QString &mode);
};

    } // namespace Internal
} // namespace Qt4ProjectManager

#endif  // MAEMORUNFACTORIES_H

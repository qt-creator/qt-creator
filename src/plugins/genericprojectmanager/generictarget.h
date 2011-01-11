/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef GENERICTARGET_H
#define GENERICTARGET_H

#include <projectexplorer/target.h>

#include "genericbuildconfiguration.h"

#include <QtCore/QStringList>
#include <QtCore/QVariantMap>

namespace ProjectExplorer {
class IBuildConfigurationFactory;
} // namespace ProjectExplorer

namespace GenericProjectManager {

namespace Internal {

const char * const GENERIC_DESKTOP_TARGET_ID("GenericProjectManager.GenericTarget");

class GenericProject;
class GenericRunConfiguration;

class GenericTargetFactory;

class GenericTarget : public ProjectExplorer::Target
{
    Q_OBJECT
    friend class GenericTargetFactory;

public:
    explicit GenericTarget(GenericProject *parent);
    ~GenericTarget();

    ProjectExplorer::BuildConfigWidget *createConfigWidget();

    GenericProject *genericProject() const;

    GenericBuildConfigurationFactory *buildConfigurationFactory() const;
    ProjectExplorer::DeployConfigurationFactory *deployConfigurationFactory() const;
    GenericBuildConfiguration *activeBuildConfiguration() const;

protected:
    bool fromMap(const QVariantMap &map);

private:
    GenericBuildConfigurationFactory *m_buildConfigurationFactory;
    ProjectExplorer::DeployConfigurationFactory *m_deployConfigurationFactory;
};

class GenericTargetFactory : public ProjectExplorer::ITargetFactory
{
    Q_OBJECT

public:
    explicit GenericTargetFactory(QObject *parent = 0);
    ~GenericTargetFactory();

    bool supportsTargetId(const QString &id) const;

    QStringList availableCreationIds(ProjectExplorer::Project *parent) const;
    QString displayNameForId(const QString &id) const;

    bool canCreate(ProjectExplorer::Project *parent, const QString &id) const;
    GenericTarget *create(ProjectExplorer::Project *parent, const QString &id);
    bool canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const;
    GenericTarget *restore(ProjectExplorer::Project *parent, const QVariantMap &map);
};

} // namespace Internal

} // namespace GenericProjectManager

#endif // GENERICTARGET_H

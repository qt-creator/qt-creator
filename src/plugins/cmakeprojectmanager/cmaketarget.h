/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef CMAKETARGET_H
#define CMAKETARGET_H

#include "cmakebuildconfiguration.h"

#include <projectexplorer/target.h>

namespace CMakeProjectManager {

namespace Internal {

const char * const DEFAULT_CMAKE_TARGET_ID("CMakeProjectManager.DefaultCMakeTarget");

class CMakeBuildConfiguration;
class CMakeBuildConfigurationFactory;
class CMakeProject;
class CMakeTargetFactory;

class CMakeTarget : public ProjectExplorer::Target
{
    Q_OBJECT
    friend class CMakeTargetFactory;

public:
    CMakeTarget(CMakeProject *parent);
    ~CMakeTarget();

    ProjectExplorer::BuildConfigWidget *createConfigWidget();

    CMakeProject *cmakeProject() const;
    CMakeBuildConfiguration *activeBuildConfiguration() const;

    CMakeBuildConfigurationFactory *buildConfigurationFactory() const;

    QString defaultBuildDirectory() const;

protected:
    bool fromMap(const QVariantMap &map);

private slots:
    void updateRunConfigurations();

private:
    CMakeBuildConfigurationFactory *m_buildConfigurationFactory;
};

class CMakeTargetFactory : public ProjectExplorer::ITargetFactory
{
    Q_OBJECT

public:
    CMakeTargetFactory(QObject *parent = 0);
    ~CMakeTargetFactory();

    bool supportsTargetId(const QString &id) const;

    QStringList supportedTargetIds(ProjectExplorer::Project *parent) const;
    QString displayNameForId(const QString &id) const;

    bool canCreate(ProjectExplorer::Project *parent, const QString &id) const;
    CMakeTarget *create(ProjectExplorer::Project *parent, const QString &id);
    bool canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const;
    CMakeTarget *restore(ProjectExplorer::Project *parent, const QVariantMap &map);
};

} // namespace Internal

} // namespace CMakeProjectManager

#endif // CMAKETARGET_H

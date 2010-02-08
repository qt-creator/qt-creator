/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
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

    CMakeProject *cmakeProject() const;
    CMakeBuildConfiguration *activeBuildConfiguration() const;

    CMakeBuildConfigurationFactory *buildConfigurationFactory() const;

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

    QStringList availableCreationIds(ProjectExplorer::Project *parent) const;
    QString displayNameForId(const QString &id) const;

    bool canCreate(ProjectExplorer::Project *parent, const QString &id) const;
    CMakeTarget *create(ProjectExplorer::Project *parent, const QString &id);
    bool canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const;
    CMakeTarget *restore(ProjectExplorer::Project *parent, const QVariantMap &map);
};

} // namespace Internal

} // namespace CMakeProjectManager

#endif // CMAKETARGET_H

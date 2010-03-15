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

#ifndef GENERICBUILDCONFIGURATION_H
#define GENERICBUILDCONFIGURATION_H

#include <projectexplorer/buildconfiguration.h>

namespace GenericProjectManager {
namespace Internal {

class GenericTarget;
class GenericBuildConfigurationFactory;

class GenericBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class GenericBuildConfigurationFactory;

public:
    explicit GenericBuildConfiguration(GenericTarget *parent);
    virtual ~GenericBuildConfiguration();

    GenericTarget *genericTarget() const;

    virtual ProjectExplorer::Environment environment() const;
    virtual QString buildDirectory() const;

    QString rawBuildDirectory() const;
    void setBuildDirectory(const QString &buildDirectory);

    QVariantMap toMap() const;

protected:
    GenericBuildConfiguration(GenericTarget *parent, GenericBuildConfiguration *source);
    GenericBuildConfiguration(GenericTarget *parent, const QString &id);
    virtual bool fromMap(const QVariantMap &map);

private:
    QString m_buildDirectory;
};

class GenericBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit GenericBuildConfigurationFactory(QObject *parent = 0);
    virtual ~GenericBuildConfigurationFactory();

    QStringList availableCreationIds(ProjectExplorer::Target *parent) const;
    QString displayNameForId(const QString &id) const;

    bool canCreate(ProjectExplorer::Target *parent, const QString &id) const;
    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent, const QString &id);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const;
    ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
};

} // namespace GenericProjectManager
} // namespace Internal
#endif // GENERICBUILDCONFIGURATION_H

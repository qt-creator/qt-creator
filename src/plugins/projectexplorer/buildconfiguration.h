/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef BUILDCONFIGURATION_H
#define BUILDCONFIGURATION_H

#include "projectexplorer_export.h"
#include "environment.h"

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/QObject>

#include "buildstep.h"
#include "projectconfiguration.h"

namespace ProjectExplorer {

class Project;

class PROJECTEXPLORER_EXPORT BuildConfiguration : public ProjectConfiguration
{
    Q_OBJECT

public:
    // ctors are protected
    virtual ~BuildConfiguration();

    QList<BuildStep *> buildSteps() const;
    void insertBuildStep(int position, BuildStep *step);
    void removeBuildStep(int position);
    void moveBuildStepUp(int position);

    QList<BuildStep *> cleanSteps() const;
    void insertCleanStep(int position, BuildStep *step);
    void removeCleanStep(int position);
    void moveCleanStepUp(int position);

    virtual Environment environment() const = 0;
    virtual QString buildDirectory() const = 0;

    Project *project() const;

    virtual QVariantMap toMap() const;

signals:
    void environmentChanged();
    void buildDirectoryChanged();

protected:
    BuildConfiguration(Project *project, const QString &id);
    BuildConfiguration(Project *project, BuildConfiguration *source);

    virtual bool fromMap(const QVariantMap &map);

private:
    QList<BuildStep *> m_buildSteps;
    QList<BuildStep *> m_cleanSteps;
    Project *m_project;
};

class PROJECTEXPLORER_EXPORT IBuildConfigurationFactory :
    public QObject
{
    Q_OBJECT

public:
    explicit IBuildConfigurationFactory(QObject *parent = 0);
    virtual ~IBuildConfigurationFactory();

    // used to show the list of possible additons to a project, returns a list of types
    virtual QStringList availableCreationIds(Project *parent) const = 0;
    // used to translate the types to names to display to the user
    virtual QString displayNameForId(const QString &id) const = 0;

    virtual bool canCreate(Project *parent, const QString &id) const = 0;
    virtual BuildConfiguration *create(Project *parent, const QString &id) = 0;
    // used to recreate the runConfigurations when restoring settings
    virtual bool canRestore(Project *parent, const QVariantMap &map) const = 0;
    virtual BuildConfiguration *restore(Project *parent, const QVariantMap &map) = 0;
    virtual bool canClone(Project *parent, BuildConfiguration *product) const = 0;
    virtual BuildConfiguration *clone(Project *parent, BuildConfiguration *product) = 0;

signals:
    void availableCreationIdsChanged();
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::BuildConfiguration *);

#endif // BUILDCONFIGURATION_H

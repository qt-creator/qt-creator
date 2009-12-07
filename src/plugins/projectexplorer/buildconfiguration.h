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

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QVariant>

#include "buildstep.h"

namespace ProjectExplorer {

class Project;

class PROJECTEXPLORER_EXPORT BuildConfiguration : public QObject
{
    Q_OBJECT

public:
    // ctors are protected
    virtual ~BuildConfiguration();

    QString displayName() const;
    void setDisplayName(const QString &name);

    QMap<QString, QVariant> toMap() const;
    void setValuesFromMap(QMap<QString, QVariant> map);

    QList<BuildStep *> buildSteps() const;
    void insertBuildStep(int position, BuildStep *step);
    void removeBuildStep(int position);
    void moveBuildStepUp(int position);

    QList<BuildStep *> cleanSteps() const;
    void insertCleanStep(int position, BuildStep *step);
    void removeCleanStep(int position);
    void moveCleanStepUp(int position);

    Project *project() const;

    virtual Environment environment() const = 0;
    virtual QString buildDirectory() const = 0;

signals:
    void environmentChanged();
    void buildDirectoryChanged();
    void displayNameChanged();

protected:
    BuildConfiguration(Project *project);
    BuildConfiguration(BuildConfiguration *source);

    // TODO remove those
    QVariant value(const QString &key) const;
    void setValue(const QString &key, QVariant value);

private:
    QList<BuildStep *> m_buildSteps;
    QList<BuildStep *> m_cleanSteps;
    QHash<QString, QVariant> m_values;
    Project *m_project;
};

class PROJECTEXPLORER_EXPORT IBuildConfigurationFactory : public QObject
{
    Q_OBJECT

public:
    IBuildConfigurationFactory(QObject *parent = 0);
    virtual ~IBuildConfigurationFactory();

    // used to show the list of possible additons to a project, returns a list of types
    virtual QStringList availableCreationTypes() const = 0;
    // used to translate the types to names to display to the user
    virtual QString displayNameForType(const QString &type) const = 0;

    // creates build configuration(s) for given type and adds them to project
    // if successfull returns the BuildConfiguration that should be shown in the
    // project mode for editing
    virtual BuildConfiguration *create(const QString &type) const = 0;

    // clones a given BuildConfiguration, should not add it to the project
    virtual BuildConfiguration *clone(BuildConfiguration *source) const = 0;

    // restores a BuildConfiguration with the name and adds it to the project
    virtual BuildConfiguration *restore() const = 0;

signals:
    void availableCreationTypesChanged();
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::BuildConfiguration *);

#endif // BUILDCONFIGURATION_H

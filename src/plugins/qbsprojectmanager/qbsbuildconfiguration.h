/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QBSBUILDCONFIGURATION_H
#define QBSBUILDCONFIGURATION_H

#include "qbsprojectmanager_global.h"

#include <projectexplorer/buildconfiguration.h>
#include <qtsupport/baseqtversion.h>

namespace ProjectExplorer { class FileNode; }

namespace QbsProjectManager {

namespace Internal {

class QbsBuildConfigurationFactory;
class QbsBuildConfigurationWidget;
class QbsBuildStep;
class QbsProject;

class QbsBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

public:
    explicit QbsBuildConfiguration(ProjectExplorer::Target *target);

    ProjectExplorer::NamedWidget *createConfigWidget();

    QbsBuildStep *qbsStep() const;
    QVariantMap qbsConfiguration() const;
    QString buildDirectory() const;

    Internal::QbsProject *project() const;

    QVariantMap toMap() const;

    ProjectExplorer::IOutputParser *createOutputParser() const;

    bool isEnabled() const;
    QString disabledReason() const;

    BuildType buildType() const;

    void setChangedFiles(const QStringList &files);
    QStringList changedFiles() const;

signals:
    void qbsConfigurationChanged();

protected:
    QbsBuildConfiguration(ProjectExplorer::Target *target, QbsBuildConfiguration *source);
    QbsBuildConfiguration(ProjectExplorer::Target *target, const Core::Id id);
    bool fromMap(const QVariantMap &map);

private slots:
    void buildStepInserted(int pos);

private:
    static QbsBuildConfiguration *setup(ProjectExplorer::Target *t,
                                        const QString &defaultDisplayName,
                                        const QString &displayName,
                                        const QVariantMap &buildData,
                                        const Utils::FileName &directory);
    void setBuildDirectory(const Utils::FileName &dir);

    bool m_isParsing;
    bool m_parsingError;
    Utils::FileName m_buildDirectory;
    QStringList m_changedFiles;

    friend class QbsBuildConfigurationFactory;
    friend class QbsBuildConfigurationWidget;
};

class QbsBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit QbsBuildConfigurationFactory(QObject *parent = 0);
    ~QbsBuildConfigurationFactory();

    QList<Core::Id> availableCreationIds(const ProjectExplorer::Target *parent) const;
    QString displayNameForId(const Core::Id id) const;

    bool canCreate(const ProjectExplorer::Target *parent, const Core::Id id) const;
    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent, const Core::Id id, const QString &name = QString());
    bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const;
    ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source);
    bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);

private:
    bool canHandle(const ProjectExplorer::Target *t) const;
};

} // namespace Internal
} // namespace QbsProjectManager

#endif // QBSBUILDCONFIGURATION_H

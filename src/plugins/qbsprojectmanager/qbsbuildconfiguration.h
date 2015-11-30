/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QBSBUILDCONFIGURATION_H
#define QBSBUILDCONFIGURATION_H

#include "qbsprojectmanager_global.h"

#include <projectexplorer/buildconfiguration.h>
#include <qtsupport/baseqtversion.h>

namespace ProjectExplorer {
class BuildStep;
class FileNode;
}

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

    Internal::QbsProject *project() const;

    ProjectExplorer::IOutputParser *createOutputParser() const;

    bool isEnabled() const;
    QString disabledReason() const;

    BuildType buildType() const;

    void setChangedFiles(const QStringList &files);
    QStringList changedFiles() const;

    void setActiveFileTags(const QStringList &fileTags);
    QStringList activeFileTags() const;

    void setProducts(const QStringList &products);
    QStringList products() const;

    void emitBuildTypeChanged();

    static QString equivalentCommandLine(const ProjectExplorer::BuildStep *buildStep);

signals:
    void qbsConfigurationChanged();

protected:
    QbsBuildConfiguration(ProjectExplorer::Target *target, QbsBuildConfiguration *source);
    QbsBuildConfiguration(ProjectExplorer::Target *target, Core::Id id);
    bool fromMap(const QVariantMap &map);

private slots:
    void buildStepInserted(int pos);

private:
    static QbsBuildConfiguration *setup(ProjectExplorer::Target *t,
                                        const QString &defaultDisplayName,
                                        const QString &displayName,
                                        const QVariantMap &buildData,
                                        const Utils::FileName &directory);

    bool m_isParsing;
    bool m_parsingError;
    QStringList m_changedFiles;
    QStringList m_activeFileTags;
    QStringList m_products;

    friend class QbsBuildConfigurationFactory;
    friend class QbsBuildConfigurationWidget;
};

class QbsBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit QbsBuildConfigurationFactory(QObject *parent = 0);
    ~QbsBuildConfigurationFactory();

    int priority(const ProjectExplorer::Target *parent) const;
    QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const;
    int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const;
    QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k,
                                                        const QString &projectPath) const;
    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent,
                                                const ProjectExplorer::BuildInfo *info) const;

    bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const;
    ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source);
    bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);

private:
    bool canHandle(const ProjectExplorer::Target *t) const;
    ProjectExplorer::BuildInfo *createBuildInfo(const ProjectExplorer::Kit *k,
                                                ProjectExplorer::BuildConfiguration::BuildType type) const;
};

} // namespace Internal
} // namespace QbsProjectManager

#endif // QBSBUILDCONFIGURATION_H

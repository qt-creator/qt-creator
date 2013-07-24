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

#ifndef QT4BUILDCONFIGURATION_H
#define QT4BUILDCONFIGURATION_H

#include "qt4projectmanager_global.h"

#include <projectexplorer/buildconfiguration.h>
#include <qtsupport/baseqtversion.h>

namespace ProjectExplorer { class FileNode; }

namespace Qt4ProjectManager {

class QmakeBuildInfo;
class QMakeStep;
class MakeStep;
class Qt4BuildConfigurationFactory;
class Qt4ProFileNode;

namespace Internal { class Qt4ProjectConfigWidget; }

class QT4PROJECTMANAGER_EXPORT Qt4BuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

public:
    explicit Qt4BuildConfiguration(ProjectExplorer::Target *target);
    ~Qt4BuildConfiguration();

    ProjectExplorer::NamedWidget *createConfigWidget();
    bool isShadowBuild() const;

    void setSubNodeBuild(Qt4ProjectManager::Qt4ProFileNode *node);
    Qt4ProjectManager::Qt4ProFileNode *subNodeBuild() const;

    ProjectExplorer::FileNode *fileNodeBuild() const;
    void setFileNodeBuild(ProjectExplorer::FileNode *node);

    QtSupport::BaseQtVersion::QmakeBuildConfigs qmakeBuildConfiguration() const;
    void setQMakeBuildConfiguration(QtSupport::BaseQtVersion::QmakeBuildConfigs config);

    /// \internal for qmakestep
    // used by qmake step to notify that the qmake args have changed
    // not really nice, the build configuration should save the arguments
    // since they are needed for reevaluation
    void emitQMakeBuildConfigurationChanged();

    QStringList configCommandLineArguments() const;

    // Those functions are used in a few places.
    // The drawback is that we shouldn't actually depend on them being always there
    // That is generally the stuff that is asked should normally be transferred to
    // Qt4Project *
    // So that we can later enable people to build qt4projects the way they would like
    QMakeStep *qmakeStep() const;
    MakeStep *makeStep() const;

    QString makefile() const;

    enum MakefileState { MakefileMatches, MakefileForWrongProject, MakefileIncompatible, MakefileMissing };
    MakefileState compareToImportFrom(const QString &makefile);
    static bool removeQMLInspectorFromArguments(QString *args);
    static Utils::FileName extractSpecFromArguments(QString *arguments,
                                            const QString &directory, const QtSupport::BaseQtVersion *version,
                                            QStringList *outArgs = 0);

    QVariantMap toMap() const;

    virtual bool isEnabled() const;
    virtual QString disabledReason() const;
    /// \internal For Qt4Project, since that manages the parsing information
    void setEnabled(bool enabled);

    BuildType buildType() const;

    /// returns whether the Qt version in the profile supports shadow building (also true for no Qt version)
    bool supportsShadowBuilds();

public slots:
    void emitProFileEvaluateNeeded();

signals:
    /// emitted for setQMakeBuildConfig, not emitted for Qt version changes, even
    /// if those change the qmakebuildconfig
    void qmakeBuildConfigurationChanged();
    void shadowBuildChanged();

private slots:
    void kitChanged();
    void toolChainUpdated(ProjectExplorer::ToolChain *tc);
    void qtVersionsChanged(const QList<int> &, const QList<int> &, const QList<int> &changed);

protected:
    Qt4BuildConfiguration(ProjectExplorer::Target *target, Qt4BuildConfiguration *source);
    Qt4BuildConfiguration(ProjectExplorer::Target *target, const Core::Id id);
    virtual bool fromMap(const QVariantMap &map);

private:
    void ctor();
    QString defaultShadowBuildDirectory() const;
    void setBuildDirectory(const Utils::FileName &directory);
    void updateShadowBuild();

    class LastKitState
    {
    public:
        LastKitState();
        explicit LastKitState(ProjectExplorer::Kit *k);
        bool operator ==(const LastKitState &other) const;
        bool operator !=(const LastKitState &other) const;
    private:
        int m_qtVersion;
        QString m_toolchain;
        QString m_sysroot;
        QString m_mkspec;
    };
    LastKitState m_lastKitState;

    bool m_shadowBuild;
    bool m_isEnabled;
    bool m_qtVersionSupportsShadowBuilds;
    QtSupport::BaseQtVersion::QmakeBuildConfigs m_qmakeBuildConfiguration;
    Qt4ProjectManager::Qt4ProFileNode *m_subNodeBuild;
    ProjectExplorer::FileNode *m_fileNodeBuild;

    friend class Internal::Qt4ProjectConfigWidget;
    friend class Qt4BuildConfigurationFactory;
};

class QT4PROJECTMANAGER_EXPORT Qt4BuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit Qt4BuildConfigurationFactory(QObject *parent = 0);
    ~Qt4BuildConfigurationFactory();

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

private slots:
    void update();

private:
    bool canHandle(const ProjectExplorer::Target *t) const;
    QmakeBuildInfo *createBuildInfo(const ProjectExplorer::Kit *k, const QString &projectPath,
                                    ProjectExplorer::BuildConfiguration::BuildType type) const;
};

} // namespace Qt4ProjectManager

#endif // QT4BUILDCONFIGURATION_H

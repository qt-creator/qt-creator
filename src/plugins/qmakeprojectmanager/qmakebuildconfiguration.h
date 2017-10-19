/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qmakeprojectmanager_global.h"

#include <projectexplorer/buildconfiguration.h>
#include <qtsupport/baseqtversion.h>

namespace ProjectExplorer { class FileNode; }

namespace QmakeProjectManager {

class QmakeBuildInfo;
class QMakeStep;
class MakeStep;
class QmakeBuildConfigurationFactory;
class QmakeProFileNode;

namespace Internal { class QmakeProjectConfigWidget; }

class QMAKEPROJECTMANAGER_EXPORT QmakeBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

public:
    explicit QmakeBuildConfiguration(ProjectExplorer::Target *target);
    ~QmakeBuildConfiguration();

    ProjectExplorer::NamedWidget *createConfigWidget() override;
    bool isShadowBuild() const;

    void setSubNodeBuild(QmakeProFileNode *node);
    QmakeProFileNode *subNodeBuild() const;

    ProjectExplorer::FileNode *fileNodeBuild() const;
    void setFileNodeBuild(ProjectExplorer::FileNode *node);

    QtSupport::BaseQtVersion::QmakeBuildConfigs qmakeBuildConfiguration() const;
    void setQMakeBuildConfiguration(QtSupport::BaseQtVersion::QmakeBuildConfigs config);

    /// suffix should be unique
    static QString shadowBuildDirectory(const QString &profilePath, const ProjectExplorer::Kit *k,
                                        const QString &suffix, BuildConfiguration::BuildType type);

    /// \internal for qmakestep
    // used by qmake step to notify that the qmake args have changed
    // not really nice, the build configuration should save the arguments
    // since they are needed for reevaluation
    void emitQMakeBuildConfigurationChanged();

    QStringList configCommandLineArguments() const;

    // Those functions are used in a few places.
    // The drawback is that we shouldn't actually depend on them being always there
    // That is generally the stuff that is asked should normally be transferred to
    // QmakeProject *
    // So that we can later enable people to build qmake the way they would like
    QMakeStep *qmakeStep() const;
    MakeStep *makeStep() const;

    QString makefile() const;

    enum MakefileState { MakefileMatches, MakefileForWrongProject, MakefileIncompatible, MakefileMissing };
    MakefileState compareToImportFrom(const QString &makefile, QString *errorString = 0);
    static Utils::FileName extractSpecFromArguments(QString *arguments,
                                            const QString &directory, const QtSupport::BaseQtVersion *version,
                                            QStringList *outArgs = 0);

    QVariantMap toMap() const override;

    virtual bool isEnabled() const override;
    virtual QString disabledReason() const override;
    /// \internal For QmakeProject, since that manages the parsing information
    void setEnabled(bool enabled);

    BuildType buildType() const override;

    void addToEnvironment(Utils::Environment &env) const override;

    void emitProFileEvaluateNeeded();

signals:
    /// emitted for setQMakeBuildConfig, not emitted for Qt version changes, even
    /// if those change the qmakebuildconfig
    void qmakeBuildConfigurationChanged();
    void shadowBuildChanged();

private:
    void kitChanged();
    void toolChainUpdated(ProjectExplorer::ToolChain *tc);
    void qtVersionsChanged(const QList<int> &, const QList<int> &, const QList<int> &changed);

protected:
    QmakeBuildConfiguration(ProjectExplorer::Target *target, QmakeBuildConfiguration *source);
    QmakeBuildConfiguration(ProjectExplorer::Target *target, Core::Id id);
    bool fromMap(const QVariantMap &map) override;

private:
    void ctor();

    class LastKitState
    {
    public:
        LastKitState();
        explicit LastKitState(ProjectExplorer::Kit *k);
        bool operator ==(const LastKitState &other) const;
        bool operator !=(const LastKitState &other) const;
    private:
        int m_qtVersion = -1;
        QByteArray m_toolchain;
        QString m_sysroot;
        QString m_mkspec;
    };
    LastKitState m_lastKitState;

    bool m_shadowBuild = true;
    bool m_isEnabled = true;
    QtSupport::BaseQtVersion::QmakeBuildConfigs m_qmakeBuildConfiguration = 0;
    QmakeProFileNode *m_subNodeBuild = nullptr;
    ProjectExplorer::FileNode *m_fileNodeBuild = nullptr;

    friend class Internal::QmakeProjectConfigWidget;
    friend class QmakeBuildConfigurationFactory;
};

class QMAKEPROJECTMANAGER_EXPORT QmakeBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit QmakeBuildConfigurationFactory(QObject *parent = 0);

    int priority(const ProjectExplorer::Target *parent) const override;
    QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const override;
    int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
    QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k,
                                                        const QString &projectPath) const override;
    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent,
                                                const ProjectExplorer::BuildInfo *info) const override;

    bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const override;
    ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) override;
    bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    ProjectExplorer::BuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map) override;
protected:
    void configureBuildConfiguration(ProjectExplorer::Target *parent, QmakeBuildConfiguration *bc, const QmakeBuildInfo *info) const;

private:
    void update();

    bool canHandle(const ProjectExplorer::Target *t) const;
    QmakeBuildInfo *createBuildInfo(const ProjectExplorer::Kit *k, const QString &projectPath,
                                    ProjectExplorer::BuildConfiguration::BuildType type) const;
};

} // namespace QmakeProjectManager

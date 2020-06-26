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

#include <projectexplorer/projectconfigurationaspects.h>
#include <projectexplorer/buildconfiguration.h>
#include <qtsupport/baseqtversion.h>

namespace ProjectExplorer { class FileNode; }

namespace QmakeProjectManager {

class QMakeStep;
class QmakeBuildSystem;
class QmakeMakeStep;
class QmakeProFileNode;

class QMAKEPROJECTMANAGER_EXPORT QmakeBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

public:
    QmakeBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);
    ~QmakeBuildConfiguration() override;

    ProjectExplorer::BuildSystem *buildSystem() const final;

    void setSubNodeBuild(QmakeProFileNode *node);
    QmakeProFileNode *subNodeBuild() const;

    ProjectExplorer::FileNode *fileNodeBuild() const;
    void setFileNodeBuild(ProjectExplorer::FileNode *node);

    QtSupport::BaseQtVersion::QmakeBuildConfigs qmakeBuildConfiguration() const;
    void setQMakeBuildConfiguration(QtSupport::BaseQtVersion::QmakeBuildConfigs config);

    /// suffix should be unique
    static Utils::FilePath shadowBuildDirectory(const Utils::FilePath &profilePath,
                                                const ProjectExplorer::Kit *k,
                                                const QString &suffix,
                                                BuildConfiguration::BuildType type);

    QStringList configCommandLineArguments() const;

    // Those functions are used in a few places.
    // The drawback is that we shouldn't actually depend on them being always there
    // That is generally the stuff that is asked should normally be transferred to
    // QmakeProject *
    // So that we can later enable people to build qmake the way they would like
    QMakeStep *qmakeStep() const;
    QmakeMakeStep *makeStep() const;

    QmakeBuildSystem *qmakeBuildSystem() const;

    QString makefile() const;

    enum MakefileState { MakefileMatches, MakefileForWrongProject, MakefileIncompatible, MakefileMissing };
    MakefileState compareToImportFrom(const QString &makefile, QString *errorString = nullptr);
    static QString extractSpecFromArguments(
            QString *arguments, const QString &directory, const QtSupport::BaseQtVersion *version,
            QStringList *outArgs = nullptr);

    QVariantMap toMap() const override;

    BuildType buildType() const override;

    void addToEnvironment(Utils::Environment &env) const override;
    static void setupBuildEnvironment(ProjectExplorer::Kit *k, Utils::Environment &env);

    static QString unalignedBuildDirWarning();
    static bool isBuildDirAtSafeLocation(const QString &sourceDir, const QString &buildDir);
    bool isBuildDirAtSafeLocation() const;

    ProjectExplorer::TriState separateDebugInfo() const;
    void forceSeparateDebugInfo(bool sepDebugInfo);

    ProjectExplorer::TriState qmlDebugging() const;
    void forceQmlDebugging(bool enable);

    ProjectExplorer::TriState useQtQuickCompiler() const;
    void forceQtQuickCompiler(bool enable);

signals:
    /// emitted for setQMakeBuildConfig, not emitted for Qt version changes, even
    /// if those change the qmakebuildconfig
    void qmakeBuildConfigurationChanged();

    void separateDebugInfoChanged();
    void qmlDebuggingChanged();
    void useQtQuickCompilerChanged();

protected:
    bool fromMap(const QVariantMap &map) override;
    bool regenerateBuildFiles(ProjectExplorer::Node *node = nullptr) override;

private:
    void restrictNextBuild(const ProjectExplorer::RunConfiguration *rc) override;

    void kitChanged();
    void toolChainUpdated(ProjectExplorer::ToolChain *tc);
    void qtVersionsChanged(const QList<int> &, const QList<int> &, const QList<int> &changed);
    void updateProblemLabel();

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

    QtSupport::BaseQtVersion::QmakeBuildConfigs m_qmakeBuildConfiguration;
    QmakeProFileNode *m_subNodeBuild = nullptr;
    ProjectExplorer::FileNode *m_fileNodeBuild = nullptr;
    QmakeBuildSystem *m_buildSystem = nullptr;
};

class QMAKEPROJECTMANAGER_EXPORT QmakeBuildConfigurationFactory : public ProjectExplorer::BuildConfigurationFactory
{
public:
    QmakeBuildConfigurationFactory();
};

} // namespace QmakeProjectManager

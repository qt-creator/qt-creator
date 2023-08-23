// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmakeprojectmanager_global.h"

#include <projectexplorer/buildaspects.h>
#include <projectexplorer/buildconfiguration.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtbuildaspects.h>

#include <utils/aspects.h>

namespace ProjectExplorer {
class FileNode;
class MakeStep;
} // ProjectExplorer

namespace QmakeProjectManager {

class QMakeStep;
class QmakeBuildSystem;
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

    QtSupport::QtVersion::QmakeBuildConfigs qmakeBuildConfiguration() const;
    void setQMakeBuildConfiguration(QtSupport::QtVersion::QmakeBuildConfigs config);

    /// suffix should be unique
    static Utils::FilePath shadowBuildDirectory(const Utils::FilePath &profilePath,
                                                const ProjectExplorer::Kit *k,
                                                const QString &suffix,
                                                BuildConfiguration::BuildType type);

    QStringList configCommandLineArguments() const;

    // This function is used in a few places.
    // The drawback is that we shouldn't actually depend on them being always there
    // That is generally the stuff that is asked should normally be transferred to
    // QmakeProject *
    // So that we can later enable people to build qmake the way they would like
    QMakeStep *qmakeStep() const;

    QmakeBuildSystem *qmakeBuildSystem() const;

    Utils::FilePath makefile() const;

    enum MakefileState { MakefileMatches, MakefileForWrongProject, MakefileIncompatible, MakefileMissing };
    MakefileState compareToImportFrom(const Utils::FilePath &makefile, QString *errorString = nullptr);
    static QString extractSpecFromArguments(
            QString *arguments, const Utils::FilePath &directory, const QtSupport::QtVersion *version,
            QStringList *outArgs = nullptr);

    void toMap(Utils::Store &map) const override;

    BuildType buildType() const override;

    void addToEnvironment(Utils::Environment &env) const override;

    static QString unalignedBuildDirWarning();
    static bool isBuildDirAtSafeLocation(const Utils::FilePath &sourceDir,
                                         const Utils::FilePath &buildDir);
    bool isBuildDirAtSafeLocation() const;

    void forceSeparateDebugInfo(bool sepDebugInfo);
    void forceQmlDebugging(bool enable);
    void forceQtQuickCompiler(bool enable);

    ProjectExplorer::SeparateDebugInfoAspect separateDebugInfo{this};
    QtSupport::QmlDebuggingAspect qmlDebugging{this};
    QtSupport::QtQuickCompilerAspect useQtQuickCompiler{this};
    Utils::SelectionAspect runSystemFunctions{this};

    bool runQmakeSystemFunctions() const;

signals:
    /// emitted for setQMakeBuildConfig, not emitted for Qt version changes, even
    /// if those change the qmakebuildconfig
    void qmakeBuildConfigurationChanged();

    void separateDebugInfoChanged();
    void qmlDebuggingChanged();
    void useQtQuickCompilerChanged();

protected:
    void fromMap(const Utils::Store &map) override;
    bool regenerateBuildFiles(ProjectExplorer::Node *node = nullptr) override;

private:
    void restrictNextBuild(const ProjectExplorer::RunConfiguration *rc) override;

    void kitChanged();
    void toolChainUpdated(ProjectExplorer::ToolChain *tc);
    void qtVersionsChanged(const QList<int> &, const QList<int> &, const QList<int> &changed);
    void updateProblemLabel();

    ProjectExplorer::MakeStep *makeStep() const;

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

    QtSupport::QtVersion::QmakeBuildConfigs m_qmakeBuildConfiguration;
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

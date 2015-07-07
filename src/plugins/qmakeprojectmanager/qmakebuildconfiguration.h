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

#ifndef QMAKEBUILDCONFIGURATION_H
#define QMAKEBUILDCONFIGURATION_H

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

    ProjectExplorer::NamedWidget *createConfigWidget();
    bool isShadowBuild() const;

    void setSubNodeBuild(QmakeProjectManager::QmakeProFileNode *node);
    QmakeProjectManager::QmakeProFileNode *subNodeBuild() const;

    ProjectExplorer::FileNode *fileNodeBuild() const;
    void setFileNodeBuild(ProjectExplorer::FileNode *node);

    QtSupport::BaseQtVersion::QmakeBuildConfigs qmakeBuildConfiguration() const;
    void setQMakeBuildConfiguration(QtSupport::BaseQtVersion::QmakeBuildConfigs config);

    /// suffix should be unique
    static QString shadowBuildDirectory(const QString &profilePath, const ProjectExplorer::Kit *k,
                                 const QString &suffix);

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
    MakefileState compareToImportFrom(const QString &makefile);
    static Utils::FileName extractSpecFromArguments(QString *arguments,
                                            const QString &directory, const QtSupport::BaseQtVersion *version,
                                            QStringList *outArgs = 0);

    QVariantMap toMap() const;

    virtual bool isEnabled() const;
    virtual QString disabledReason() const;
    /// \internal For QmakeProject, since that manages the parsing information
    void setEnabled(bool enabled);

    BuildType buildType() const;

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
    QmakeBuildConfiguration(ProjectExplorer::Target *target, QmakeBuildConfiguration *source);
    QmakeBuildConfiguration(ProjectExplorer::Target *target, Core::Id id);
    virtual bool fromMap(const QVariantMap &map);
    void setBuildDirectory(const Utils::FileName &directory);

private:
    void ctor();
    QString defaultShadowBuildDirectory() const;

    class LastKitState
    {
    public:
        LastKitState();
        explicit LastKitState(ProjectExplorer::Kit *k);
        bool operator ==(const LastKitState &other) const;
        bool operator !=(const LastKitState &other) const;
    private:
        int m_qtVersion;
        QByteArray m_toolchain;
        QString m_sysroot;
        QString m_mkspec;
    };
    LastKitState m_lastKitState;

    bool m_shadowBuild = true;
    bool m_isEnabled = false;
    QtSupport::BaseQtVersion::QmakeBuildConfigs m_qmakeBuildConfiguration = 0;
    QmakeProjectManager::QmakeProFileNode *m_subNodeBuild = nullptr;
    ProjectExplorer::FileNode *m_fileNodeBuild = nullptr;

    friend class Internal::QmakeProjectConfigWidget;
    friend class QmakeBuildConfigurationFactory;
};

class QMAKEPROJECTMANAGER_EXPORT QmakeBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit QmakeBuildConfigurationFactory(QObject *parent = 0);
    ~QmakeBuildConfigurationFactory();

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
protected:
    void configureBuildConfiguration(ProjectExplorer::Target *parent, QmakeBuildConfiguration *bc, const QmakeBuildInfo *info) const;

private slots:
    void update();

private:
    bool canHandle(const ProjectExplorer::Target *t) const;
    QmakeBuildInfo *createBuildInfo(const ProjectExplorer::Kit *k, const QString &projectPath,
                                    ProjectExplorer::BuildConfiguration::BuildType type) const;
};

} // namespace QmakeProjectManager

#endif // QMAKEBUILDCONFIGURATION_H

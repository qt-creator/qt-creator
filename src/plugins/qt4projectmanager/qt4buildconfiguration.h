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

#include "buildconfigurationinfo.h"

#include <projectexplorer/buildconfiguration.h>
#include <qtsupport/baseqtversion.h>

namespace ProjectExplorer { class FileNode; }

namespace Qt4ProjectManager {

class QMakeStep;
class MakeStep;
class Qt4BuildConfigurationFactory;
class Qt4ProFileNode;

class QT4PROJECTMANAGER_EXPORT Qt4BuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class Qt4BuildConfigurationFactory;

public:
    explicit Qt4BuildConfiguration(ProjectExplorer::Target *target);
    ~Qt4BuildConfiguration();

    ProjectExplorer::NamedWidget *createConfigWidget();
    QString buildDirectory() const;
    bool shadowBuild() const;
    QString shadowBuildDirectory() const;
    void setShadowBuildAndDirectory(bool shadowBuild, const QString &buildDirectory);

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

    static Qt4BuildConfiguration *setup(ProjectExplorer::Target *t,
                                        QString defaultDisplayName,
                                        QString displayName,
                                        QtSupport::BaseQtVersion::QmakeBuildConfigs qmakeBuildConfiguration,
                                        QString additionalArguments,
                                        QString directory,
                                        bool importing);
    /// returns whether the Qt version in the profile supports shadow building (also true for no Qt version)
    bool supportsShadowBuilds();

public slots:
    void emitProFileEvaluateNeeded();

signals:
    /// emitted for setQMakeBuildConfig, not emitted for Qt version changes, even
    /// if those change the qmakebuildconfig
    void qmakeBuildConfigurationChanged();

private slots:
    void kitChanged();
    void toolChainUpdated(ProjectExplorer::ToolChain *tc);
    void qtVersionsChanged(const QList<int> &, const QList<int> &, const QList<int> &changed);
    void emitBuildDirectoryChanged();

protected:
    Qt4BuildConfiguration(ProjectExplorer::Target *target, Qt4BuildConfiguration *source);
    Qt4BuildConfiguration(ProjectExplorer::Target *target, const Core::Id id);
    virtual bool fromMap(const QVariantMap &map);

private:
    void ctor();
    QString rawBuildDirectory() const;
    QString defaultShadowBuildDirectory() const;

    class LastKitState
    {
    public:
        LastKitState();
        explicit LastKitState(ProjectExplorer::Kit *k);
        bool operator ==(const LastKitState &other);
        bool operator !=(const LastKitState &other);
    private:
        int m_qtVersion;
        QString m_toolchain;
        QString m_sysroot;
        QString m_mkspec;
    };
    LastKitState m_lastKitState;

    bool m_shadowBuild;
    bool m_isEnabled;
    QString m_buildDirectory;
    QString m_lastEmmitedBuildDirectory;
    bool m_qtVersionSupportsShadowBuilds;
    QtSupport::BaseQtVersion::QmakeBuildConfigs m_qmakeBuildConfiguration;
    Qt4ProjectManager::Qt4ProFileNode *m_subNodeBuild;
    ProjectExplorer::FileNode *m_fileNodeBuild;
};

class QT4PROJECTMANAGER_EXPORT Qt4BuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit Qt4BuildConfigurationFactory(QObject *parent = 0);
    ~Qt4BuildConfigurationFactory();

    QList<Core::Id> availableCreationIds(const ProjectExplorer::Target *parent) const;
    QString displayNameForId(const Core::Id id) const;

    bool canCreate(const ProjectExplorer::Target *parent, const Core::Id id) const;
    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent, const Core::Id id, const QString &name = QString());
    bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const;
    ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source);
    bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);

    static QList<BuildConfigurationInfo> availableBuildConfigurations(const ProjectExplorer::Kit *k, const QString &proFilePath);
    static QString buildConfigurationDisplayName(const BuildConfigurationInfo &info);

private slots:
    void update();

private:
    bool canHandle(const ProjectExplorer::Target *t) const;
};

} // namespace Qt4ProjectManager

#endif // QT4BUILDCONFIGURATION_H

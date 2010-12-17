/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QT4BUILDCONFIGURATION_H
#define QT4BUILDCONFIGURATION_H

#include "qtversionmanager.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/toolchaintype.h>

namespace ProjectExplorer {
class ToolChain;
}

namespace Qt4ProjectManager {

class QMakeStep;
class MakeStep;
class Qt4Target;

namespace Internal {
class Qt4ProFileNode;
class Qt4BuildConfigurationFactory;
}

class Qt4BuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class Internal::Qt4BuildConfigurationFactory;

public:
    explicit Qt4BuildConfiguration(Qt4Target *target);
    virtual ~Qt4BuildConfiguration();

    Qt4Target *qt4Target() const;

    virtual Utils::Environment baseEnvironment() const;

    virtual QString buildDirectory() const;
    bool shadowBuild() const;
    QString shadowBuildDirectory() const;
    void setShadowBuildAndDirectory(bool shadowBuild, const QString &buildDirectory);

    void setSubNodeBuild(Qt4ProjectManager::Internal::Qt4ProFileNode *node);
    Qt4ProjectManager::Internal::Qt4ProFileNode *subNodeBuild() const;

    // returns the qtVersion
    QtVersion *qtVersion() const;
    void setQtVersion(QtVersion *);

    ProjectExplorer::ToolChain *toolChain() const;
    void setToolChainType(ProjectExplorer::ToolChainType type);
    ProjectExplorer::ToolChainType toolChainType() const;

    QtVersion::QmakeBuildConfigs qmakeBuildConfiguration() const;
    void setQMakeBuildConfiguration(QtVersion::QmakeBuildConfigs config);

    /// \internal for qmakestep
    void emitProFileEvaluteNeeded();
    // used by qmake step to notify that the qmake args have changed
    // not really nice, the build configuration should save the arguments
    // since they are needed for reevaluation
    void emitQMakeBuildConfigurationChanged();
    // used by qmake step to notify that the build directory was initialized
    // not really nice
    void emitBuildDirectoryInitialized();
    // used by S60CreatePackageStep to notify that the smart installer property changed
    // not really nice
    void emitS60CreatesSmartInstallerChanged();

    QStringList configCommandLineArguments() const;

    // Those functions are used in a few places.
    // The drawback is that we shouldn't actually depend on them being always there
    // That is generally the stuff that is asked should normally be transferred to
    // Qt4Project *
    // So that we can later enable people to build qt4projects the way they would like
    QMakeStep *qmakeStep() const;
    MakeStep *makeStep() const;

    QString makeCommand() const;
    QString defaultMakeTarget() const;
    QString makefile() const;

    bool compareToImportFrom(const QString &makefile);
    static void removeQMLInspectorFromArguments(QString *args);
    static QString extractSpecFromArguments(QString *arguments,
                                            const QString &directory, const QtVersion *version,
                                            QStringList *outArgs = 0);

    QVariantMap toMap() const;

    ProjectExplorer::IOutputParser *createOutputParser() const;

signals:
    /// emitted if the qt version changes (either directly, or because the default qt version changed
    /// or because the user changed the settings for the qt version
    void qtVersionChanged();
    /// emitted iff the setToolChainType() function is called, not emitted for qtversion changes
    /// even if those result in a toolchain change
    void toolChainTypeChanged();
    /// emitted for setQMakeBuildConfig, not emitted for qt version changes, even
    /// if those change the qmakebuildconfig
    void qmakeBuildConfigurationChanged();
    /// emitted when smart installer property of S60 create package step changes
    void s60CreatesSmartInstallerChanged();

    /// emitted if the build configuration changed in a way that
    /// should trigger a reevaluation of all .pro files
    void proFileEvaluateNeeded(Qt4ProjectManager::Qt4BuildConfiguration *);

    void buildDirectoryInitialized();

private slots:
    void qtVersionsChanged(const QList<int> &changedVersions);
    void emitBuildDirectoryChanged();

protected:
    Qt4BuildConfiguration(Qt4Target *target, Qt4BuildConfiguration *source);
    Qt4BuildConfiguration(Qt4Target *target, const QString &id);
    virtual bool fromMap(const QVariantMap &map);

private:
    void ctor();
    void pickValidQtVersion();
    QString rawBuildDirectory() const;

    bool m_shadowBuild;
    QString m_buildDirectory;
    QString m_lastEmmitedBuildDirectory;
    int m_qtVersionId;
    int m_toolChainType;
    QtVersion::QmakeBuildConfigs m_qmakeBuildConfiguration;
    Qt4ProjectManager::Internal::Qt4ProFileNode *m_subNodeBuild;
};

namespace Internal {
class Qt4BuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit Qt4BuildConfigurationFactory(QObject *parent = 0);
    ~Qt4BuildConfigurationFactory();

    QStringList availableCreationIds(ProjectExplorer::Target *parent) const;
    QString displayNameForId(const QString &id) const;

    bool canCreate(ProjectExplorer::Target *parent, const QString &id) const;
    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent, const QString &id);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const;
    ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);

private slots:
    void update();

private:
    struct VersionInfo {
        VersionInfo()
            : versionId(-1)
            {}
        VersionInfo(const QString &d, int v)
            : displayName(d), versionId(v)
            {}
        QString displayName;
        int versionId;
    };

    QMap<QString, VersionInfo> m_versions;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4BUILDCONFIGURATION_H

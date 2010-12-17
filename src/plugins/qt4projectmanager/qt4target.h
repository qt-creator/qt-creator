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

#ifndef QT4TARGET_H
#define QT4TARGET_H

#include "qt4buildconfiguration.h"
#include <projectexplorer/target.h>

#include <QtGui/QPixmap>

namespace Qt4ProjectManager {

class Qt4Project;

namespace Internal {
class Qt4ProFileNode;
class Qt4TargetFactory;
class Qt4BuildConfigurationFactory;
class Qt4DeployConfigurationFactory;

struct BuildConfigurationInfo {
    explicit BuildConfigurationInfo(QtVersion *v = 0, QtVersion::QmakeBuildConfigs bc = QtVersion::QmakeBuildConfig(0),
                                    const QString &aa = QString(), const QString &d = QString()) :
        version(v), buildConfig(bc), additionalArguments(aa), directory(d)
    { }
    QtVersion *version;
    QtVersion::QmakeBuildConfigs buildConfig;
    QString additionalArguments;
    QString directory;
};
}

class Qt4Target : public ProjectExplorer::Target
{
    Q_OBJECT
    friend class Internal::Qt4TargetFactory;

public:
    explicit Qt4Target(Qt4Project *parent, const QString &id);
    virtual ~Qt4Target();

    ProjectExplorer::BuildConfigWidget *createConfigWidget();

    Qt4BuildConfiguration *activeBuildConfiguration() const;
    Qt4ProjectManager::Qt4Project *qt4Project() const;

    Qt4BuildConfiguration *addQt4BuildConfiguration(QString displayName,
                                                              QtVersion *qtversion,
                                                              QtVersion::QmakeBuildConfigs qmakeBuildConfiguration,
                                                              QString additionalArguments,
                                                              QString directory);
    void addRunConfigurationForPath(const QString &proFilePath);

    Internal::Qt4BuildConfigurationFactory *buildConfigurationFactory() const;
    ProjectExplorer::DeployConfigurationFactory *deployConfigurationFactory() const;

    QList<ProjectExplorer::ToolChainType> filterToolChainTypes(const QList<ProjectExplorer::ToolChainType> &candidates) const;
    ProjectExplorer::ToolChainType preferredToolChainType(const QList<ProjectExplorer::ToolChainType> &candidates) const;

    QString defaultBuildDirectory() const;
    static QString defaultShadowBuildDirectory(const QString &projectLocation, const QString &id);

    bool fromMap(const QVariantMap &map);

signals:
    void buildDirectoryInitialized();
    /// emitted if the build configuration changed in a way that
    /// should trigger a reevaluation of all .pro files
    void proFileEvaluateNeeded(Qt4ProjectManager::Qt4Target *);

private slots:
    void updateQtVersion();
    void onAddedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void onAddedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void slotUpdateDeviceInformation();
    void onProFileEvaluateNeeded(Qt4ProjectManager::Qt4BuildConfiguration *bc);
    void emitProFileEvaluateNeeded();
    void updateToolTipAndIcon();

private:
    const QPixmap m_connectedPixmap;
    const QPixmap m_disconnectedPixmap;

    Internal::Qt4BuildConfigurationFactory *m_buildConfigurationFactory;
    Internal::Qt4DeployConfigurationFactory *m_deployConfigurationFactory;
};

namespace Internal {
class Qt4TargetFactory : public ProjectExplorer::ITargetFactory
{
    Q_OBJECT

public:
    Qt4TargetFactory(QObject *parent = 0);
    ~Qt4TargetFactory();

    virtual bool supportsTargetId(const QString &id) const;

    QStringList availableCreationIds(ProjectExplorer::Project *parent) const;
    QString displayNameForId(const QString &id) const;

    bool canCreate(ProjectExplorer::Project *parent, const QString &id) const;
    Qt4ProjectManager::Qt4Target *create(ProjectExplorer::Project *parent, const QString &id);
    Qt4ProjectManager::Qt4Target *create(ProjectExplorer::Project *parent, const QString &id, QList<QtVersion *> versions);
    Qt4ProjectManager::Qt4Target *create(ProjectExplorer::Project *parent, const QString &id, QList<Internal::BuildConfigurationInfo> infos);
    bool canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const;
    Qt4ProjectManager::Qt4Target *restore(ProjectExplorer::Project *parent, const QVariantMap &map);
};
}

} // namespace Qt4ProjectManager

#endif // QT4TARGET_H

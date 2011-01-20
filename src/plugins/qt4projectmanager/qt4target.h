/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#include "qtversionmanager.h"

#include <projectexplorer/target.h>
#include <projectexplorer/projectnodes.h>

namespace Qt4ProjectManager {

class Qt4Project;

namespace Internal {
class Qt4ProFileNode;
}

struct BuildConfigurationInfo {
    explicit BuildConfigurationInfo(QtVersion *v, QtVersion::QmakeBuildConfigs bc,
                                    const QString &aa, const QString &d) :
        version(v), buildConfig(bc), additionalArguments(aa), directory(d)
    { }
    QtVersion *version;
    QtVersion::QmakeBuildConfigs buildConfig;
    QString additionalArguments;
    QString directory;
};

class Qt4BaseTarget : public ProjectExplorer::Target
{
    Q_OBJECT
public:
    explicit Qt4BaseTarget(Qt4Project *parent, const QString &id);
    virtual ~Qt4BaseTarget();

    ProjectExplorer::BuildConfigWidget *createConfigWidget();

    Qt4BuildConfiguration *activeBuildConfiguration() const;
    Qt4ProjectManager::Qt4Project *qt4Project() const;

    // This is the same for almost all Qt4Targets
    // so for now offer a convience function
    Qt4BuildConfiguration *addQt4BuildConfiguration(QString displayName,
                                                            QtVersion *qtversion,
                                                            QtVersion::QmakeBuildConfigs qmakeBuildConfiguration,
                                                            QString additionalArguments,
                                                            QString directory);

    virtual void createApplicationProFiles() = 0;
    virtual QList<ProjectExplorer::ToolChainType> filterToolChainTypes(const QList<ProjectExplorer::ToolChainType> &candidates) const;
    virtual ProjectExplorer::ToolChainType preferredToolChainType(const QList<ProjectExplorer::ToolChainType> &candidates) const;
    virtual QString defaultBuildDirectory() const;

    virtual QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Node *n) = 0;
signals:
    void buildDirectoryInitialized();
    /// emitted if the build configuration changed in a way that
    /// should trigger a reevaluation of all .pro files
    void proFileEvaluateNeeded(Qt4ProjectManager::Qt4BaseTarget *);

protected:
    void removeUnconfiguredCustomExectutableRunConfigurations();

private slots:
    void onAddedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void onProFileEvaluateNeeded(Qt4ProjectManager::Qt4BuildConfiguration *bc);
    void emitProFileEvaluateNeeded();
};

class QT4PROJECTMANAGER_EXPORT Qt4BaseTargetFactory : public ProjectExplorer::ITargetFactory
{
    Q_OBJECT
public:
    Qt4BaseTargetFactory(QObject *parent);
    ~Qt4BaseTargetFactory();

    virtual QString defaultShadowBuildDirectory(const QString &projectLocation, const QString &id) =0;
    virtual QList<BuildConfigurationInfo> availableBuildConfigurations(const QString &proFilePath) = 0;

    virtual Qt4BaseTarget *create(ProjectExplorer::Project *parent, const QString &id) = 0;
    virtual Qt4BaseTarget *create(ProjectExplorer::Project *parent,
                                  const QString &id,
                                  QList<BuildConfigurationInfo> infos) = 0;

    static Qt4BaseTargetFactory *qt4BaseTargetFactoryForId(const QString &id);
};

} // namespace Qt4ProjectManager

#endif // QT4TARGET_H

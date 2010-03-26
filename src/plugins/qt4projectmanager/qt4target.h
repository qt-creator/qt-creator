/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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
class ProFileReader;
class Qt4ProFileNode;
class Qt4TargetFactory;
class Qt4BuildConfigurationFactory;

struct BuildConfigurationInfo {
    explicit BuildConfigurationInfo(QtVersion *v = 0, QtVersion::QmakeBuildConfigs bc = QtVersion::QmakeBuildConfig(0),
                                    const QStringList &aa = QStringList(), const QString &d = QString()) :
        version(v), buildConfig(bc), additionalArguments(aa), directory(d)
    { }
    QtVersion *version;
    QtVersion::QmakeBuildConfigs buildConfig;
    QStringList additionalArguments;
    QString directory;
};

class Qt4Target : public ProjectExplorer::Target
{
    Q_OBJECT
    friend class Qt4TargetFactory;

public:
    explicit Qt4Target(Qt4Project *parent, const QString &id);
    virtual ~Qt4Target();

    Qt4BuildConfiguration *activeBuildConfiguration() const;
    Qt4ProjectManager::Qt4Project *qt4Project() const;

    Internal::Qt4BuildConfiguration *addQt4BuildConfiguration(QString displayName,
                                                              QtVersion *qtversion,
                                                              QtVersion::QmakeBuildConfigs qmakeBuildConfiguration,
                                                              QStringList additionalArguments,
                                                              QString directory);
    void addRunConfigurationForPath(const QString &proFilePath);

    Internal::Qt4BuildConfigurationFactory *buildConfigurationFactory() const;

    QList<ProjectExplorer::ToolChain::ToolChainType> filterToolChainTypes(const QList<ProjectExplorer::ToolChain::ToolChainType> &candidates) const;
    ProjectExplorer::ToolChain::ToolChainType preferredToolChainType(const QList<ProjectExplorer::ToolChain::ToolChainType> &candidates) const;

    QString defaultBuildDirectory() const;

signals:
    void buildDirectoryInitialized();
    /// emitted if the build configuration changed in a way that
    /// should trigger a reevaluation of all .pro files
    void proFileEvaluateNeeded(Qt4ProjectManager::Internal::Qt4Target *);

protected:
    bool fromMap(const QVariantMap &map);

private slots:
    void updateQtVersion();
    void onAddedRunConfiguration(ProjectExplorer::RunConfiguration *rc);
    void onAddedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void slotUpdateDeviceInformation();
    void onProFileEvaluateNeeded(Qt4ProjectManager::Internal::Qt4BuildConfiguration *bc);
    void emitProFileEvaluateNeeded();
    void updateToolTipAndIcon();

private:
    const QPixmap m_connectedPixmap;
    const QPixmap m_disconnectedPixmap;

    Internal::Qt4BuildConfigurationFactory *m_buildConfigurationFactory;
};

class Qt4TargetFactory : public ProjectExplorer::ITargetFactory
{
    Q_OBJECT

public:
    Qt4TargetFactory(QObject *parent = 0);
    ~Qt4TargetFactory();

    QStringList availableCreationIds(ProjectExplorer::Project *parent) const;
    QString displayNameForId(const QString &id) const;

    bool canCreate(ProjectExplorer::Project *parent, const QString &id) const;
    Internal::Qt4Target *create(ProjectExplorer::Project *parent, const QString &id);
    Internal::Qt4Target *create(ProjectExplorer::Project *parent, const QString &id, QList<QtVersion *> versions);
    Internal::Qt4Target *create(ProjectExplorer::Project *parent, const QString &id, QList<BuildConfigurationInfo> infos);
    bool canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const;
    Internal::Qt4Target *restore(ProjectExplorer::Project *parent, const QVariantMap &map);
};

} // namespace Internal

} // namespace Qt4ProjectManager

#endif // QT4TARGET_H

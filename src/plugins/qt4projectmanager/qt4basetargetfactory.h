/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QT4BASETARGETFACTORY_H
#define QT4BASETARGETFACTORY_H

#include "qt4projectmanager_global.h"

#include <projectexplorer/task.h>
#include <projectexplorer/target.h>

#include <QtCore/QList>

namespace QtSupport {
class QtVersionNumber;
}

namespace Qt4ProjectManager {
class Qt4TargetSetupWidget;
struct BuildConfigurationInfo;

class QT4PROJECTMANAGER_EXPORT Qt4BaseTargetFactory : public ProjectExplorer::ITargetFactory
{
    Q_OBJECT
public:
    explicit Qt4BaseTargetFactory(QObject *parent);
    virtual ~Qt4BaseTargetFactory();

    virtual Qt4TargetSetupWidget *createTargetSetupWidget(const QString &id,
                                                          const QString &proFilePath,
                                                          const QtSupport::QtVersionNumber &minimumQtVersion,
                                                          const QtSupport::QtVersionNumber &maximumQtVersion,
                                                          bool importEnabled,
                                                          QList<BuildConfigurationInfo> importInfos);

    /// suffix should be unique
    virtual QString shadowBuildDirectory(const QString &profilePath, const QString &id, const QString &suffix);
    /// used by the default implementation of shadowBuildDirectory
    virtual QString buildNameForId(const QString &id) const;

    /// used by the default implementation of createTargetSetupWidget
    /// not needed otherwise
    /// by default creates one debug + one release buildconfiguration per qtversion
    virtual QList<BuildConfigurationInfo> availableBuildConfigurations(const QString &id, const QString &proFilePath,
                                                                       const QtSupport::QtVersionNumber &minimumQtVersion,
                                                                       const QtSupport::QtVersionNumber &maximumQtVersion);

    virtual QList<ProjectExplorer::Task> reportIssues(const QString &proFile);
    /// only used in the TargetSetupPage
    virtual QIcon iconForId(const QString &id) const = 0;

    virtual QSet<QString> targetFeatures(const QString &id) const = 0;
    virtual bool selectByDefault(const QString &id) const;

    virtual ProjectExplorer::Target *create(ProjectExplorer::Project *parent, const QString &id) = 0;
    virtual ProjectExplorer::Target *create(ProjectExplorer::Project *parent, const QString &id, const QList<BuildConfigurationInfo> &infos) = 0;
    virtual ProjectExplorer::Target *create(ProjectExplorer::Project *parent, const QString &id, Qt4TargetSetupWidget *widget);

    static Qt4BaseTargetFactory *qt4BaseTargetFactoryForId(const QString &id);

    static QList<Qt4BaseTargetFactory *> qt4BaseTargetFactoriesForIds(const QStringList &ids);

protected:
    static QString msgBuildConfigurationName(const BuildConfigurationInfo &info);
};

} // namespace Qt4ProjectManager

#endif // QT4BASETARGETFACTORY_H

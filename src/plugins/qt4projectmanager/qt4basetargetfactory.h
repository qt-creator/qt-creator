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

#ifndef QT4BASETARGETFACTORY_H
#define QT4BASETARGETFACTORY_H

#include "qt4projectmanager_global.h"

#include <projectexplorer/target.h>

#include <QtCore/QList>

namespace Qt4ProjectManager {
class Qt4TargetSetupWidget;
class QtVersionNumber;
struct BuildConfigurationInfo;

class QT4PROJECTMANAGER_EXPORT Qt4BaseTargetFactory : public ProjectExplorer::ITargetFactory
{
    Q_OBJECT
public:
    explicit Qt4BaseTargetFactory(QObject *parent);
    virtual ~Qt4BaseTargetFactory();

    virtual Qt4TargetSetupWidget *createTargetSetupWidget(const QString &id,
                                                          const QString &proFilePath,
                                                          const QtVersionNumber &minimumQtVersion,
                                                          bool importEnabled,
                                                          QList<BuildConfigurationInfo> importInfos);

    virtual QString defaultShadowBuildDirectory(const QString &projectLocation, const QString &id) =0;
    /// used by the default implementation of createTargetSetupWidget
    /// not needed otherwise
    virtual QList<BuildConfigurationInfo> availableBuildConfigurations(const QString &id, const QString &proFilePath, const QtVersionNumber &minimumQtVersion) = 0;
    /// only used in the TargetSetupPage
    virtual QIcon iconForId(const QString &id) const = 0;

    virtual bool isMobileTarget(const QString &id) = 0;

    virtual ProjectExplorer::Target *create(ProjectExplorer::Project *parent, const QString &id) = 0;
    virtual ProjectExplorer::Target *create(ProjectExplorer::Project *parent, const QString &id, const QList<BuildConfigurationInfo> &infos) = 0;
    virtual ProjectExplorer::Target *create(ProjectExplorer::Project *parent, const QString &id, Qt4TargetSetupWidget *widget);

    static Qt4BaseTargetFactory *qt4BaseTargetFactoryForId(const QString &id);

protected:
    static QString msgBuildConfigurationName(const BuildConfigurationInfo &info);
};

} // namespace Qt4ProjectManager

#endif // QT4BASETARGETFACTORY_H

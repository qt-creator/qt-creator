/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef QT4SYMBIANTARGETFACTORY_H
#define QT4SYMBIANTARGETFACTORY_H

#include "qt4basetargetfactory.h"

namespace Qt4ProjectManager {
namespace Internal {
class Qt4SymbianTargetFactory : public Qt4BaseTargetFactory
{
    Q_OBJECT
public:
    Qt4SymbianTargetFactory(QObject *parent = 0);
    ~Qt4SymbianTargetFactory();

    QStringList supportedTargetIds(ProjectExplorer::Project *parent) const;
    bool supportsTargetId(const QString &id) const;
    QString displayNameForId(const QString &id) const;
    QIcon iconForId(const QString &id) const;

    bool canCreate(ProjectExplorer::Project *parent, const QString &id) const;
    bool canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const;
    virtual ProjectExplorer::Target *restore(ProjectExplorer::Project *parent, const QVariantMap &map);

    virtual ProjectExplorer::Target *create(ProjectExplorer::Project *parent, const QString &id);
    virtual ProjectExplorer::Target *create(ProjectExplorer::Project *parent, const QString &id, const QList<BuildConfigurationInfo> &infos);
    virtual ProjectExplorer::Target *create(ProjectExplorer::Project *parent, const QString &id, Qt4TargetSetupWidget *widget);

    QString defaultShadowBuildDirectory(const QString &projectLocation, const QString &id);
    QList<ProjectExplorer::Task> reportIssues(const QString &proFile);
    QList<BuildConfigurationInfo> availableBuildConfigurations(const QString &id, const QString &proFilePath, const QtVersionNumber &minimumQtVersion);
    bool isMobileTarget(const QString &id);
    bool supportsShadowBuilds(const QString &id);
};

} // namespace Internal
} // namespace Qt4ProjectManager


#endif // QT4SYMBIANTARGETFACTORY_H

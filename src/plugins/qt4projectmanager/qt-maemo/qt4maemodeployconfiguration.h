/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QT4PROJECTMANAGER_QT4DEPLOYCONFIGURATION_H
#define QT4PROJECTMANAGER_QT4DEPLOYCONFIGURATION_H

#include <projectexplorer/deployconfiguration.h>

namespace Qt4ProjectManager {
namespace Internal {

class Target;

class Qt4MaemoDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_OBJECT

public:
    explicit Qt4MaemoDeployConfigurationFactory(QObject *parent = 0);

    ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent, const QString &id);
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4PROJECTMANAGER_QT4DEPLOYCONFIGURATION_H

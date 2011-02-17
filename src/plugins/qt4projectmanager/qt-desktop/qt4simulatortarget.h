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

#ifndef QT4SIMULATORTARGET_H
#define QT4SIMULATORTARGET_H

#include "qt4target.h"

namespace Qt4ProjectManager {

class Qt4Project;

namespace Internal {

class Qt4SimulatorTarget : public Qt4BaseTarget
{
    friend class Qt4SimulatorTargetFactory;
    Q_OBJECT
public:
    explicit Qt4SimulatorTarget(Qt4Project *parent, const QString &id);
    virtual ~Qt4SimulatorTarget();

    Internal::Qt4BuildConfigurationFactory *buildConfigurationFactory() const;
    ProjectExplorer::DeployConfigurationFactory *deployConfigurationFactory() const;

    void createApplicationProFiles();
    QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Node *n);

    static QString defaultDisplayName();

private:
    Internal::Qt4BuildConfigurationFactory *m_buildConfigurationFactory;
    ProjectExplorer::DeployConfigurationFactory *m_deployConfigurationFactory;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4SIMULATORTARGET_H

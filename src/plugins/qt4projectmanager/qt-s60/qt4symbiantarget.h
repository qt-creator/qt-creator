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

#ifndef QT4SYMBIANTARGET_H
#define QT4SYMBIANTARGET_H

#include "qt4target.h"

#include <QtGui/QPixmap>

namespace Qt4ProjectManager {
class Qt4Project;
namespace Internal {


class Qt4SymbianTarget : public Qt4BaseTarget
{
    friend class Qt4SymbianTargetFactory; // for from Map
    Q_OBJECT
public:
    explicit Qt4SymbianTarget(Qt4Project *parent, const QString &id);
    virtual ~Qt4SymbianTarget();

    Internal::Qt4BuildConfigurationFactory *buildConfigurationFactory() const;
    ProjectExplorer::DeployConfigurationFactory *deployConfigurationFactory() const;

    QList<ProjectExplorer::ToolChainType> filterToolChainTypes(const QList<ProjectExplorer::ToolChainType> &candidates) const;
    ProjectExplorer::ToolChainType preferredToolChainType(const QList<ProjectExplorer::ToolChainType> &candidates) const;

    QString defaultBuildDirectory() const;

    void createApplicationProFiles();
    virtual QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Node *n);

    static QString defaultDisplayName(const QString &id);
    static QIcon iconForId(const QString &id);
private:
     bool isSymbianConnectionAvailable(QString &tooltipText);

private slots:
    void onAddedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void slotUpdateDeviceInformation();
    void updateToolTipAndIcon();

private:
    const QPixmap m_connectedPixmap;
    const QPixmap m_disconnectedPixmap;

    Internal::Qt4BuildConfigurationFactory *m_buildConfigurationFactory;
    ProjectExplorer::DeployConfigurationFactory *m_deployConfigurationFactory;
};
} // namespace Internal
} // namespace Qt4ProjectManager
#endif // QT4SYMBIANTARGET_H

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

#ifndef QT4MAEMOTARGET_H
#define QT4MAEMOTARGET_H

#include "qt4target.h"

namespace Qt4ProjectManager {
class Qt4Project;
namespace Internal {
class Qt4MaemoDeployConfigurationFactory;

class Qt4MaemoTarget : public Qt4BaseTarget
{
    friend class Qt4MaemoTargetFactory;
    Q_OBJECT
public:
    explicit Qt4MaemoTarget(Qt4Project *parent, const QString &id);
    virtual ~Qt4MaemoTarget();

    Internal::Qt4BuildConfigurationFactory *buildConfigurationFactory() const;
    ProjectExplorer::DeployConfigurationFactory *deployConfigurationFactory() const;

    QString defaultBuildDirectory() const;

    void createApplicationProFiles();

    static QString defaultDisplayName();

private:
    Internal::Qt4BuildConfigurationFactory *m_buildConfigurationFactory;
    Internal::Qt4MaemoDeployConfigurationFactory *m_deployConfigurationFactory;
};
}
}

#endif // QT4MAEMOTARGET_H

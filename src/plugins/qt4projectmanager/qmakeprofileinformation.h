/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef QT4PM_QMAKEPROFILEINFORMATION_H
#define QT4PM_QMAKEPROFILEINFORMATION_H

#include "qt4projectmanager_global.h"

#include <projectexplorer/profilemanager.h>

namespace Qt4ProjectManager {

class QT4PROJECTMANAGER_EXPORT QmakeProfileInformation : public ProjectExplorer::ProfileInformation
{
    Q_OBJECT

public:
    QmakeProfileInformation();

    Core::Id dataId() const;

    unsigned int priority() const; // the higher the closer to the top.

    QVariant defaultValue(ProjectExplorer::Profile *p) const;

    QList<ProjectExplorer::Task> validate(ProjectExplorer::Profile *p) const;

    ProjectExplorer::ProfileConfigWidget *createConfigWidget(ProjectExplorer::Profile *p) const;

    ItemList toUserOutput(ProjectExplorer::Profile *p) const;

    static void setMkspec(ProjectExplorer::Profile *p, const Utils::FileName &fn);
    static Utils::FileName mkspec(const ProjectExplorer::Profile *p);
    static Utils::FileName effectiveMkspec(const ProjectExplorer::Profile *p);
    static Utils::FileName defaultMkspec(const ProjectExplorer::Profile *p);

};

} // namespace Qt4ProjectManager

#endif // QT4PM_QMAKEPROFILEINFORMATION_H

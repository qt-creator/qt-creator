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

#ifndef QMLPROJECTMANAGER_H
#define QMLPROJECTMANAGER_H

#include <projectexplorer/iprojectmanager.h>
#include <coreplugin/icontext.h>

namespace QmlProjectManager {

class QmlProject;

namespace Internal {

class Manager: public ProjectExplorer::IProjectManager
{
    Q_OBJECT

public:
    Manager();

    virtual QString mimeType() const;
    virtual ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString);

    void notifyChanged(const QString &fileName);

    void registerProject(QmlProject *project);
    void unregisterProject(QmlProject *project);

private:
    QList<QmlProject *> m_projects;
};

} // namespace Internal
} // namespace QmlProjectManager

#endif // QMLPROJECTMANAGER_H

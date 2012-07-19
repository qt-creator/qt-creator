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

#ifndef PROJECTMANAGERINTERFACE_H
#define PROJECTMANAGERINTERFACE_H

#include "projectexplorer_export.h"

#include <QObject>

namespace Core {
class Context;
}
namespace ProjectExplorer {

class Project;

class PROJECTEXPLORER_EXPORT IProjectManager
    : public QObject
{
    Q_OBJECT

public:
    IProjectManager() {}

    virtual QString mimeType() const = 0;
    virtual Project *openProject(const QString &fileName, QString *errorString) = 0;
};

} // namespace ProjectExplorer

#endif //PROJECTMANAGERINTERFACE_H

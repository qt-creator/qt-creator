/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef PROJECTMANAGERINTERFACE_H
#define PROJECTMANAGERINTERFACE_H

#include "projectexplorer_export.h"

#include <QtCore/QObject>

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

    virtual Core::Context projectContext() const = 0; //TODO move into project
    virtual Core::Context projectLanguage() const = 0; //TODO move into project

    virtual QString mimeType() const = 0;
    virtual Project *openProject(const QString &fileName) = 0;
};

} // namespace ProjectExplorer

#endif //PROJECTMANAGERINTERFACE_H

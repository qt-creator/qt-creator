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

#ifndef IPROJECTPROPERTIES_H
#define IPROJECTPROPERTIES_H

#include "projectexplorer_export.h"

#include <QtCore/QObject>

QT_FORWARD_DECLARE_CLASS(QIcon)

namespace ProjectExplorer {
class Project;
class Target;

namespace Constants {
    const int PANEL_LEFT_MARGIN = 70;
}

class PROJECTEXPLORER_EXPORT IPropertiesPanel
{
public:
    IPropertiesPanel()
    { }
    virtual ~IPropertiesPanel()
    { }

    virtual QString displayName() const = 0;
    virtual QIcon icon() const = 0;
    virtual QWidget *widget() const = 0;
};

class PROJECTEXPLORER_EXPORT IPanelFactory : public QObject
{
    Q_OBJECT
public:
    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
};

class PROJECTEXPLORER_EXPORT IProjectPanelFactory : public IPanelFactory
{
    Q_OBJECT
public:
    virtual bool supports(Project *project) = 0;
    virtual IPropertiesPanel *createPanel(Project *project) = 0;
};

class PROJECTEXPLORER_EXPORT ITargetPanelFactory : public IPanelFactory
{
    Q_OBJECT
public:
    virtual bool supports(Target *target) = 0;
    virtual IPropertiesPanel *createPanel(Target *target) = 0;
};

} // namespace ProjectExplorer

#endif // IPROJECTPROPERTIES_H

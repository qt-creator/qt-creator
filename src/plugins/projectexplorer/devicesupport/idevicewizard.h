/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
#ifndef IDEVICEWIZARD_H
#define IDEVICEWIZARD_H

#include "idevice.h"
#include <projectexplorer/projectexplorer_export.h>

#include <QWizard>

namespace ProjectExplorer {

/*!
  \class ProjectExplorer::IDeviceWizard

  \brief Provides an interface for wizards creating an IDevice subclass.

  A class implementing this interface is a wizard whose final result is
  an \c IDevice object. The wizard will be started when the user chooses the
  "Add..." action from the "Devices" options page.
*/
class PROJECTEXPLORER_EXPORT IDeviceWizard : public QWizard
{
    Q_OBJECT

public:
    virtual IDevice::Ptr device() = 0;

protected:
    IDeviceWizard(QWidget *parent) : QWizard(parent) { }
};

} // namespace ProjectExplorer

#endif // IDEVICEWIZARD_H

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
#ifndef IDEVICEWIDGET_H
#define IDEVICEWIDGET_H

#include "idevice.h"
#include <projectexplorer/projectexplorer_export.h>

#include <QWidget>

namespace ProjectExplorer {

/*!
 \class ProjectExplorer::IDeviceWidget

 \brief Provides an interface for the widget configuring an IDevice.

 A class implementing this interface will display a widget in the configuration
 options page "Devices".
 It is used to let the user configure a particular device.
*/

class PROJECTEXPLORER_EXPORT IDeviceWidget : public QWidget
{
    Q_OBJECT
public:

    /*!
     * \brief Ensures that all changes in the UI are propagated to the device object.
     *
     * If the device is always updated right when the change happens, the implementation of
     * this function can be empty. Note, however, that you cannot generally rely on the
     * QLineEdit::editingFinished() signal being emitted on time if some button in the dialog is
     * clicked (e.g. "Apply"). So if you have any handlers for line edit changes, they should
     * probably be called here.
     */
    virtual void updateDeviceFromUi() = 0;

protected:
    IDeviceWidget(const IDevice::Ptr &device, QWidget *parent = 0)
        : QWidget(parent), m_device(device) {}

    IDevice::Ptr device() const { return m_device; }

private:
    IDevice::Ptr m_device;
};

} // namespace ProjectExplorer

#endif // IDEVICEWIDGET_H

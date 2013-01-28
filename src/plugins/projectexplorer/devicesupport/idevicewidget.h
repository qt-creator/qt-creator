/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
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

/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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

#ifndef DEVICEPROCESSESDIALOG_H
#define DEVICEPROCESSESDIALOG_H

#include "../projectexplorer_export.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>

#include <QDialog>

namespace ProjectExplorer {

class KitChooser;

namespace Internal { class DeviceProcessesDialogPrivate; }

class PROJECTEXPLORER_EXPORT DeviceProcessesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceProcessesDialog(QWidget *parent = 0);
    ~DeviceProcessesDialog();
    void addAcceptButton(const QString &label);
    void addCloseButton();

    void setDevice(const IDevice::ConstPtr &device);
    void showAllDevices();
    DeviceProcess currentProcess() const;
    KitChooser *kitChooser() const;
    void logMessage(const QString &line);

protected:
    DeviceProcessesDialog(KitChooser *chooser, QWidget *parent);

private:
    void setKitVisible(bool);

    Internal::DeviceProcessesDialogPrivate * const d;
};

} // namespace RemoteLinux

#endif // REMOTELINUXPROCESSESDIALOG_H

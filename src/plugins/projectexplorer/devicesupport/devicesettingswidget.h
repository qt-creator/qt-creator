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

#ifndef DEVICESETTINGSWIDGET_H
#define DEVICESETTINGSWIDGET_H

#include "devicesupport/idevice.h"

#include <QList>
#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QSignalMapper;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {
class IDevice;
class DeviceManager;
class DeviceManagerModel;
class IDeviceWidget;

namespace Internal {
namespace Ui { class DeviceSettingsWidget; }
class NameValidator;

class DeviceSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    DeviceSettingsWidget(QWidget *parent);
    ~DeviceSettingsWidget();

    void saveSettings();
    QString searchKeywords() const;

private slots:
    void handleDeviceUpdated(Core::Id id);
    void currentDeviceChanged(int index);
    void addDevice();
    void removeDevice();
    void deviceNameEditingFinished();
    void setDefaultDevice();
    void handleAdditionalActionRequest(int actionId);
    void handleProcessListRequested();

private:
    void initGui();
    void displayCurrent();
    void setDeviceInfoWidgetsEnabled(bool enable);
    IDevice::ConstPtr currentDevice() const;
    int currentIndex() const;
    void clearDetails();
    QString parseTestOutput();
    void fillInValues();
    void updateDeviceFromUi();

    Ui::DeviceSettingsWidget *m_ui;
    DeviceManager * const m_deviceManager;
    DeviceManagerModel * const m_deviceManagerModel;
    NameValidator * const m_nameValidator;
    QList<QPushButton *> m_additionalActionButtons;
    QSignalMapper * const m_additionalActionsMapper;
    IDeviceWidget *m_configWidget;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // DEVICESETTINGSWIDGET_H

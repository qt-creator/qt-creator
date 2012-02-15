/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef LINUXDEVICECONFIGURATIONSSETTINGSWIDGET_H
#define LINUXDEVICECONFIGURATIONSSETTINGSWIDGET_H

#include <QList>
#include <QScopedPointer>
#include <QString>
#include <QPushButton>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QSignalMapper;
QT_END_NAMESPACE

namespace RemoteLinux {
class ILinuxDeviceConfigurationFactory;
class LinuxDeviceConfiguration;
class LinuxDeviceConfigurations;
class ILinuxDeviceConfigurationWidget;

namespace Internal {
namespace Ui { class LinuxDeviceConfigurationsSettingsWidget; }
class NameValidator;

class LinuxDeviceConfigurationsSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    LinuxDeviceConfigurationsSettingsWidget(QWidget *parent);
    ~LinuxDeviceConfigurationsSettingsWidget();

    void saveSettings();
    QString searchKeywords() const;

private slots:
    void currentConfigChanged(int index);
    void addConfig();
    void deleteConfig();
    void configNameEditingFinished();
    void setDefaultDevice();

    void showGenerateSshKeyDialog();

    void handleAdditionalActionRequest(const QString &actionId);

private:
    void initGui();
    void displayCurrent();
    QSharedPointer<const LinuxDeviceConfiguration> currentConfig() const;
    int currentIndex() const;
    void clearDetails();
    QString parseTestOutput();
    void fillInValues();
    const ILinuxDeviceConfigurationFactory *factoryForCurrentConfig() const;

    Ui::LinuxDeviceConfigurationsSettingsWidget *m_ui;
    const QScopedPointer<LinuxDeviceConfigurations> m_devConfigs;
    NameValidator * const m_nameValidator;
    bool m_saveSettingsRequested;
    QList<QPushButton *> m_additionalActionButtons;
    QSignalMapper * const m_additionalActionsMapper;
    ILinuxDeviceConfigurationWidget *m_configWidget;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // LINUXDEVICECONFIGURATIONSSETTINGSWIDGET_H

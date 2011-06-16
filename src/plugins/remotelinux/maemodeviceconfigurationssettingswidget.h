/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef MAEMODEVICECONFIGURATIONSSETTINGSWIDGET_H
#define MAEMODEVICECONFIGURATIONSSETTINGSWIDGET_H

#include <QtCore/QList>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>
#include <QtGui/QPushButton>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QSignalMapper;

class Ui_MaemoDeviceConfigurationsSettingsWidget;
QT_END_NAMESPACE

namespace RemoteLinux {
class ILinuxDeviceConfigurationFactory;
class LinuxDeviceConfiguration;

namespace Internal {

class NameValidator;
class LinuxDeviceConfigurations;

class MaemoDeviceConfigurationsSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    MaemoDeviceConfigurationsSettingsWidget(QWidget *parent);
    ~MaemoDeviceConfigurationsSettingsWidget();

    void saveSettings();
    QString searchKeywords() const;

private slots:
    void currentConfigChanged(int index);
    void addConfig();
    void deleteConfig();
    void configNameEditingFinished();
    void authenticationTypeChanged();
    void hostNameEditingFinished();
    void sshPortEditingFinished();
    void timeoutEditingFinished();
    void userNameEditingFinished();
    void passwordEditingFinished();
    void keyFileEditingFinished();
    void showPassword(bool showClearText);
    void handleFreePortsChanged();
    void setDefaultKeyFilePath();
    void setDefaultDevice();

    void showGenerateSshKeyDialog();
    void setPrivateKey(const QString &path);

    void handleAdditionalActionRequest(const QString &actionId);

private:
    void initGui();
    void displayCurrent();
    QSharedPointer<const LinuxDeviceConfiguration> currentConfig() const;
    int currentIndex() const;
    void clearDetails();
    QString parseTestOutput();
    void fillInValues();
    void updatePortsWarningLabel();
    const ILinuxDeviceConfigurationFactory *factoryForCurrentConfig() const;

    Ui_MaemoDeviceConfigurationsSettingsWidget *m_ui;
    const QScopedPointer<LinuxDeviceConfigurations> m_devConfigs;
    NameValidator * const m_nameValidator;
    bool m_saveSettingsRequested;
    QList<QPushButton *> m_additionalActionButtons;
    QSignalMapper * const m_additionalActionsMapper;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMODEVICECONFIGURATIONSSETTINGSWIDGET_H

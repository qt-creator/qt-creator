/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAEMOSETTINGSWIDGET_H
#define MAEMOSETTINGSWIDGET_H

#include "maemodeviceconfigurations.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;

class Ui_maemoSettingsWidget;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class MaemoSshRunner;
class MaemoSshDeployer;
class NameValidator;
class PortAndTimeoutValidator;

class MaemoSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    MaemoSettingsWidget(QWidget *parent);
    ~MaemoSettingsWidget();
    void saveSettings();
private slots:
    void selectionChanged();
    void addConfig();
    void deleteConfig();
    void configNameEditingFinished();
    void deviceTypeChanged();
    void authenticationTypeChanged();
    void hostNameEditingFinished();
    void sshPortEditingFinished();
    void gdbServerPortEditingFinished();
    void timeoutEditingFinished();
    void userNameEditingFinished();
    void passwordEditingFinished();
    void keyFileEditingFinished();

    // For configuration testing.
    void testConfig();
    void processSshOutput(const QString &data);
    void handleTestThreadFinished();
    void stopConfigTest();

    // For key deploying.
    void deployKey();
    void handleDeployThreadFinished();
    void stopDeploying();

private:
    void initGui();
    void display(const MaemoDeviceConfig &devConfig);
    MaemoDeviceConfig &currentConfig();
    void setPortOrTimeout(const QLineEdit *lineEdit, int &confVal,
                          PortAndTimeoutValidator *validator);
    void clearDetails();
    QString parseTestOutput();

    Ui_maemoSettingsWidget *m_ui;
    QList<MaemoDeviceConfig> m_devConfs;
    NameValidator * const m_nameValidator;
    PortAndTimeoutValidator * const m_sshPortValidator;
    PortAndTimeoutValidator * const m_gdbServerPortValidator;
    PortAndTimeoutValidator * const m_timeoutValidator;
    MaemoSshRunner *m_deviceTester;
    MaemoSshDeployer *m_keyDeployer;
    QString m_deviceTestOutput;
    QString m_defaultTestOutput;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOSETTINGSWIDGET_H

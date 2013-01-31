/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#ifndef QNX_INTERNAL_BLACKBERRYDEVICECONFIGURATIONWIZARDPAGES_H
#define QNX_INTERNAL_BLACKBERRYDEVICECONFIGURATIONWIZARDPAGES_H

#include <projectexplorer/devicesupport/idevice.h>

#include <QWizardPage>

namespace QSsh {
class SshKeyGenerator;
}

namespace Qnx {
namespace Internal {
namespace Ui {
class BlackBerryDeviceConfigurationWizardSetupPage;
class BlackBerryDeviceConfigurationWizardSshKeyPage;
}

class BlackBerryDeviceConfigurationWizardSetupPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit BlackBerryDeviceConfigurationWizardSetupPage(QWidget *parent = 0);
    ~BlackBerryDeviceConfigurationWizardSetupPage();

    void initializePage();
    bool isComplete() const;

    QString deviceName() const;
    QString hostName() const;
    QString password() const;
    QString debugToken() const;
    ProjectExplorer::IDevice::MachineType machineType() const;

private slots:
    void handleMachineTypeChanged();

private:
    Ui::BlackBerryDeviceConfigurationWizardSetupPage *m_ui;
};

class BlackBerryDeviceConfigurationWizardSshKeyPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit BlackBerryDeviceConfigurationWizardSshKeyPage(QWidget *parent = 0);
    ~BlackBerryDeviceConfigurationWizardSshKeyPage();

    void initializePage();
    bool isComplete() const;

    QString privateKey() const;
    QString publicKey() const;
    bool isGenerated() const;

    QSsh::SshKeyGenerator *keyGenerator() const;

private slots:
    void findMatchingPublicKey(const QString &privateKeyPath);
    void generateSshKey();

private:
    void saveKeys();

    Ui::BlackBerryDeviceConfigurationWizardSshKeyPage *m_ui;

    QSsh::SshKeyGenerator *m_keyGen;
    bool m_isGenerated;
};

class BlackBerryDeviceConfigurationWizardFinalPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit BlackBerryDeviceConfigurationWizardFinalPage(QWidget *parent = 0);
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYDEVICECONFIGURATIONWIZARDPAGES_H

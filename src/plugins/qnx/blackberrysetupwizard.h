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

#ifndef BLACKBERRYSETUPWIZARD_H
#define BLACKBERRYSETUPWIZARD_H

#include <QWizard>
#include <QList>
#include <QByteArray>

#include <projectexplorer/devicesupport/idevice.h>

namespace QSsh {
class SshKeyGenerator;
}

namespace Qnx {
namespace Internal {

class BlackBerrySetupWizardWelcomePage;
class BlackBerrySetupWizardNdkPage;
class BlackBerrySetupWizardKeysPage;
class BlackBerrySetupWizardDevicePage;
class BlackBerrySetupWizardFinishPage;
class BlackBerryCsjRegistrar;
class BlackBerryCertificate;
class BlackBerryDeviceInformation;
class BlackBerryDebugTokenRequester;
class BlackBerryDebugTokenUploader;

class BlackBerrySetupWizard : public QWizard
{
    Q_OBJECT
public:
    explicit BlackBerrySetupWizard(QWidget *parent = 0);
    virtual ~BlackBerrySetupWizard();

public slots:
    void accept();

signals:
    void stepFinished();

private slots:
    void processNextStep();
    void deviceInfoFinished(int status);
    void registrarFinished(int status, const QString &errorString);
    void certificateCreated(int status);
    void debugTokenArrived(int status);
    void uploaderFinished(int status);
    void requestDevicePin();
    void createKeys();
    void generateDeveloperCertificate();
    void generateSshKeys();
    void requestDebugToken();
    void uploadDebugToken();
    void writeDeviceInformation();

private:
    enum PageId {
        WelcomePageId,
        NdkPageId,
        KeysPageId,
        DevicePageId,
        FinishPageId
    };

    struct Step {
        QByteArray slot;
        QString message;
    };

    void registerStep(const QByteArray &slot, const QString &message);
    void setBusy(bool busy);
    void cleanupFiles() const;
    void reset();

    QString privateKeyPath() const;
    QString publicKeyPath() const;
    QString deviceName() const;
    QString storeLocation() const;
    QString rdkPath() const;
    QString pbdtPath() const;
    QString csjPin() const;
    QString password() const;
    QString devicePassword() const;
    QString hostName() const;

    bool isPhysicalDevice() const;

    ProjectExplorer::IDevice::Ptr device();

    BlackBerrySetupWizardWelcomePage *m_welcomePage;
    BlackBerrySetupWizardNdkPage *m_ndkPage;
    BlackBerrySetupWizardKeysPage *m_keysPage;
    BlackBerrySetupWizardDevicePage *m_devicePage;
    BlackBerrySetupWizardFinishPage *m_finishPage;

    BlackBerryCsjRegistrar *m_registrar;
    BlackBerryCertificate *m_certificate;
    BlackBerryDeviceInformation *m_deviceInfo;
    BlackBerryDebugTokenRequester *m_requester;
    BlackBerryDebugTokenUploader *m_uploader;

    QSsh::SshKeyGenerator *m_keyGenerator;

    QString m_devicePin;

    QList<Step*> m_stepList;

    int m_currentStep;
    int m_stepOffset;
};

} // namespace Qnx
} // namespace Internal

#endif // BLACKBERRYSETUPWIZARD_H

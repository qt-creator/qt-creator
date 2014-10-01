/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNX_INTERNAL_BLACKBERRYDEVICECONFIGURATIONWIZARDPAGES_H
#define QNX_INTERNAL_BLACKBERRYDEVICECONFIGURATIONWIZARDPAGES_H

#include "blackberrydevicelistdetector.h"

#include <projectexplorer/devicesupport/idevice.h>

#include <QWizardPage>
#include <QListWidgetItem>

namespace QSsh { class SshKeyGenerator; }

namespace Qnx {
namespace Internal {
namespace Ui {
class BlackBerryDeviceConfigurationWizardSetupPage;
class BlackBerryDeviceConfigurationWizardQueryPage;
class BlackBerryDeviceConfigurationWizardConfigPage;
}
class BlackBerrySshKeysGenerator;
class BlackBerryDeviceInformation;
class BlackBerrySigningUtils;

struct BlackBerryDeviceConfigurationWizardHolder {
    QString devicePin;
    QString deviceName;
    QString scmBundle;
    QString debugTokenAuthor;
    bool isSimulator;
    bool debugTokenValid;
    bool deviceInfoRetrieved;
    bool isProductionDevice;

    BlackBerryDeviceConfigurationWizardHolder()
        : isSimulator(false)
        , debugTokenValid(false)
        , deviceInfoRetrieved(false)
        , isProductionDevice(true) {}
};

class BlackBerryDeviceConfigurationWizardSetupPage : public QWizardPage
{
    Q_OBJECT
public:
    enum ItemKind {
        SpecifyManually, Autodetected, PleaseWait, Note
    };

    explicit BlackBerryDeviceConfigurationWizardSetupPage(QWidget *parent = 0);
    ~BlackBerryDeviceConfigurationWizardSetupPage();

    void initializePage();
    bool isComplete() const;

    QString hostName() const;
    QString password() const;
private slots:
    void onDeviceSelectionChanged();
    void onDeviceDetected(const QString &deviceName, const QString &hostName, bool isSimulator);
    void onDeviceListDetectorFinished();

private:
    void refreshDeviceList();
    QListWidgetItem *createDeviceListItem(const QString &displayName, ItemKind itemKind) const;
    QListWidgetItem *findDeviceListItem(ItemKind itemKind) const;

    Ui::BlackBerryDeviceConfigurationWizardSetupPage *m_ui;
    BlackBerryDeviceListDetector *m_deviceListDetector;
};

class BlackBerryDeviceConfigurationWizardQueryPage : public QWizardPage
{
    Q_OBJECT
    enum QueryState
    {
        Querying = 0, GeneratingSshKey, Done
    };

public:
    explicit BlackBerryDeviceConfigurationWizardQueryPage(BlackBerryDeviceConfigurationWizardHolder &holder, QWidget *parent = 0);
    ~BlackBerryDeviceConfigurationWizardQueryPage();

    void initializePage();
    bool isComplete() const;

private slots:
    void processQueryFinished(int status);
    void sshKeysGenerationFailed(const QString &error);
    void processSshKeys(const QByteArray &privateKey, const QByteArray &publicKey);

private:
    void checkAndGenerateSSHKeys();
    void queryDone();
    void setState(QueryState state, const QString &message);

    Ui::BlackBerryDeviceConfigurationWizardQueryPage *m_ui;
    BlackBerryDeviceConfigurationWizardHolder &m_holder;
    BlackBerryDeviceInformation *m_deviceInformation;
    QueryState m_state;
};

class BlackBerryDeviceConfigurationWizardConfigPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit BlackBerryDeviceConfigurationWizardConfigPage(BlackBerryDeviceConfigurationWizardHolder &holder, QWidget *parent = 0);
    ~BlackBerryDeviceConfigurationWizardConfigPage();

    void initializePage();
    bool isComplete() const;

    QString configurationName() const;
    QString debugToken() const;
private slots:
    void generateDebugToken();
    void importDebugToken();

private:
    Ui::BlackBerryDeviceConfigurationWizardConfigPage *m_ui;
    BlackBerryDeviceConfigurationWizardHolder &m_holder;
    BlackBerrySigningUtils &m_utils;
};

class BlackBerryDeviceConfigurationWizardFinalPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit BlackBerryDeviceConfigurationWizardFinalPage(QWidget *parent = 0);
};

} // namespace Internal
} // namespace Qnx

Q_DECLARE_METATYPE(Qnx::Internal::BlackBerryDeviceConfigurationWizardSetupPage::ItemKind)

#endif // QNX_INTERNAL_BLACKBERRYDEVICECONFIGURATIONWIZARDPAGES_H

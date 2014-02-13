/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
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

#include "blackberrysetupwidget.h"

#include "blackberryconfigurationmanager.h"
#include "blackberryconfiguration.h"
#include "blackberrysigningutils.h"
#include "blackberrydeviceconfigurationwizard.h"
#include "blackberryinstallwizard.h"
#include "blackberrycertificate.h"
#include "qnxconstants.h"

#include <projectexplorer/devicesupport/devicemanager.h>

#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDesktopServices>
#include <QWizard>
#include <QUrl>
#include <QMessageBox>

namespace Qnx {
namespace Internal {

SetupItem::SetupItem(const QString &desc, QWidget *parent)
: QFrame(parent)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(validate()));

    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    QVBoxLayout *frameLayout = new QVBoxLayout(this);

    QHBoxLayout *childLayout = new QHBoxLayout;
    frameLayout->addLayout(childLayout);

    m_icon = new QLabel;
    childLayout->addWidget(m_icon);

    m_label = new QLabel;
    m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    childLayout->addWidget(m_label);

    m_button = new QPushButton;
    childLayout->addWidget(m_button);
    QObject::connect(m_button, SIGNAL(clicked()), this, SLOT(onFixPressed()));

    if (!desc.isEmpty()) {
        m_desc = new QLabel(desc);
        m_desc->setWordWrap(true);
        QFont font = m_desc->font();
        font.setItalic(true);
        m_desc->setFont(font);
        frameLayout->addWidget(m_desc);
    }
}

void SetupItem::set(Status status, const QString &message, const QString &fixText)
{
    const char *icon;
    switch (status) {
    case Ok:
        icon = Qnx::Constants::QNX_OK_ICON;
        break;
    case Info:
        icon = Qnx::Constants::QNX_INFO_ICON;
        break;
    case Warning:
        icon = Qnx::Constants::QNX_WARNING_ICON;
        break;
    case Error:
        icon = Qnx::Constants::QNX_ERROR_ICON;
        break;
    }
    m_icon->setPixmap(QPixmap(QLatin1String(icon)));
    m_label->setText(message);
    m_button->setVisible(!fixText.isEmpty());
    m_button->setText(fixText);
}

void SetupItem::onFixPressed()
{
    fix();
    validate();
}

void SetupItem::validateLater()
{
    // BlackBerryConfigurationManager.settingsChanged and DeviceManager.updated signals
    // may be emitted multiple times during the same event handling. This would result in multiple
    // validation() calls even through just one is needed.
    // QTimer allows to merge those multiple signal emits into a single validate() call.
    m_timer.start();
}

APILevelSetupItem::APILevelSetupItem(QWidget *parent)
: SetupItem(tr("API Level defines kits, Qt versions, compilers, debuggers needed"
               " for building BlackBerry applications."), parent)
{
    validate();
    connect(&BlackBerryConfigurationManager::instance(), SIGNAL(settingsChanged()),
            this, SLOT(validateLater()));
}

void APILevelSetupItem::validate()
{
    FoundTypes found = resolvedFoundType();
    if (!found.testFlag(Any))
        set(Error, tr("No API Level is installed."), tr("Install"));
    else if (!found.testFlag(Valid))
        set(Error, tr("No valid API Level is installed."), tr("Install"));
    else if (!found.testFlag(Active))
        set(Error, tr("Valid API Level is not activated."), tr("Activate"));
    else if (!found.testFlag(V_10_2))
        set(Warning, tr("Valid API Level 10.2 or newer is not installed."), tr("Install"));
    else if (!found.testFlag(V_10_2_AS_DEFAULT))
        set(Warning, tr("Valid API Level 10.2 or newer is not set as default."), tr("Set"));
    else
        set(Ok, tr("API Level is configured."));
    // TODO: should we check something more e.g. BB10 kits are valid?
}

void APILevelSetupItem::fix()
{
    FoundTypes found = resolvedFoundType();
    if (!found.testFlag(Any) || !found.testFlag(Valid)) {
        installAPILevel();
    } else if (!found.testFlag(Active)) {
        foreach (BlackBerryConfiguration *config,
                BlackBerryConfigurationManager::instance().configurations()) {
            if (config->isValid() && !config->isActive()) {
                config->activate();
                break;
            }
        }
    } else if (!found.testFlag(V_10_2)) {
        // TODO: install filter for 10.2 only
        installAPILevel();
    } else if (!found.testFlag(V_10_2_AS_DEFAULT)) {
        BlackBerryConfigurationManager::instance().setDefaultConfiguration(0);
    }
}

APILevelSetupItem::FoundTypes APILevelSetupItem::resolvedFoundType()
{
    FoundTypes found;

    // TODO: for now, all Trunk versions are understood as 10.2 compliant
    BlackBerryVersionNumber version_10_2(QLatin1String("10.2.0.0"));
    foreach (BlackBerryConfiguration *config,
            BlackBerryConfigurationManager::instance().configurations()) {
        found |= Any;
        if (config->isValid()) {
            found |= Valid;
            if (config->isActive())
                found |= Active;
            if (config->version() > version_10_2)
                found |= V_10_2;
        }
    }

    BlackBerryConfiguration *config = BlackBerryConfigurationManager::instance().defaultConfiguration();
    if (config && config->version() > version_10_2)
        found |= V_10_2_AS_DEFAULT;

    return found;
}

void APILevelSetupItem::installAPILevel()
{
    BlackBerryInstallWizard wizard(
            BlackBerryInstallerDataHandler::InstallMode,
            BlackBerryInstallerDataHandler::ApiLevel, QString(), this);
    connect(&wizard, SIGNAL(processFinished()), this, SLOT(handleInstallationFinished()));
    wizard.exec();
}

void APILevelSetupItem::handleInstallationFinished()
{
    // manually-added API Levels are automatically registered by BlackBerryInstallWizard
    // auto-detected API Levels needs to reloaded explicitly
    BlackBerryConfigurationManager::instance().loadAutoDetectedConfigurations();
    validate();
}

SigningKeysSetupItem::SigningKeysSetupItem(QWidget *parent)
: SetupItem(tr("Signing keys are needed for signing BlackBerry applications"
                       " and managing debug tokens."), parent)
{
    validate();
    connect(&BlackBerrySigningUtils::instance(), SIGNAL(defaultCertificateLoaded(int)),
            this, SLOT(validate()));
}

void SigningKeysSetupItem::validate()
{
    BlackBerrySigningUtils &utils = BlackBerrySigningUtils::instance();
    if (utils.hasLegacyKeys())
        set(Error, tr("Found legacy BlackBerry signing keys."), tr("Update"));
    else if (!utils.hasRegisteredKeys())
        set(Error, tr("Cannot find BlackBerry signing keys."), tr("Request"));
    else if (!QFileInfo(BlackBerryConfigurationManager::instance().defaultKeystorePath()).exists())
        set(Error, tr("Cannot find developer certificate."), tr("Create"));
    else if (utils.defaultCertificateOpeningStatus() != BlackBerrySigningUtils::Opened)
        set(Info, tr("Developer certificate is not opened."), tr("Open"));
    else
        set(Ok, tr("Signing keys are ready to use."));
}

void SigningKeysSetupItem::fix()
{
    BlackBerrySigningUtils &utils = BlackBerrySigningUtils::instance();
    if (utils.hasLegacyKeys()) {
        QDesktopServices::openUrl(QUrl(QLatin1String(Qnx::Constants::QNX_LEGACY_KEYS_URL)));
    } else if (!utils.hasRegisteredKeys()) {
        QDesktopServices::openUrl(QUrl(QLatin1String(Qnx::Constants::QNX_REGISTER_KEYS_URL)));
    } else if (!QFileInfo(BlackBerryConfigurationManager::instance().defaultKeystorePath()).exists()) {
        set(Info, tr("Opening certificate..."));
        utils.createCertificate();
    } else if (utils.defaultCertificateOpeningStatus() != BlackBerrySigningUtils::Opened) {
        connect(&utils, SIGNAL(defaultCertificateLoaded(int)), this, SLOT(defaultCertificateLoaded(int)));
        utils.openDefaultCertificate(this);
    }
}

void SigningKeysSetupItem::defaultCertificateLoaded(int status)
{
    BlackBerrySigningUtils &utils = BlackBerrySigningUtils::instance();
    disconnect(&utils, SIGNAL(defaultCertificateLoaded(int)), this, SLOT(defaultCertificateLoaded(int)));
    switch (status) {
    case BlackBerryCertificate::Success:
        // handled by the connect in ctor already
        break;
    case BlackBerryCertificate::WrongPassword:
        QMessageBox::critical(this, tr("Qt Creator"), tr("Invalid certificate password."));
        break;
    case BlackBerryCertificate::Busy:
    case BlackBerryCertificate::InvalidOutputFormat:
    case BlackBerryCertificate::Error:
        QMessageBox::critical(this, tr("Qt Creator"), tr("Error loading certificate."));
        break;
    }
}

DeviceSetupItem::DeviceSetupItem(QWidget *parent)
: SetupItem(tr("BlackBerry 10 device or simulator is needed for running BlackBerry applications."),
            parent)
{
    validate();
    connect(ProjectExplorer::DeviceManager::instance(), SIGNAL(updated()),
            this, SLOT(validateLater()));
}

void DeviceSetupItem::validate()
{
    bool found = false;
    ProjectExplorer::DeviceManager *manager = ProjectExplorer::DeviceManager::instance();
    for (int i = 0; i < manager->deviceCount(); i ++) {
        ProjectExplorer::IDevice::ConstPtr device = manager->deviceAt(i);
        if (device->type() == Constants::QNX_BB_OS_TYPE) {
            found = true;
            break;
        }
    }
    if (!found)
        set(Error, tr("No BlackBerry 10 device or simulator is registered."), tr("Add"));
    else
        set(Ok, tr("BlackBerry 10 device or simulator is registered."));
    // TODO: check for existence of an API Level matching a device?
}

void DeviceSetupItem::fix()
{
    BlackBerryDeviceConfigurationWizard wizard(this);
    if (wizard.exec() == QDialog::Accepted)
        ProjectExplorer::DeviceManager::instance()->addDevice(wizard.device());
}

BlackBerrySetupWidget::BlackBerrySetupWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    layout->addWidget(new APILevelSetupItem);
    layout->addWidget(new SigningKeysSetupItem);
    layout->addWidget(new DeviceSetupItem);

    layout->addStretch();

    QLabel *howTo = new QLabel;
    howTo->setTextFormat(Qt::RichText);
    howTo->setTextInteractionFlags(Qt::TextBrowserInteraction);
    howTo->setOpenExternalLinks(true);
    howTo->setText(tr("<a href=\"%1\">How to Setup Qt Creator for BlackBerry 10 development</a>")
                   .arg(QLatin1String(Qnx::Constants::QNX_BLACKBERRY_SETUP_URL)));
    layout->addWidget(howTo);
}

} // namespace Internal
} // namespace Qnx

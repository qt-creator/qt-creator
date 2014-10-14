/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
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

#include "blackberrysetupwizardpages.h"
#include "blackberryndksettingswidget.h"
#include "blackberrysigningutils.h"
#include "ui_blackberrysetupwizardkeyspage.h"
#include "ui_blackberrysetupwizardcertificatepage.h"
#include "ui_blackberrysetupwizarddevicepage.h"
#include "ui_blackberrysetupwizardfinishpage.h"

#include <QVBoxLayout>
#include <QFileInfo>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QAbstractButton>
#include <QDesktopServices>
#include <QUrl>

#include <QDebug>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerrySetupWizardWelcomePage::BlackBerrySetupWizardWelcomePage(QWidget *parent) :
    QWizardPage(parent)
{
    const QString welcomeMessage =
        tr("Welcome to the BlackBerry Development "
           "Environment Setup Wizard.\nThis wizard will guide you through "
           "the essential steps to deploy a ready-to-go development environment "
           "for BlackBerry 10 devices.");

    setTitle(tr("BlackBerry Development Environment Setup"));

    QLabel *label = new QLabel(this);
    label->setWordWrap(true);
    label->setText(welcomeMessage);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addStretch();
    layout->addWidget(label);
    layout->addStretch();

    setLayout(layout);
}

//-----------------------------------------------------------------------------

BlackBerrySetupWizardNdkPage::BlackBerrySetupWizardNdkPage(QWidget *parent) :
    QWizardPage(parent),
    m_widget(0)
{
    setTitle(tr("Configure the NDK Path"));

    m_widget = new BlackBerryNDKSettingsWidget(this);
    m_widget->setWizardMessageVisible(false);

    connect(m_widget, SIGNAL(targetsUpdated()), this, SIGNAL(completeChanged()));
    connect(m_widget, SIGNAL(targetsUpdated()), this, SIGNAL(targetsUpdated()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_widget);

    setLayout(layout);
}

BlackBerrySetupWizardNdkPage::~BlackBerrySetupWizardNdkPage()
{
}

bool BlackBerrySetupWizardNdkPage::isComplete() const
{
    return m_widget->hasActiveNdk();
}

//-----------------------------------------------------------------------------

BlackBerrySetupWizardKeysPage::BlackBerrySetupWizardKeysPage(QWidget *parent) :
    QWizardPage(parent),
    m_ui(0),
    m_complete(false)
{
    setTitle(tr("Setup Signing Keys"));

    initUi();
}

BlackBerrySetupWizardKeysPage::~BlackBerrySetupWizardKeysPage()
{
    delete m_ui;
    m_ui = 0;
}

void BlackBerrySetupWizardKeysPage::showKeysMessage(const QString &url)
{
    const QMessageBox::StandardButton button = QMessageBox::question(this,
            tr("Qt Creator"),
            tr("This wizard will be closed and you will be taken to the BlackBerry "
                "key request web page. Do you want to continue?"),
            QMessageBox::Yes | QMessageBox::No);

    if (button == QMessageBox::Yes) {
        QDesktopServices::openUrl(QUrl(url));
        wizard()->reject();
    }
}

bool BlackBerrySetupWizardKeysPage::isComplete() const
{
    return m_complete;
}

void BlackBerrySetupWizardKeysPage::initUi()
{
    BlackBerrySigningUtils &utils = BlackBerrySigningUtils::instance();

    m_ui = new Ui::BlackBerrySetupWizardKeysPage;
    m_ui->setupUi(this);

    if (utils.hasLegacyKeys()) {
        m_ui->linkLabel->setVisible(false);
        m_ui->legacyLabel->setVisible(true);
        m_ui->statusLabel->setVisible(false);

        setComplete(false);
    } else if (utils.hasRegisteredKeys()) {
        m_ui->linkLabel->setVisible(false);
        m_ui->legacyLabel->setVisible(false);
        m_ui->statusLabel->setVisible(true);

        setComplete(true);
    } else {
        m_ui->linkLabel->setVisible(true);
        m_ui->legacyLabel->setVisible(false);
        m_ui->statusLabel->setVisible(false);

        setComplete(false);
    }

    connect(m_ui->linkLabel, SIGNAL(linkActivated(QString)),
            this, SLOT(showKeysMessage(QString)));
    connect(m_ui->legacyLabel, SIGNAL(linkActivated(QString)),
            this, SLOT(showKeysMessage(QString)));
    connect(m_ui->helpLabel, SIGNAL(linkActivated(QString)),
            this, SLOT(showKeysMessage(QString)));
}

void BlackBerrySetupWizardKeysPage::setComplete(bool complete)
{
    if (m_complete != complete) {
        m_complete = complete;
        m_ui->linkLabel->setVisible(!complete);
        m_ui->statusLabel->setVisible(complete);
        emit completeChanged();
    }
}

//-----------------------------------------------------------------------------

const char BlackBerrySetupWizardCertificatePage::AuthorField[] = "CertificatePage::Author";
const char BlackBerrySetupWizardCertificatePage::PasswordField[] = "CertificatePage::Password";
const char BlackBerrySetupWizardCertificatePage::PasswordField2[] = "CertificatePage::Password2";

BlackBerrySetupWizardCertificatePage::BlackBerrySetupWizardCertificatePage(QWidget *parent)
    : QWizardPage(parent),
    m_ui(0),
    m_complete(false)
{
    setTitle(tr("Create Developer Certificate"));

    initUi();
}

bool BlackBerrySetupWizardCertificatePage::isComplete() const
{
    return m_complete;
}

void BlackBerrySetupWizardCertificatePage::validate()
{
    if (m_ui->author->text().isEmpty()
            || m_ui->password->text().isEmpty()
            || m_ui->password2->text().isEmpty()) {
        m_ui->status->clear();
        setComplete(false);
        return;
    }

    if (m_ui->password->text() != m_ui->password2->text()) {
        m_ui->status->setText(tr("The entered passwords do not match."));
        setComplete(false);
        return;
    }

    if (m_ui->password->text().size() < 6) {
        // TODO: Use tr() once string freeze is over
        m_ui->status->setText(QCoreApplication::translate("Qnx::Internal::BlackBerryCreateCertificateDialog", "Password must be at least 6 characters long."));
        setComplete(false);
        return;
    }

    m_ui->status->clear();
    setComplete(true);
}

void BlackBerrySetupWizardCertificatePage::checkBoxChanged(int state)
{
    if (state == Qt::Checked) {
        m_ui->password->setEchoMode(QLineEdit::Normal);
        m_ui->password2->setEchoMode(QLineEdit::Normal);
    } else {
        m_ui->password->setEchoMode(QLineEdit::Password);
        m_ui->password2->setEchoMode(QLineEdit::Password);
    }
}

void BlackBerrySetupWizardCertificatePage::setComplete(bool complete)
{
    if (m_complete != complete) {
        m_complete = complete;
        emit completeChanged();
    }
}

void BlackBerrySetupWizardCertificatePage::initUi()
{
    m_ui = new Ui::BlackBerrySetupWizardCertificatePage;
    m_ui->setupUi(this);
    m_ui->status->clear();

    connect(m_ui->author, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(m_ui->password, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(m_ui->password2, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(m_ui->showPassword, SIGNAL(stateChanged(int)),
            this, SLOT(checkBoxChanged(int)));

    registerField(QLatin1String(AuthorField) + QLatin1Char('*'), m_ui->author);
    registerField(QLatin1String(PasswordField) + QLatin1Char('*'), m_ui->password);
    registerField(QLatin1String(PasswordField2) + QLatin1Char('*'), m_ui->password2);
}

//-----------------------------------------------------------------------------

const char BlackBerrySetupWizardDevicePage::NameField[] = "DevicePage::Name";
const char BlackBerrySetupWizardDevicePage::IpAddressField[] = "DevicePage::IpAddress";
const char BlackBerrySetupWizardDevicePage::PasswordField[] = "DevicePage::PasswordField";
const char BlackBerrySetupWizardDevicePage::PhysicalDeviceField[] = "DevicePage::PhysicalDeviceField";


BlackBerrySetupWizardDevicePage::BlackBerrySetupWizardDevicePage(QWidget *parent)
    : QWizardPage(parent),
    m_ui(0)
{
    setTitle(tr("Configure BlackBerry Device Connection"));

    m_ui = new Ui::BlackBerrySetupWizardDevicePage;
    m_ui->setupUi(this);

    m_ui->deviceName->setText(tr("BlackBerry Device"));
    m_ui->ipAddress->setText(QLatin1String("169.254.0.1"));

    connect(m_ui->deviceName, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));
    connect(m_ui->ipAddress, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));
    connect(m_ui->password, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));
    connect(m_ui->physicalDevice, SIGNAL(toggled(bool)), this, SIGNAL(completeChanged()));

    registerField(QLatin1String(NameField) + QLatin1Char('*'), m_ui->deviceName);
    registerField(QLatin1String(IpAddressField) + QLatin1Char('*'), m_ui->ipAddress);
    registerField(QLatin1String(PasswordField), m_ui->password);
    registerField(QLatin1String(PhysicalDeviceField), m_ui->physicalDevice);
}

bool BlackBerrySetupWizardDevicePage::isComplete() const
{
    if (m_ui->deviceName->text().isEmpty() || m_ui->ipAddress->text().isEmpty())
        return false;

    const bool passwordMandatory = m_ui->physicalDevice->isChecked();

    if (passwordMandatory && m_ui->password->text().isEmpty())
        return false;

    return true;
}

//-----------------------------------------------------------------------------

BlackBerrySetupWizardFinishPage::BlackBerrySetupWizardFinishPage(QWidget *parent)
    : QWizardPage(parent),
    m_ui(0)
{
    setTitle(tr("Your environment is ready to be configured."));

    m_ui = new Ui::BlackBerrySetupWizardFinishPage;
    m_ui->setupUi(this);
    setProgress(QString(), -1);
}

void BlackBerrySetupWizardFinishPage::setProgress(const QString &status, int progress)
{
    if (progress < 0) {
        m_ui->progressBar->hide();
        m_ui->statusLabel->clear();
        return;
    } else if (!m_ui->progressBar->isVisible()) {
        m_ui->progressBar->show();
    }

    m_ui->statusLabel->setText(status);
    m_ui->progressBar->setValue(progress);
}

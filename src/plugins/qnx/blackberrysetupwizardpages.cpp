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

#include "blackberrysetupwizardpages.h"
#include "blackberryndksettingswidget.h"
#include "blackberryconfiguration.h"
#include "ui_blackberrysetupwizardkeyspage.h"
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
    m_widget->setRemoveButtonVisible(false);

    connect(m_widget, SIGNAL(sdkPathChanged(QString)), this, SIGNAL(completeChanged()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_widget);

    setLayout(layout);
}

BlackBerrySetupWizardNdkPage::~BlackBerrySetupWizardNdkPage()
{
}

bool BlackBerrySetupWizardNdkPage::isComplete() const
{
    return m_widget->hasValidSdkPath();
}

//-----------------------------------------------------------------------------

const char BlackBerrySetupWizardKeysPage::PbdtPathField[] = "KeysPage::PbdtPath";
const char BlackBerrySetupWizardKeysPage::RdkPathField[] = "KeysPage::RdkPath";
const char BlackBerrySetupWizardKeysPage::CsjPinField[] = "KeysPage::CsjPin";
const char BlackBerrySetupWizardKeysPage::PasswordField[] = "KeysPage::Password";
const char BlackBerrySetupWizardKeysPage::Password2Field[] = "KeysPage::Password2";

BlackBerrySetupWizardKeysPage::BlackBerrySetupWizardKeysPage(QWidget *parent) :
    QWizardPage(parent),
    m_ui(0),
    m_complete(false)
{
    setTitle(tr("Register Signing Keys"));

    initUi();
}

BlackBerrySetupWizardKeysPage::~BlackBerrySetupWizardKeysPage()
{
    delete m_ui;
    m_ui = 0;
}

void BlackBerrySetupWizardKeysPage::validateFields()
{
    if (!m_ui->pbdtPath->isValid()
            || !m_ui->rdkPath->isValid()
            || m_ui->csjPin->text().isEmpty()
            || m_ui->password->text().isEmpty()
            || m_ui->password2->text().isEmpty()) {
        m_ui->statusLabel->clear();
        setComplete(false);
        return;
    }

    if (m_ui->password->text() != m_ui->password2->text()) {
        m_ui->statusLabel->setText(tr("Passwords do not match."));
        setComplete(false);
        return;
    }

    m_ui->statusLabel->clear();

    setComplete(true);
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
    m_ui = new Ui::BlackBerrySetupWizardKeysPage;
    m_ui->setupUi(this);
    m_ui->statusLabel->clear();

    setupCsjPathChooser(m_ui->pbdtPath);
    setupCsjPathChooser(m_ui->rdkPath);

    connect(m_ui->pbdtPath, SIGNAL(changed(QString)),
            this, SLOT(csjAutoComplete(QString)));
    connect(m_ui->rdkPath, SIGNAL(changed(QString)),
            this, SLOT(csjAutoComplete(QString)));
    connect(m_ui->pbdtPath, SIGNAL(changed(QString)), this, SLOT(validateFields()));
    connect(m_ui->rdkPath, SIGNAL(changed(QString)), this, SLOT(validateFields()));
    connect(m_ui->csjPin, SIGNAL(textChanged(QString)), this, SLOT(validateFields()));
    connect(m_ui->password, SIGNAL(textChanged(QString)), this, SLOT(validateFields()));
    connect(m_ui->password2, SIGNAL(textChanged(QString)), this, SLOT(validateFields()));
    connect(m_ui->linkLabel, SIGNAL(linkActivated(QString)),
            this, SLOT(showKeysMessage(QString)));

    registerField(QLatin1String(PbdtPathField) + QLatin1Char('*'),
            m_ui->pbdtPath, "path", SIGNAL(changed(QString)));
    registerField(QLatin1String(RdkPathField) + QLatin1Char('*'),
            m_ui->rdkPath, "path", SIGNAL(changed(QString)));
    registerField(QLatin1String(CsjPinField) + QLatin1Char('*'),
            m_ui->csjPin);
    registerField(QLatin1String(PasswordField) + QLatin1Char('*'),
            m_ui->password);
    registerField(QLatin1String(Password2Field) + QLatin1Char('*'),
            m_ui->password2);
}

void BlackBerrySetupWizardKeysPage::csjAutoComplete(const QString &path)
{
    Utils::PathChooser *chooser = 0;
    QString file = path;

    if (file.contains(QLatin1String("PBDT"))) {
        file.replace(QLatin1String("PBDT"), QLatin1String("RDK"));
        chooser = m_ui->rdkPath;
    } else if (file.contains(QLatin1String("RDK"))) {
        file.replace(QLatin1String("RDK"), QLatin1String("PBDT"));
        chooser = m_ui->pbdtPath;
    }

    if (!chooser)
        return;

    QFileInfo fileInfo(file);

    if (fileInfo.exists())
        chooser->setPath(file);
}

void BlackBerrySetupWizardKeysPage::setupCsjPathChooser(Utils::PathChooser *chooser)
{
    chooser->setExpectedKind(Utils::PathChooser::File);
    chooser->setPromptDialogTitle(tr("Browse CSJ File"));
    chooser->setPromptDialogFilter(tr("CSJ files (*.csj)"));
}

void BlackBerrySetupWizardKeysPage::setComplete(bool complete)
{
    if (m_complete != complete) {
        m_complete = complete;
        emit completeChanged();
    }
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
    setSubTitle(tr("Your environment is ready to be configured."));

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

/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
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
**************************************************************************/

#include "s60publishingsissettingspageovi.h"
#include "ui_s60publishingsissettingspageovi.h"
#include "s60publisherovi.h"
#include "s60certificateinfo.h"

#include <QtGui/QAbstractButton>

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

S60PublishingSisSettingsPageOvi::S60PublishingSisSettingsPageOvi(S60PublisherOvi *publisher, QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::S60PublishingSisSettingsPageOvi),
    m_publisher(publisher)
{
    ui->setupUi(this);

    //Setup labels which display icons about the state of the entry
    //error icons
    ui->capabilitiesErrorLabel->hide();
    ui->qtVersionErrorLabel->hide();
    ui->uid3ErrorLabel->hide();
    ui->globalVendorNameErrorLabel->hide();
    ui->localisedVendorNamesErrorLabel->hide();
    //ok icons
    ui->capabilitiesOkLabel->hide();
    ui->qtVersionOkLabel->hide();
    ui->uid3OkLabel->hide();
    ui->globalVendorNameOkLabel->hide();
    ui->localisedVendorNamesOkLabel->hide();
    //warning icons
    ui->globalVendorNameWarningLabel->hide();
    ui->localisedVendorNamesWarningLabel->hide();
    ui->qtVersionWarningLabel->hide();
    ui->uid3WarningLabel->hide();
}

void S60PublishingSisSettingsPageOvi::initializePage()
{
    //Finish creation of the publisher
    m_publisher->completeCreation();

    showWarningsForUnenforcableChecks();

    //Check Global Vendor Name
    ui->globalVendorNameLineEdit->setText(m_publisher->globalVendorName());
    globalVendorNameChanged();
    connect(ui->globalVendorNameLineEdit,SIGNAL(textChanged(QString)),SLOT(globalVendorNameChanged()));

    //Check Localised Vendor Names
    ui->localisedVendorNamesLineEdit->setText(m_publisher->localisedVendorNames());
    localisedVendorNamesChanged();
    connect(ui->localisedVendorNamesLineEdit,SIGNAL(textChanged(QString)),SLOT(localisedVendorNamesChanged()));

    //Check Qt Version Used in Builds
    ui->qtVersionDisplayLabel->setText(m_publisher->qtVersion());
    qtVersionChanged();

    //Check UID3
    ui->uid3LineEdit->setText(m_publisher->uid3());
    uid3Changed();
    connect(ui->uid3LineEdit,SIGNAL(textChanged(QString)),SLOT(uid3Changed()));

    //Check for capabilities which are not signed for
    ui->capabilitiesDisplayLabel->setText(m_publisher->capabilities());
    capabilitiesChanged();
}

void S60PublishingSisSettingsPageOvi::cleanupPage()
{
    m_publisher->cleanUp();
}

S60PublishingSisSettingsPageOvi::~S60PublishingSisSettingsPageOvi()
{
    delete ui;
}

void S60PublishingSisSettingsPageOvi::reflectSettingState(bool settingState, QLabel *okLabel, QLabel *errorLabel, QLabel *errorReasonLabel, const QString &errorReason)
{
    if (!settingState) {
        okLabel->hide();
        errorLabel->show();
        errorReasonLabel->setTextFormat(Qt::RichText);
        errorReasonLabel->setText(errorReason);
        errorReasonLabel->show();
    } else {
        okLabel->show();
        errorLabel->hide();
        errorReasonLabel->hide();
    }

    // This is a hack.
    // the labels change size, most likely increasing height but that doesn't change the wizard layout
    // It essentially forces QWizard to update its layout. (Until setTitleFormat checks whether the format changed at all...)
    // todo figure out whether the QWizard should be doing that automatically
    wizard()->setTitleFormat(wizard()->titleFormat());
}

void S60PublishingSisSettingsPageOvi::globalVendorNameChanged()
{
    reflectSettingState(m_publisher->isVendorNameValid(ui->globalVendorNameLineEdit->text()),
                        ui->globalVendorNameOkLabel,
                        ui->globalVendorNameErrorLabel,
                        ui->globalVendorNameErrorReasonLabel,
                        tr("%1 is a default vendor name used for testing and development. <br>"
                           "The Vendor_Name field cannot contain the name 'Nokia'. <br>"
                           "You are advised against using the default names 'Vendor' and 'Vendor-EN'. <br>"
                           "You should also not leave the entry blank. <br>"
                           "see <a href=\"http://www.forum.nokia.com/Distribute/Packaging_and_signing.xhtml\">Packaging and Signing</a> for guidelines.<br>")
                        .arg("\"" + ui->globalVendorNameLineEdit->text() + "\""));
    m_publisher->setVendorName(ui->globalVendorNameLineEdit->text());
}

void S60PublishingSisSettingsPageOvi::localisedVendorNamesChanged()
{
    QStringList localisedVendorNames = ui->localisedVendorNamesLineEdit->text().split(',');

    bool settingState = true;
    QStringList wrongVendorNames;

    foreach (const QString &localisedVendorName, localisedVendorNames) {
        if (!m_publisher->isVendorNameValid(localisedVendorName)) {
            wrongVendorNames.append(localisedVendorName);
            settingState = false;
        }
    }

    QString pluralOrSingular = tr("%1 is a default vendor name used for testing and development.").arg(wrongVendorNames.join(", "));
    if (wrongVendorNames.count() > 1)
        pluralOrSingular = tr("%1 are default vendor names used for testing and development.").arg(wrongVendorNames.join(", "));

    reflectSettingState(settingState,
                        ui->localisedVendorNamesOkLabel,
                        ui->localisedVendorNamesErrorLabel,
                        ui->localisedVendorNamesErrorReasonLabel,
                        tr("%1 <br>"
                           "The Vendor_Name field cannot contain the name 'Nokia'. <br>"
                           "You are advised against using the default names 'Vendor' and 'Vendor-EN'. <br>"
                           "You should also not leave the entry blank. <br>"
                           "See <a href=\"http://www.forum.nokia.com/Distribute/Packaging_and_signing.xhtml\">"
                           "Packaging and Signing</a> for guidelines.<br>").arg(pluralOrSingular));
    m_publisher->setLocalVendorNames(ui->localisedVendorNamesLineEdit->text());
}

void S60PublishingSisSettingsPageOvi::qtVersionChanged()
{
}

void S60PublishingSisSettingsPageOvi::uid3Changed()
{
    QString testUID3ErrorMsg = tr("The application UID %1 is only for testing and development.<br>"
                               "SIS packages built with it cannot be distributed via the Ovi Store.<br>");

    QString symbianSignedUID3ErrorMsg = tr("The application UID %1 is a symbiansigned.com UID. <br>"
                                        "Applications with this UID will be rejected by "
                                        "Application Signing Services for Ovi Store.<br>"
                                        "If you want to continue with a symbiansigned.com UID, "
                                        "sign your application on symbiansigned.com and upload the "
                                        "signed application to Publish to Ovi.<br>");

    QString errorMsg = tr("The application UID %1 is not an acceptable UID.<br>"
                          "SIS packages built with it cannot be signed by "
                          "Application Signing Services for Ovi Store.<br>");

    if (m_publisher->isTestUID3(ui->uid3LineEdit->text())) {
        errorMsg = testUID3ErrorMsg;
    } else if (m_publisher->isKnownSymbianSignedUID3(ui->uid3LineEdit->text())) {
        errorMsg = symbianSignedUID3ErrorMsg;
    }

    reflectSettingState(m_publisher->isUID3Valid(ui->uid3LineEdit->text()),
                        ui->uid3OkLabel,
                        ui->uid3ErrorLabel,
                        ui->uid3ErrorReasonLabel,
                        tr("The application UID is a global unique indentifier of the SIS package.<br>") +
                        errorMsg.arg(ui->uid3LineEdit->text()) +
                        tr("To get a unique application UID for your package file,<br>"
                           "please register at <a href=\"http://info.publish.ovi.com/\">publish.ovi.com</a>"));

    if (m_publisher->isUID3Valid(ui->uid3LineEdit->text())) {
        ui->uid3WarningLabel->show();
        ui->uid3WarningReasonLabel->setText("If this UID is from symbiansigned.com, It will be "
                                            "rejected by Application Signing Services for Ovi Store.<br>"
                                            "If you want to continue with a symbiansigned.com UID, "
                                            "sign your application on symbiansigned.com and upload "
                                            "the signed application to Publish to Ovi.<br>"
                                            "It is, however, recommended that you obtain a UID from "
                                            "<a href=\"http://info.publish.ovi.com/\">publish.ovi.com</a>");
        ui->uid3WarningReasonLabel->show();
    } else {
        ui->uid3WarningLabel->hide();
        ui->uid3WarningReasonLabel->hide();
    }

    m_publisher->setAppUid(ui->uid3LineEdit->text());
}

void S60PublishingSisSettingsPageOvi::capabilitiesChanged()
{
    QStringList capabilities = ui->capabilitiesDisplayLabel->text().split(',');
    QString errorMessage;

    //Check for certified Signed capabilities
    QStringList capabilitesNeedingCertifiedSigned;
    foreach (const QString &capability, capabilities) {
       if( m_publisher->isCapabilityOneOf(capability, S60PublisherOvi::CertifiedSigned)) {
            capabilitesNeedingCertifiedSigned.append(capability);
            capabilities.removeOne(capability);
        }
    }

    if (!capabilitesNeedingCertifiedSigned.isEmpty())
        errorMessage.append(tr("%1 need(s) to be certified signed. "
                               "Please go to <a href=\"symbiansigned.com\">symbiansigned.com</a> for guidance.")
                            .arg(capabilitesNeedingCertifiedSigned.join(", ")));

    //Check for capabilities needing manufacturer approval
    QStringList capabilitiesNeedingManufacturerApproved;

    foreach (const QString &capability, capabilities) {
       if( m_publisher->isCapabilityOneOf(capability, S60PublisherOvi::ManufacturerApproved))
            capabilitiesNeedingManufacturerApproved.append(capability);
    }

    if (!capabilitiesNeedingManufacturerApproved.isEmpty()) {
        errorMessage.append(tr("<br>%1 need(s) manufacturer approval.<br>").arg(capabilitiesNeedingManufacturerApproved.join(", ")));
    }

    errorMessage.prepend(tr("Some capabilities might require a special kind of signing or approval from the manufacturer.<br>"));

    reflectSettingState(capabilitesNeedingCertifiedSigned.isEmpty() && capabilitiesNeedingManufacturerApproved.isEmpty(),
                        ui->capabilitiesOkLabel,
                        ui->capabilitiesErrorLabel,
                        ui->capabilitiesErrorReasonLabel,
                        errorMessage);
}

void S60PublishingSisSettingsPageOvi::showWarningsForUnenforcableChecks()
{
    //Warn about use of unreleased Qt Versions
    //ui->qtVersionWarningLabel->show(); //looks better without...
    ui->qtVersionWarningReasonLabel->setText(tr("Please verify that you have a released version of Qt. <br>"
                                                "<a href=\"http://wiki.forum.nokia.com/index.php/Nokia_Smart_Installer_for_Symbian\">Qt Packages Distributed by Smart Installer</a> has a list of released Qt versions."));
}

} // namespace Internal
} // namespace Qt4ProjectManager

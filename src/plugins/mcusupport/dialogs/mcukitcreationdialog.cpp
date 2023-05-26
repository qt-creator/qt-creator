// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcukitcreationdialog.h"
#include "ui_mcukitcreationdialog.h"

#include "../mcuabstractpackage.h"
#include "../mcusupportconstants.h"
#include "../mcusupporttr.h"

#include <utils/filepath.h>
#include <utils/icon.h>

#include <coreplugin/icore.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QStyle>

namespace McuSupport::Internal {

McuKitCreationDialog::McuKitCreationDialog(const MessagesList &messages,
                                           const SettingsHandler::Ptr &settingsHandler,
                                           McuPackagePtr qtMCUPackage,
                                           QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::McuKitCreationDialog)
    , m_messages(messages)
{
    ui->setupUi(this);
    ui->iconLabel->setPixmap(Utils::Icon(":/mcusupport/images/mcusupportdevice.png").pixmap());
    m_previousButton = ui->buttonBox->addButton("<", QDialogButtonBox::ActionRole);
    m_nextButton = ui->buttonBox->addButton(">", QDialogButtonBox::ActionRole);
    m_fixButton = ui->buttonBox->addButton(Tr::tr("Fix"), QDialogButtonBox::ActionRole);
    m_helpButton = ui->buttonBox->addButton(Tr::tr("Help"), QDialogButtonBox::HelpRole);
    // prevent clicking the buttons from closing the message box
    m_nextButton->disconnect();
    m_previousButton->disconnect();

    if (messages.size() == 1) {
        m_nextButton->setVisible(false);
        m_previousButton->setVisible(false);
    }
    //display first message
    if (messages.size() > 1)
        updateMessage(1);

    if (qtMCUPackage->isValidStatus())
        ui->qtMCUsPathLabel->setText(
            Tr::tr("Qt for MCUs path %1").arg(qtMCUPackage->path().toUserOutput()));
    connect(m_nextButton, &QPushButton::clicked, [=] { updateMessage(1); });
    connect(m_previousButton, &QPushButton::clicked, [=] { updateMessage(-1); });
    connect(m_fixButton, &QPushButton::clicked, [=] {
        // Open the MCU Options widget on the current platform
        settingsHandler->setInitialPlatformName(m_messages[m_currentIndex].platform);
        Core::ICore::showOptionsDialog(Constants::SETTINGS_ID);
        // reset the initial platform name
        settingsHandler->setInitialPlatformName("");
    });
    connect(m_helpButton, &QPushButton::clicked, [] {
        QDesktopServices::openUrl(QUrl("https://doc.qt.io/QtForMCUs/qtul-prerequisites.html"));
    });
}

void McuKitCreationDialog::updateMessage(const int inc)
{
    m_currentIndex += inc;
    m_nextButton->setEnabled(m_currentIndex < (m_messages.size() - 1));
    m_previousButton->setEnabled(m_currentIndex > 0);
    ui->textLabel->setText(QString("<b>%1 %2</b> : %3")
                               .arg(Tr::tr("Target"),
                                    (m_messages[m_currentIndex].status == McuSupportMessage::Warning
                                         ? Tr::tr("Warning")
                                         : Tr::tr("Error")),
                                    m_messages[m_currentIndex].platform));
    ui->iconLabel->setPixmap(
        QApplication::style()
            ->standardIcon(m_messages[m_currentIndex].status == McuSupportMessage::Warning
                               ? QStyle::SP_MessageBoxWarning
                               : QStyle::SP_MessageBoxCritical)
            .pixmap(64, 64));
    ui->informationLabel->setText(QString("<b>%1</b>: %2<br><br><b>%3</b>: %4")
                                      .arg(Tr::tr("Package"),
                                           m_messages[m_currentIndex].packageName,
                                           Tr::tr("Status"),
                                           m_messages.at(m_currentIndex).message));
    ui->messageCountLabel->setText(QString("%1 / %2").arg(QString::number(m_currentIndex + 1),
                                                          QString::number(m_messages.size())));
}

McuKitCreationDialog::~McuKitCreationDialog()
{
    delete ui;
}

} // namespace McuSupport::Internal

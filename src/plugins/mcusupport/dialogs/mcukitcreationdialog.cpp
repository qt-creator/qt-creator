// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcukitcreationdialog.h"

#include "../mcuabstractpackage.h"
#include "../mcusupportconstants.h"
#include "../mcusupporttr.h"

#include <utils/filepath.h>
#include <utils/icon.h>
#include <utils/layoutbuilder.h>

#include <coreplugin/icore.h>

#include <QApplication>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QStyle>

namespace McuSupport::Internal {

McuKitCreationDialog::McuKitCreationDialog(const MessagesList &messages,
                                           const SettingsHandler::Ptr &settingsHandler,
                                           McuPackagePtr qtMCUPackage,
                                           QWidget *parent)
    : QDialog(parent)
    , m_messages(messages)
{
    resize(500, 300);
    setWindowTitle(Tr::tr("Qt for MCUs Kit Creation"));

    m_iconLabel = new QLabel;
    m_iconLabel->setAlignment(Qt::AlignTop);

    m_textLabel = new QLabel;

    m_informationLabel = new QLabel;
    m_informationLabel->setWordWrap(true);
    m_informationLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_informationLabel->setAlignment(Qt::AlignTop);

    m_qtMCUsPathLabel = new QLabel;

    auto line = new QFrame;
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Sunken);

    auto buttonBox = new QDialogButtonBox(Qt::Vertical);
    buttonBox->setStandardButtons(QDialogButtonBox::Ignore);

    m_messageCountLabel = new QLabel;
    m_messageCountLabel->setAlignment(Qt::AlignCenter);

    using namespace Layouting;
    Row {
        Column {
            Row {
                m_iconLabel,
                Column {
                    m_textLabel,
                    m_informationLabel,
                },
            },
            m_qtMCUsPathLabel,
        },
        line,
        Column {
            buttonBox,
            m_messageCountLabel,
        },
    }.attachTo(this);

    m_previousButton = buttonBox->addButton("<", QDialogButtonBox::ActionRole);
    m_nextButton = buttonBox->addButton(">", QDialogButtonBox::ActionRole);
    QPushButton *fixButton = buttonBox->addButton(Tr::tr("Fix"), QDialogButtonBox::ActionRole);
    QPushButton *helpButton = buttonBox->addButton(Tr::tr("Help"), QDialogButtonBox::HelpRole);

    if (messages.size() == 1) {
        m_nextButton->setVisible(false);
        m_previousButton->setVisible(false);
    }
    //display first message
    if (messages.size() > 1)
        updateMessage(1);

    if (qtMCUPackage->isValidStatus())
        m_qtMCUsPathLabel->setText(
            Tr::tr("Qt for MCUs path %1").arg(qtMCUPackage->path().toUserOutput()));
    connect(m_nextButton, &QPushButton::clicked, this, [this] { updateMessage(1); });
    connect(m_previousButton, &QPushButton::clicked, this, [this] { updateMessage(-1); });
    connect(fixButton, &QPushButton::clicked, this, [this, settingsHandler] {
        // Open the MCU Options widget on the current platform
        settingsHandler->setInitialPlatformName(m_messages[m_currentIndex].platform);
        Core::ICore::showOptionsDialog(Constants::SETTINGS_ID);
        // reset the initial platform name
        settingsHandler->setInitialPlatformName("");
    });
    connect(helpButton, &QPushButton::clicked, [] {
        QDesktopServices::openUrl(QUrl("https://doc.qt.io/QtForMCUs/qtul-prerequisites.html"));
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
}

void McuKitCreationDialog::updateMessage(const int inc)
{
    m_currentIndex += inc;
    m_nextButton->setEnabled(m_currentIndex < (m_messages.size() - 1));
    m_previousButton->setEnabled(m_currentIndex > 0);
    m_textLabel->setText(QString("<b>%1 %2</b> : %3")
                               .arg(Tr::tr("Target"),
                                    (m_messages[m_currentIndex].status == McuSupportMessage::Warning
                                         ? Tr::tr("Warning")
                                         : Tr::tr("Error")),
                                    m_messages[m_currentIndex].platform));
    m_iconLabel->setPixmap(
        QApplication::style()
            ->standardIcon(m_messages[m_currentIndex].status == McuSupportMessage::Warning
                               ? QStyle::SP_MessageBoxWarning
                               : QStyle::SP_MessageBoxCritical)
            .pixmap(64, 64));
    m_informationLabel->setText(QString("<b>%1</b>: %2<br><br><b>%3</b>: %4")
                                      .arg(Tr::tr("Package"),
                                           m_messages[m_currentIndex].packageName,
                                           Tr::tr("Status"),
                                           m_messages.at(m_currentIndex).message));
    m_messageCountLabel->setText(QString("%1 / %2").arg(QString::number(m_currentIndex + 1),
                                                          QString::number(m_messages.size())));
}

} // namespace McuSupport::Internal

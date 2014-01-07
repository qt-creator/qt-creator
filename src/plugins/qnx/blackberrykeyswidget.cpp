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

#include "blackberrykeyswidget.h"
#include "blackberryconfigurationmanager.h"
#include "blackberrycertificate.h"
#include "blackberrysigningutils.h"
#include "blackberrycreatecertificatedialog.h"
#include "ui_blackberrykeyswidget.h"

#include <QInputDialog>
#include <QMessageBox>

namespace Qnx {
namespace Internal {

BlackBerryKeysWidget::BlackBerryKeysWidget(QWidget *parent) :
    QWidget(parent),
    m_utils(BlackBerrySigningUtils::instance()),
    m_ui(new Ui_BlackBerryKeysWidget)
{
    m_ui->setupUi(this);
    m_ui->keyStatus->setTextFormat(Qt::RichText);
    m_ui->keyStatus->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_ui->keyStatus->setOpenExternalLinks(true);
    m_ui->openCertificateButton->setVisible(false);

    connect(m_ui->createCertificateButton, SIGNAL(clicked()),
            this, SLOT(createCertificate()));
    connect(m_ui->clearCertificateButton, SIGNAL(clicked()),
            this, SLOT(clearCertificate()));
    connect(m_ui->openCertificateButton, SIGNAL(clicked()),
            this, SLOT(loadDefaultCertificate()));
}

void BlackBerryKeysWidget::certificateLoaded(int status)
{
    disconnect(&m_utils, SIGNAL(defaultCertificateLoaded(int)), this, SLOT(certificateLoaded(int)));

    switch (status) {
    case BlackBerryCertificate::Success:
        m_ui->certificateAuthor->setText(m_utils.defaultCertificate()->author());
        m_ui->openCertificateButton->setVisible(false);
        break;
    case BlackBerryCertificate::WrongPassword:
        if (QMessageBox::question(this, tr("Qt Creator"),
                    tr("Invalid certificate password. Try again?"),
                    QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            loadDefaultCertificate();
        } else {
            m_ui->certificateAuthor->clear();
            m_ui->openCertificateButton->setVisible(true);
        }
        break;
    case BlackBerryCertificate::Busy:
    case BlackBerryCertificate::InvalidOutputFormat:
    case BlackBerryCertificate::Error:
        setCertificateError(tr("Error loading certificate."));
        m_ui->openCertificateButton->setVisible(true);
        break;
    }
}

void BlackBerryKeysWidget::createCertificate()
{
    BlackBerryCreateCertificateDialog dialog;

    const int result = dialog.exec();

    if (result == QDialog::Rejected)
        return;

    BlackBerryCertificate *certificate = dialog.certificate();

    if (certificate) {
        m_utils.setDefaultCertificate(certificate);
        updateCertificateSection();
    }
}

void BlackBerryKeysWidget::clearCertificate()
{
    if (QMessageBox::warning(this, tr("Qt Creator"),
                tr("This action cannot be undone. Would you like to continue?"),
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        m_utils.deleteDefaultCertificate();
        updateCertificateSection();
    }
}

void BlackBerryKeysWidget::showEvent(QShowEvent *)
{
    updateKeysSection();
    updateCertificateSection();
}

void BlackBerryKeysWidget::updateCertificateSection()
{
    if (m_utils.hasDefaultCertificate()) {
        setCreateCertificateVisible(false);

        BlackBerryConfigurationManager &configManager = BlackBerryConfigurationManager::instance();

        m_ui->certificatePath->setText(configManager.defaultKeystorePath());
        m_ui->certificateAuthor->setText(tr("Loading..."));

        loadDefaultCertificate();
    } else {
        setCreateCertificateVisible(true);
    }
}

void BlackBerryKeysWidget::updateKeysSection()
{
    if (m_utils.hasLegacyKeys()) {
        m_ui->keyStatus->setText(tr("It appears you are using legacy key files. "
                    "Please refer to the "
                    "<a href=\"https://developer.blackberry.com/native/documentation"
                    "/core/com.qnx.doc.native_sdk.devguide/com.qnx.doc.native_sdk.devguide/topic/bbid_to_sa.html\">"
                    "BlackBerry website</a> to find out how to update your keys."));
    } else if (m_utils.hasRegisteredKeys()) {
        m_ui->keyStatus->setText(tr("Your keys are ready to be used"));
    } else {
        m_ui->keyStatus->setText(tr("No keys found. Please refer to the "
                    "<a href=\"https://www.blackberry.com/SignedKeys/codesigning.html\">BlackBerry website</a> "
                    "to find out how to request your keys."));
    }
}

void BlackBerryKeysWidget::loadDefaultCertificate()
{
    const BlackBerryCertificate *certificate = m_utils.defaultCertificate();

    if (certificate) {
        m_ui->certificateAuthor->setText(certificate->author());
        m_ui->openCertificateButton->setVisible(false);
        return;
    }

    connect(&m_utils, SIGNAL(defaultCertificateLoaded(int)), this, SLOT(certificateLoaded(int)));
    m_utils.openDefaultCertificate();
}

void BlackBerryKeysWidget::setCertificateError(const QString &error)
{
    m_ui->certificateAuthor->clear();
    QMessageBox::critical(this, tr("Qt Creator"), error);
}

void BlackBerryKeysWidget::setCreateCertificateVisible(bool visible)
{
    m_ui->pathLabel->setVisible(!visible);
    m_ui->authorLabel->setVisible(!visible);
    m_ui->certificatePath->setVisible(!visible);
    m_ui->certificateAuthor->setVisible(!visible);
    m_ui->clearCertificateButton->setVisible(!visible);
    m_ui->openCertificateButton->setVisible(!visible);
    m_ui->noCertificateLabel->setVisible(visible);
    m_ui->createCertificateButton->setVisible(visible);
}

} // namespace Internal
} // namespace Qnx


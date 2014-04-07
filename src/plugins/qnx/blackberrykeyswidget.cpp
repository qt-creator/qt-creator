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
#include "blackberrydebugtokenreader.h"
#include "blackberrydebugtokenpinsdialog.h"
#include "blackberrydebugtokenrequester.h"
#include "blackberrydebugtokenrequestdialog.h"
#include "ui_blackberrykeyswidget.h"

#include "qnxconstants.h"

#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>

#include <QStandardItemModel>

namespace Qnx {
namespace Internal {

BlackBerryKeysWidget::BlackBerryKeysWidget(QWidget *parent) :
    QWidget(parent),
    m_utils(BlackBerrySigningUtils::instance()),
    m_ui(new Ui_BlackBerryKeysWidget),
    m_dtModel(new QStandardItemModel(this)),
    m_requester(new BlackBerryDebugTokenRequester(this))
{
    m_ui->setupUi(this);
    m_ui->keyStatus->setTextFormat(Qt::RichText);
    m_ui->keyStatus->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_ui->keyStatus->setOpenExternalLinks(true);
    m_ui->openCertificateButton->setVisible(false);
    m_ui->editDbTkButton->setEnabled(false);
    m_ui->removeDbTkButton->setEnabled(false);
    m_ui->debugTokens->setModel(m_dtModel);

    updateDebugTokenList();

    connect(m_ui->createCertificateButton, SIGNAL(clicked()),
            this, SLOT(createCertificate()));
    connect(m_ui->clearCertificateButton, SIGNAL(clicked()),
            this, SLOT(clearCertificate()));
    connect(m_ui->openCertificateButton, SIGNAL(clicked()),
            this, SLOT(loadDefaultCertificate()));
    connect(m_ui->requestDbTkButton, SIGNAL(clicked()),
            this, SLOT(requestDebugToken()));
    connect(m_ui->importDbTkButton, SIGNAL(clicked()),
            this, SLOT(importDebugToken()));
    connect(m_ui->editDbTkButton, SIGNAL(clicked()),
            this, SLOT(editDebugToken()));
    connect(m_ui->removeDbTkButton, SIGNAL(clicked()),
            this, SLOT(removeDebugToken()));
    connect(m_requester, SIGNAL(finished(int)),
            this, SLOT(requestFinished(int)));
    connect(m_ui->debugTokens, SIGNAL(pressed(QModelIndex)),
            this, SLOT(updateUi(QModelIndex)));
    connect(&m_utils, SIGNAL(debugTokenListChanged()),
            this, SLOT(updateDebugTokenList()));
}

void BlackBerryKeysWidget::saveSettings()
{
    m_utils.saveDebugTokens();
}

void BlackBerryKeysWidget::initModel()
{
    m_dtModel->clear();
    QStringList headers;
    headers << tr("Path") << tr("Author") << tr("PINs") << tr("Expiry");
    m_dtModel->setHorizontalHeaderLabels(headers);
}

void BlackBerryKeysWidget::certificateLoaded(int status)
{
    disconnect(&m_utils, SIGNAL(defaultCertificateLoaded(int)), this, SLOT(certificateLoaded(int)));

    switch (status) {
    case BlackBerryCertificate::Success:
        m_ui->certificateAuthor->setText(m_utils.defaultCertificate()->author());
        m_ui->certificateAuthor->setVisible(true);
        m_ui->authorLabel->setVisible(true);
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
    if (m_utils.createCertificate())
        updateCertificateSection();
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

        m_ui->certificatePath->setText(BlackBerryConfigurationManager::instance()->defaultKeystorePath());

        const BlackBerryCertificate *certificate = m_utils.defaultCertificate();

        if (certificate) {
            m_ui->certificateAuthor->setText(certificate->author());
            m_ui->openCertificateButton->setVisible(false);
            return;
        }

        m_ui->openCertificateButton->setVisible(true);
        m_ui->certificateAuthor->setVisible(false);
        m_ui->authorLabel->setVisible(false);
    } else {
        setCreateCertificateVisible(true);
    }
}

void BlackBerryKeysWidget::updateKeysSection()
{
    if (m_utils.hasLegacyKeys()) {
        m_ui->keyStatus->setText(tr("It appears you are using legacy key files. Please refer to the "
                    "<a href=\"%1\">BlackBerry website</a> to find out how to update your keys.")
                                 .arg(QLatin1String(Qnx::Constants::QNX_LEGACY_KEYS_URL)));
    } else if (m_utils.hasRegisteredKeys()) {
        m_ui->keyStatus->setText(tr("Your keys are ready to be used"));
    } else {
        m_ui->keyStatus->setText(tr("No keys found. Please refer to the "
                    "<a href=\"%1\">BlackBerry website</a> "
                    "to find out how to request your keys.")
                                 .arg(QLatin1String(Qnx::Constants::QNX_REGISTER_KEYS_URL)));
    }
}

void BlackBerryKeysWidget::loadDefaultCertificate()
{
    connect(&m_utils, SIGNAL(defaultCertificateLoaded(int)), this, SLOT(certificateLoaded(int)));
    m_utils.openDefaultCertificate(this);
}

void BlackBerryKeysWidget::updateDebugTokenList()
{
    initModel();
    foreach (const QString &dt, m_utils.debugTokens()) {
        QList<QStandardItem*> row;
        BlackBerryDebugTokenReader debugTokenReader(dt);
        if (!debugTokenReader.isValid())
            continue;

        row << new QStandardItem(dt);
        row << new QStandardItem(debugTokenReader.author());
        row << new QStandardItem(debugTokenReader.pins());
        row << new QStandardItem(debugTokenReader.expiry());
        m_dtModel->appendRow(row);
    }

    m_ui->debugTokens->header()->resizeSections(QHeaderView::ResizeToContents);
}

void BlackBerryKeysWidget::requestDebugToken()
{
    BlackBerryDebugTokenRequestDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    m_utils.addDebugToken(dialog.debugToken());
}

void BlackBerryKeysWidget::importDebugToken()
{
    const QString debugToken = QFileDialog::getOpenFileName(this, tr("Select Debug Token"),
                                                            QString(), tr("Bar file (*.bar)"));
    if (debugToken.isEmpty())
        return;

    BlackBerryDebugTokenReader debugTokenReader(debugToken);
    if (!debugTokenReader.isValid()) {
        QMessageBox::warning(this, tr("Invalid Debug Token"),
                             tr("Debug token file %1 cannot be read.").arg(debugToken));
        return;
    }

    m_utils.addDebugToken(debugToken);
}

void BlackBerryKeysWidget::editDebugToken()
{
    const QModelIndex index = m_ui->debugTokens->currentIndex();
    if (!index.isValid())
        return;

    QString pins = m_dtModel->item(index.row(), 0)->text();

    BlackBerryDebugTokenPinsDialog dialog(pins, this);
    connect(&dialog, SIGNAL(pinsUpdated(QStringList)), this, SLOT(updateDebugToken(QStringList)));
    dialog.exec();
}

void BlackBerryKeysWidget::removeDebugToken()
{
    const QModelIndex index = m_ui->debugTokens->currentIndex();
    if (!index.isValid())
        return;

    const QString dt = m_dtModel->item(index.row(), 0)->text();
    const int result = QMessageBox::question(this, tr("Confirmation"),
            tr("Are you sure you want to remove %1?")
            .arg(dt), QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes)
        m_utils.removeDebugToken(dt);
}

void BlackBerryKeysWidget::updateDebugToken(const QStringList &pins)
{
    bool ok;
    const QString cskPassword = m_utils.cskPassword(this, &ok);
    if (!ok)
        return;

    const QString certificatePassword = m_utils.certificatePassword(this, &ok);
    if (!ok)
        return;

    const QString debugTokenPath = m_dtModel->item(m_ui->debugTokens->currentIndex().row(), 0)->text();
    m_requester->requestDebugToken(debugTokenPath,
                                   cskPassword, BlackBerryConfigurationManager::instance()->defaultKeystorePath(),
                                   certificatePassword, pins.join(QLatin1String(",")));
}

void BlackBerryKeysWidget::requestFinished(int status)
{
    QString errorString = tr("Failed to request debug token:") + QLatin1Char(' ');

    switch (status) {
    case BlackBerryDebugTokenRequester::Success:
        updateDebugTokenList();
        return;
    case BlackBerryDebugTokenRequester::WrongCskPassword:
        m_utils.clearCskPassword();
        errorString += tr("Wrong CSK password.");
        break;
    case BlackBerryDebugTokenRequester::WrongKeystorePassword:
        m_utils.clearCertificatePassword();
        errorString += tr("Wrong keystore password.");
        break;
    case BlackBerryDebugTokenRequester::NetworkUnreachable:
        errorString += tr("Network unreachable.");
        break;
    case BlackBerryDebugTokenRequester::IllegalPin:
        errorString += tr("Illegal device PIN.");
        break;
    case BlackBerryDebugTokenRequester::FailedToStartInferiorProcess:
        errorString += tr("Failed to start inferior process.");
        break;
    case BlackBerryDebugTokenRequester::InferiorProcessTimedOut:
        errorString += tr("Inferior processes timed out.");
        break;
    case BlackBerryDebugTokenRequester::InferiorProcessCrashed:
        errorString += tr("Inferior process has crashed.");
        break;
    case BlackBerryDebugTokenRequester::InferiorProcessReadError:
    case BlackBerryDebugTokenRequester::InferiorProcessWriteError:
        errorString += tr("Failed to communicate with the inferior process.");
        break;
    case BlackBerryDebugTokenRequester::NotYetRegistered:
        errorString += tr("Not yet registered to request debug tokens.");
        break;
    case BlackBerryDebugTokenRequester::UnknownError:
    default:
        m_utils.clearCertificatePassword();
        m_utils.clearCskPassword();
        errorString += tr("An unknown error has occurred.");
        break;
    }

    QMessageBox::critical(this, tr("Error"), errorString);
}

void BlackBerryKeysWidget::updateUi(const QModelIndex &index)
{
    m_ui->editDbTkButton->setEnabled(index.isValid());
    m_ui->removeDbTkButton->setEnabled(index.isValid());
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


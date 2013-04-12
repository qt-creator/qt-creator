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

#include "blackberryregisterkeydialog.h"
#include "blackberrycsjregistrar.h"
#include "blackberryconfiguration.h"
#include "blackberrycertificate.h"
#include "blackberryutils.h"
#include "ui_blackberryregisterkeydialog.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QPushButton>
#include <QMessageBox>
#include <QTextStream>

#include <utils/pathchooser.h>

namespace Qnx {
namespace Internal {

BlackBerryRegisterKeyDialog::BlackBerryRegisterKeyDialog(QWidget *parent,
        Qt::WindowFlags f) :
    QDialog(parent, f),
    m_ui(new Ui_BlackBerryRegisterKeyDialog),
    m_registrar(new BlackBerryCsjRegistrar(this)),
    m_certificate(0)
{
    m_ui->setupUi(this);
    m_ui->statusLabel->clear();

    setupCsjPathChooser(m_ui->pbdtPath);
    setupCsjPathChooser(m_ui->rdkPath);

    m_cancelButton = m_ui->buttonBox->button(QDialogButtonBox::Cancel);

    m_okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);

    setBusy(false);

    m_okButton->setEnabled(false);

    QFileInfo authorP12(BlackBerryConfiguration::instance().defaultKeystorePath());

    if (authorP12.exists()) {
        m_ui->genCert->setEnabled(false);
        m_ui->genCert->setChecked(false);
        m_ui->keystorePassword->setEnabled(false);
        m_ui->keystorePassword2->setEnabled(false);
    } else {
        m_ui->genCert->setEnabled(true);
        m_ui->genCert->setChecked(true);
        m_ui->keystorePassword->setEnabled(true);
        m_ui->keystorePassword2->setEnabled(true);
    }


    connect(m_cancelButton, SIGNAL(clicked()),
            this, SLOT(reject()));
    connect(m_okButton, SIGNAL(clicked()),
            this, SLOT(createKey()));
    connect(m_ui->pbdtPath, SIGNAL(changed(QString)),
            this, SLOT(csjAutoComplete(QString)));
    connect(m_ui->rdkPath, SIGNAL(changed(QString)),
            this, SLOT(csjAutoComplete(QString)));
    connect(m_ui->showPins, SIGNAL(stateChanged(int)),
            this, SLOT(pinCheckBoxChanged(int)));
    connect(m_registrar, SIGNAL(finished(int,QString)),
            this, SLOT(registrarFinished(int,QString)));
    connect(m_ui->genCert, SIGNAL(stateChanged(int)),
            this, SLOT(certCheckBoxChanged(int)));
    connect(m_ui->pbdtPath, SIGNAL(changed(QString)), this, SLOT(validate()));
    connect(m_ui->rdkPath, SIGNAL(changed(QString)), this, SLOT(validate()));
    connect(m_ui->csjPin, SIGNAL(textChanged(QString)), this, SLOT(validate()));
    connect(m_ui->cskPin, SIGNAL(textChanged(QString)), this, SLOT(validate()));
    connect(m_ui->cskPin2, SIGNAL(textChanged(QString)), this, SLOT(validate()));
    connect(m_ui->keystorePassword, SIGNAL(textChanged(QString)), this, SLOT(validate()));
    connect(m_ui->keystorePassword2, SIGNAL(textChanged(QString)), this, SLOT(validate()));
}

void BlackBerryRegisterKeyDialog::csjAutoComplete(const QString &path)
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

void BlackBerryRegisterKeyDialog::validate()
{
    if (!m_ui->pbdtPath->isValid()
            || !m_ui->rdkPath->isValid()
            || m_ui->csjPin->text().isEmpty()
            || m_ui->cskPin->text().isEmpty()
            || m_ui->cskPin2->text().isEmpty()) {
        m_okButton->setEnabled(false);
        m_ui->statusLabel->clear();
        return;
    }

    if (m_ui->cskPin->text() != m_ui->cskPin2->text()) {
        m_okButton->setEnabled(false);
        m_ui->statusLabel->setText(tr("CSK passwords do not match."));
        return;
    }

    if (m_ui->genCert->isChecked()) {
        if (m_ui->keystorePassword->text().isEmpty()
                || m_ui->keystorePassword2->text().isEmpty()) {
            m_okButton->setEnabled(false);
            m_ui->statusLabel->clear();
            return;
        }

        if (m_ui->keystorePassword->text()
                != m_ui->keystorePassword2->text()) {
            m_ui->statusLabel->setText(tr("Keystore password does not match."));
            m_okButton->setEnabled(false);
            return;
        }
    }

    m_ui->statusLabel->clear();
    m_okButton->setEnabled(true);
}

void BlackBerryRegisterKeyDialog::pinCheckBoxChanged(int state)
{
    if (state == Qt::Checked) {
        m_ui->csjPin->setEchoMode(QLineEdit::Normal);
        m_ui->cskPin->setEchoMode(QLineEdit::Normal);
        m_ui->cskPin2->setEchoMode(QLineEdit::Normal);
        m_ui->keystorePassword->setEchoMode(QLineEdit::Normal);
        m_ui->keystorePassword2->setEchoMode(QLineEdit::Normal);
    } else {
        m_ui->csjPin->setEchoMode(QLineEdit::Password);
        m_ui->cskPin->setEchoMode(QLineEdit::Password);
        m_ui->cskPin2->setEchoMode(QLineEdit::Password);
        m_ui->keystorePassword->setEchoMode(QLineEdit::Password);
        m_ui->keystorePassword2->setEchoMode(QLineEdit::Password);
    }
}

void BlackBerryRegisterKeyDialog::certCheckBoxChanged(int state)
{
    m_ui->keystorePassword->setEnabled(state == Qt::Checked);
    m_ui->keystorePassword2->setEnabled(state == Qt::Checked);

    validate();
}

void BlackBerryRegisterKeyDialog::createKey()
{
    setBusy(true);

    QStringList csjFiles;
    csjFiles << rdkPath() << pbdtPath();

    m_registrar->tryRegister(csjFiles, csjPin(), cskPin());
}

void BlackBerryRegisterKeyDialog::registrarFinished(int status,
        const QString &errorString)
{
    if (status == BlackBerryCsjRegistrar::Error) {
        QMessageBox::critical(this, tr("Error"), errorString);
        cleanup();
        setBusy(false);
        return;
    }

    if (m_ui->genCert->isChecked())
        generateDeveloperCertificate();
    else
        accept();
}

void BlackBerryRegisterKeyDialog::certificateCreated(int status)
{
    if (status == BlackBerryCertificate::Error) {
        QMessageBox::critical(this, tr("Error"), tr("Error creating developer certificate."));
        cleanup();
        m_certificate->deleteLater();
        m_certificate = 0;
        setBusy(false);
    } else {
        accept();
    }
}

QString BlackBerryRegisterKeyDialog::pbdtPath() const
{
    return m_ui->pbdtPath->path();
}

QString BlackBerryRegisterKeyDialog::rdkPath() const
{
    return m_ui->rdkPath->path();
}

QString BlackBerryRegisterKeyDialog::csjPin() const
{
    return m_ui->csjPin->text();
}

QString BlackBerryRegisterKeyDialog::cskPin() const
{
    return m_ui->cskPin->text();
}

QString BlackBerryRegisterKeyDialog::keystorePassword() const
{
    return m_ui->keystorePassword->text();
}

QString BlackBerryRegisterKeyDialog::keystorePath() const
{
    if (m_ui->genCert->isChecked()) {
        BlackBerryConfiguration &configuration = BlackBerryConfiguration::instance();
        return configuration.defaultKeystorePath();
    }

    return QString();
}

BlackBerryCertificate * BlackBerryRegisterKeyDialog::certificate() const
{
    return m_certificate;
}

void BlackBerryRegisterKeyDialog::generateDeveloperCertificate()
{
    m_certificate = new BlackBerryCertificate(keystorePath(),
            BlackBerryUtils::getCsjAuthor(rdkPath()), keystorePassword());

    connect(m_certificate, SIGNAL(finished(int)), this, SLOT(certificateCreated(int)));

    m_certificate->store();
}

void BlackBerryRegisterKeyDialog::cleanup() const
{
    BlackBerryConfiguration &configuration = BlackBerryConfiguration::instance();

    QFile f(configuration.barsignerCskPath());
    f.remove();

    f.setFileName(configuration.barsignerDbPath());
    f.remove();

    if (m_ui->genCert->isChecked()) {
        f.setFileName(configuration.defaultKeystorePath());
        f.remove();
    }
}

void BlackBerryRegisterKeyDialog::setBusy(bool busy)
{
    m_ui->progressBar->setVisible(busy);
    m_okButton->setEnabled(!busy);
    m_cancelButton->setEnabled(!busy);
    m_ui->rdkPath->setEnabled(!busy);
    m_ui->pbdtPath->setEnabled(!busy);
    m_ui->csjPin->setEnabled(!busy);
    m_ui->cskPin->setEnabled(!busy);
    m_ui->cskPin2->setEnabled(!busy);
    m_ui->keystorePassword->setEnabled(!busy);
    m_ui->keystorePassword2->setEnabled(!busy);
    m_ui->showPins->setEnabled(!busy);

}

void BlackBerryRegisterKeyDialog::setupCsjPathChooser(Utils::PathChooser *chooser)
{
    chooser->setExpectedKind(Utils::PathChooser::File);
    chooser->setPromptDialogTitle(tr("Browse CSJ File"));
    chooser->setPromptDialogFilter(tr("CSJ files (*.csj)"));
}

} // namespace Internal
} // namespace Qnx

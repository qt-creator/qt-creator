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

#include "blackberrykeyswidget.h"
#include "blackberryregisterkeydialog.h"
#include "blackberryconfiguration.h"
#include "blackberrycertificatemodel.h"
#include "blackberryimportcertificatedialog.h"
#include "blackberrycreatecertificatedialog.h"
#include "blackberrycertificate.h"
#include "blackberryutils.h"
#include "ui_blackberrykeyswidget.h"

#include <QFileInfo>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMessageBox>

namespace Qnx {
namespace Internal {

BlackBerryKeysWidget::BlackBerryKeysWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui_BlackBerryKeysWidget),
    m_model(new BlackBerryCertificateModel(this))
{
    m_ui->setupUi(this);
    m_ui->certificatesView->setModel(m_model);
    m_ui->certificatesView->resizeColumnsToContents();

    QHeaderView *headerView = m_ui->certificatesView->horizontalHeader();
    headerView->setResizeMode(QHeaderView::Stretch);
    headerView->setResizeMode(2, QHeaderView::Fixed);

    QItemSelectionModel *selectionModel = m_ui->certificatesView->selectionModel();
    connect(selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(handleSelectionChanged(QItemSelection,QItemSelection)));

    updateRegisterSection();

    connect(m_ui->registerButton, SIGNAL(clicked()), this, SLOT(registerKey()));
    connect(m_ui->unregisterButton, SIGNAL(clicked()), this, SLOT(unregisterKey()));
    connect(m_ui->createCertificate, SIGNAL(clicked()), this, SLOT(createCertificate()));
    connect(m_ui->importCertificate, SIGNAL(clicked()), this, SLOT(importCertificate()));
    connect(m_ui->deleteCertificate, SIGNAL(clicked()), this, SLOT(deleteCertificate()));
}

void BlackBerryKeysWidget::apply()
{
    BlackBerryConfiguration &configuration = BlackBerryConfiguration::instance();

    configuration.syncCertificates(m_model->certificates(), m_model->activeCertificate());
}

void BlackBerryKeysWidget::registerKey()
{
    BlackBerryRegisterKeyDialog dialog;

    const int result = dialog.exec();

    if (result != QDialog::Accepted)
        return;

    BlackBerryCertificate *cert = dialog.certificate();

    if (cert) {
        if (!m_model->insertCertificate(cert))
            QMessageBox::information(this, tr("Error"), tr("Could not insert default certificate."));
    }

    updateRegisterSection();
}

void BlackBerryKeysWidget::unregisterKey()
{
    const QMessageBox::StandardButton answer =
        QMessageBox::question(this, tr("Unregister Key"),
        tr("Do you really want to unregister your key? This action cannot be undone."),
        QMessageBox::Yes | QMessageBox::No);

    if (answer & QMessageBox::No)
        return;

    BlackBerryConfiguration &configuration = BlackBerryConfiguration::instance();

    QFile f(configuration.barsignerCskPath());
    f.remove();

    f.setFileName(configuration.barsignerDbPath());
    f.remove();

    updateRegisterSection();
}

void BlackBerryKeysWidget::createCertificate()
{
    BlackBerryCreateCertificateDialog dialog;

    const int result = dialog.exec();

    if (result == QDialog::Rejected)
        return;

    BlackBerryCertificate *cert = dialog.certificate();

    if (cert) {
        if (!m_model->insertCertificate(cert))
            QMessageBox::information(this, tr("Error"), tr("Error storing certificate."));
    }
}

void BlackBerryKeysWidget::importCertificate()
{
    BlackBerryImportCertificateDialog dialog;

    const int result = dialog.exec();

    if (result == QDialog::Rejected)
        return;

    BlackBerryCertificate *cert = dialog.certificate();

    if (cert) {
        if (!m_model->insertCertificate(cert))
            QMessageBox::information(this, tr("Error"), tr("This certificate already exists."));
    }
}

void BlackBerryKeysWidget::deleteCertificate()
{
    const int result = QMessageBox::question(this, tr("Delete Certificate"),
            tr("Are you sure you want to delete this certificate?"),
            QMessageBox::Yes | QMessageBox::No);

    if (result & QMessageBox::No)
        return;

    m_model->removeRow(m_ui->certificatesView->selectionModel()->currentIndex().row());
}

void BlackBerryKeysWidget::handleSelectionChanged(const QItemSelection &selected,
        const QItemSelection &deselected)
{
    Q_UNUSED(deselected);

    m_ui->deleteCertificate->setEnabled(!selected.indexes().isEmpty());
}

void BlackBerryKeysWidget::updateRegisterSection()
{
    if (BlackBerryUtils::hasRegisteredKeys()) {
        m_ui->registerButton->hide();
        m_ui->unregisterButton->show();
        m_ui->registeredLabel->setText(tr("Registered: Yes"));
    } else {
        m_ui->registerButton->show();
        m_ui->unregisterButton->hide();
        m_ui->registeredLabel->setText(tr("Registered: No"));
    }
}

} // namespace Internal
} // namespace Qnx


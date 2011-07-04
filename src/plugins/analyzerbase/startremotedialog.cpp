/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "startremotedialog.h"

#include "ui_startremotedialog.h"

#include <QtGui/QPushButton>

#include <coreplugin/icore.h>

namespace Analyzer {

StartRemoteDialog::StartRemoteDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::StartRemoteDialog)
{
    m_ui->setupUi(this);

    m_ui->keyFile->setExpectedKind(Utils::PathChooser::File);

    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup("AnalyzerStartRemoteDialog");
    m_ui->host->setText(settings->value("host").toString());
    m_ui->port->setValue(settings->value("port", 22).toInt());
    m_ui->user->setText(settings->value("user", qgetenv("USER")).toString());
    m_ui->keyFile->setPath(settings->value("keyFile").toString());
    m_ui->executable->setText(settings->value("executable").toString());
    m_ui->workingDirectory->setText(settings->value("workingDirectory").toString());
    m_ui->arguments->setText(settings->value("arguments").toString());
    settings->endGroup();

    connect(m_ui->host, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(m_ui->port, SIGNAL(valueChanged(int)),
            this, SLOT(validate()));
    connect(m_ui->password, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(m_ui->keyFile, SIGNAL(changed(QString)),
            this, SLOT(validate()));

    connect(m_ui->executable, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(m_ui->workingDirectory, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(m_ui->arguments, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));

    connect(m_ui->buttonBox, SIGNAL(accepted()),
            this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()),
            this, SLOT(reject()));

    validate();
}

StartRemoteDialog::~StartRemoteDialog()
{
    delete m_ui;
}

void StartRemoteDialog::accept()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup("AnalyzerStartRemoteDialog");
    settings->setValue("host", m_ui->host->text());
    settings->setValue("port", m_ui->port->value());
    settings->setValue("user", m_ui->user->text());
    settings->setValue("keyFile", m_ui->keyFile->path());
    settings->setValue("executable", m_ui->executable->text());
    settings->setValue("workingDirectory", m_ui->workingDirectory->text());
    settings->setValue("arguments", m_ui->arguments->text());
    settings->endGroup();

    QDialog::accept();
}

void StartRemoteDialog::validate()
{
    bool valid = !m_ui->host->text().isEmpty() && !m_ui->user->text().isEmpty()
                    && !m_ui->executable->text().isEmpty();
    valid = valid && (!m_ui->password->text().isEmpty() || m_ui->keyFile->isValid());
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

Utils::SshConnectionParameters StartRemoteDialog::sshParams() const
{
    Utils::SshConnectionParameters params(Utils::SshConnectionParameters::NoProxy);
    params.host = m_ui->host->text();
    params.userName = m_ui->user->text();
    if (m_ui->keyFile->isValid()) {
        params.authenticationType = Utils::SshConnectionParameters::AuthenticationByKey;
        params.privateKeyFile = m_ui->keyFile->path();
    } else {
        params.authenticationType = Utils::SshConnectionParameters::AuthenticationByPassword;
        params.password = m_ui->password->text();
    }
    params.port = m_ui->port->value();
    params.timeout = 1;
    return params;
}

QString StartRemoteDialog::executable() const
{
    return m_ui->executable->text();
}

QString StartRemoteDialog::arguments() const
{
    return m_ui->arguments->text();
}

QString StartRemoteDialog::workingDirectory() const
{
    return m_ui->workingDirectory->text();
}

} // namespace Analyzer

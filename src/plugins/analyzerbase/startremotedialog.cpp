/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "startremotedialog.h"

#include <coreplugin/icore.h>
#include <ssh/sshconnection.h>
#include <utils/pathchooser.h>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

namespace Analyzer {
namespace Internal {

class StartRemoteDialogPrivate
{
public:
    QLabel *hostLabel;
    QLineEdit *host;
    QLabel *userLabel;
    QLineEdit *user;
    QLabel *portLabel;
    QSpinBox *port;
    QLabel *passwordLabel;
    QLineEdit *password;
    QLabel *keyFileLabel;
    Utils::PathChooser *keyFile;
    QFormLayout *formLayout;
    QLabel *executableLabel;
    QLineEdit *executable;
    QLabel *argumentsLabel;
    QLineEdit *arguments;
    QLabel *workingDirectoryLabel;
    QLineEdit *workingDirectory;
    QDialogButtonBox *buttonBox;
};

} // namespace Internal

StartRemoteDialog::StartRemoteDialog(QWidget *parent)
    : QDialog(parent)
    , d(new Internal::StartRemoteDialogPrivate)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Start Remote Analysis"));

    QGroupBox *groupBox = new QGroupBox(tr("Remote"), this);

    d->host = new QLineEdit(groupBox);
    d->hostLabel = new QLabel(tr("Host:"), groupBox);

    d->user = new QLineEdit(groupBox);
    d->userLabel = new QLabel(tr("User:"), groupBox);

    d->port = new QSpinBox(groupBox);
    d->port->setMaximum(65535);
    d->port->setSingleStep(1);
    d->port->setValue(22);
    d->portLabel = new QLabel(tr("Port:"), groupBox);

    d->passwordLabel = new QLabel(tr("Password:"), groupBox);
    d->passwordLabel->setToolTip(tr("You need to pass either a password or an SSH key."));

    d->password = new QLineEdit(groupBox);
    d->password->setEchoMode(QLineEdit::Password);

    d->keyFileLabel = new QLabel(tr("Private key:"), groupBox);
    d->keyFileLabel->setToolTip(tr("You need to pass either a password or an SSH key."));

    d->keyFile = new Utils::PathChooser(groupBox);

    QGroupBox *groupBox2 = new QGroupBox(tr("Target"), this);

    d->executable = new QLineEdit(groupBox2);
    d->executableLabel = new QLabel(tr("Executable:"), groupBox2);

    d->arguments = new QLineEdit(groupBox2);
    d->argumentsLabel = new QLabel(tr("Arguments:"), groupBox2);

    d->workingDirectory = new QLineEdit(groupBox2);
    d->workingDirectoryLabel = new QLabel(tr("Working directory:"), groupBox2);

    d->buttonBox = new QDialogButtonBox(this);
    d->buttonBox->setOrientation(Qt::Horizontal);
    d->buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    QFormLayout *formLayout = new QFormLayout(groupBox);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->addRow(d->hostLabel, d->host);
    formLayout->addRow(d->userLabel, d->user);
    formLayout->addRow(d->portLabel, d->port);
    formLayout->addRow(d->passwordLabel, d->password);
    formLayout->addRow(d->keyFileLabel, d->keyFile);

    QFormLayout *formLayout2 = new QFormLayout(groupBox2);
    formLayout2->addRow(d->executableLabel, d->executable);
    formLayout2->addRow(d->argumentsLabel, d->arguments);
    formLayout2->addRow(d->workingDirectoryLabel, d->workingDirectory);

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->addWidget(groupBox);
    verticalLayout->addWidget(groupBox2);
    verticalLayout->addWidget(d->buttonBox);

    d->keyFile->setExpectedKind(Utils::PathChooser::File);

    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String("AnalyzerStartRemoteDialog"));
    d->host->setText(settings->value(QLatin1String("host")).toString());
    d->port->setValue(settings->value(QLatin1String("port"), 22).toInt());
    d->user->setText(settings->value(QLatin1String("user"), qgetenv("USER")).toString());
    d->keyFile->setPath(settings->value(QLatin1String("keyFile")).toString());
    d->executable->setText(settings->value(QLatin1String("executable")).toString());
    d->workingDirectory->setText(settings->value(QLatin1String("workingDirectory")).toString());
    d->arguments->setText(settings->value(QLatin1String("arguments")).toString());
    settings->endGroup();

    connect(d->host, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(d->port, SIGNAL(valueChanged(int)),
            this, SLOT(validate()));
    connect(d->password, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(d->keyFile, SIGNAL(changed(QString)),
            this, SLOT(validate()));

    connect(d->executable, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(d->workingDirectory, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(d->arguments, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));

    connect(d->buttonBox, SIGNAL(accepted()),
            this, SLOT(accept()));
    connect(d->buttonBox, SIGNAL(rejected()),
            this, SLOT(reject()));

    validate();
}

StartRemoteDialog::~StartRemoteDialog()
{
    delete d;
}

void StartRemoteDialog::accept()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String("AnalyzerStartRemoteDialog"));
    settings->setValue(QLatin1String("host"), d->host->text());
    settings->setValue(QLatin1String("port"), d->port->value());
    settings->setValue(QLatin1String("user"), d->user->text());
    settings->setValue(QLatin1String("keyFile"), d->keyFile->path());
    settings->setValue(QLatin1String("executable"), d->executable->text());
    settings->setValue(QLatin1String("workingDirectory"), d->workingDirectory->text());
    settings->setValue(QLatin1String("arguments"), d->arguments->text());
    settings->endGroup();

    QDialog::accept();
}

void StartRemoteDialog::validate()
{
    bool valid = !d->host->text().isEmpty() && !d->user->text().isEmpty()
                    && !d->executable->text().isEmpty();
    valid = valid && (!d->password->text().isEmpty() || d->keyFile->isValid());
    d->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

QSsh::SshConnectionParameters StartRemoteDialog::sshParams() const
{
    QSsh::SshConnectionParameters params;
    params.host = d->host->text();
    params.userName = d->user->text();
    if (d->keyFile->isValid()) {
        params.authenticationType = QSsh::SshConnectionParameters::AuthenticationByKey;
        params.privateKeyFile = d->keyFile->path();
    } else {
        params.authenticationType = QSsh::SshConnectionParameters::AuthenticationByPassword;
        params.password = d->password->text();
    }
    params.port = d->port->value();
    params.timeout = 10;
    return params;
}

QString StartRemoteDialog::executable() const
{
    return d->executable->text();
}

QString StartRemoteDialog::arguments() const
{
    return d->arguments->text();
}

QString StartRemoteDialog::workingDirectory() const
{
    return d->workingDirectory->text();
}

} // namespace Analyzer

/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "window.h"
#include "ui_window.h"

#include "modeltest.h"

#include <utils/ssh/sftpfilesystemmodel.h>
#include <utils/ssh/sshconnection.h>

#include <QApplication>
#include <QMessageBox>
#include <QString>

using namespace Utils;

SftpFsWindow::SftpFsWindow(QWidget *parent) : QDialog(parent), m_ui(new Ui::Window)
{
    m_ui->setupUi(this);
    connect(m_ui->connectButton, SIGNAL(clicked()), SLOT(connectToHost()));
}

SftpFsWindow::~SftpFsWindow()
{
    delete m_ui;
}

void SftpFsWindow::connectToHost()
{
    m_ui->connectButton->setEnabled(false);
    SshConnectionParameters sshParams(SshConnectionParameters::NoProxy);
    sshParams.host = m_ui->hostLineEdit->text();
    sshParams.userName = m_ui->userLineEdit->text();
    sshParams.authenticationType = SshConnectionParameters::AuthenticationByPassword;
    sshParams.password = m_ui->passwordLineEdit->text();
    sshParams.port = m_ui->portSpinBox->value();
    sshParams.timeout = 10;
    SftpFileSystemModel * const fsModel = new SftpFileSystemModel(this);
    if (m_ui->useModelTesterCheckBox->isChecked())
        new ModelTest(fsModel, this);
    connect(fsModel, SIGNAL(sftpOperationFailed(QString)),
        SLOT(handleSftpOperationFailed(QString)));
    connect(fsModel, SIGNAL(connectionError(QString)), SLOT(handleConnectionError(QString)));
    fsModel->setSshConnection(sshParams);
    m_ui->fsView->setModel(fsModel);
}

void SftpFsWindow::handleSftpOperationFailed(const QString &errorMessage)
{
    qDebug("%s: %s", Q_FUNC_INFO, qPrintable(errorMessage));
}

void SftpFsWindow::handleConnectionError(const QString &errorMessage)
{
    QMessageBox::warning(this, tr("Connection Error"),
        tr("Fatal SSH error: %1").arg(errorMessage));
    qApp->quit();
}

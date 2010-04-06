/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "maemosshconfigdialog.h"
#include "maemosshthread.h"

#include <coreplugin/icore.h>
#include <ne7ssh.h>

#include <QtCore/QDir>

using namespace Qt4ProjectManager::Internal;

MaemoSshConfigDialog::MaemoSshConfigDialog(QWidget *parent)
    : QDialog(parent)
    , m_keyDeployer(0)
{
    m_ui.setupUi(this);

    const QLatin1String root("MaemoSsh/");
    QSettings *settings = Core::ICore::instance()->settings();
    m_ui.useKeyFromPath->setChecked(settings->value(root + QLatin1String("userKey"),
        false).toBool());
    m_ui.keyFileLineEdit->setExpectedKind(Utils::PathChooser::File);
    m_ui.keyFileLineEdit->setPath(settings->value(root + QLatin1String("keyPath"),
        QDir::toNativeSeparators(QDir::homePath() + QLatin1String("/.ssh/id_rsa.pub")))
        .toString());

    connect(m_ui.deployButton, SIGNAL(clicked()), this, SLOT(deployKey()));
    connect(m_ui.generateButton, SIGNAL(clicked()), this, SLOT(generateSshKey()));
}

MaemoSshConfigDialog::~MaemoSshConfigDialog()
{
}

void MaemoSshConfigDialog::generateSshKey()
{
    if (!m_ui.keyFileLineEdit->isValid()) {
        ne7ssh ssh;
        ssh.generateKeyPair("rsa", "test", "id_rsa", "id_rsa.pub");
    } else {
        m_ui.infoLabel->setText("An public key has been created already.");
    }
}

void MaemoSshConfigDialog::deployKey()
{
    if (m_keyDeployer)
        return;

    if (m_ui.keyFileLineEdit->validatePath(m_ui.keyFileLineEdit->path())) {
        m_ui.deployButton->disconnect();
        //SshDeploySpec deploySpec(keyFile, homeDirOnDevice(currentConfig().uname)
        //    + QLatin1String("/.ssh/authorized_keys"), true);
        //m_keyDeployer = new MaemoSshDeployer(currentConfig(), QList<SshDeploySpec>()
        //    << deploySpec);
        //connect(m_keyDeployer, SIGNAL(finished()), this,
        //    SLOT(handleDeployThreadFinished()));
        if (m_keyDeployer) {
            m_keyDeployer->start();
            m_ui.deployButton->setText(tr("Stop deploying"));
            connect(m_ui.deployButton, SIGNAL(clicked()), this, SLOT(stopDeploying()));
        }
    } else {
        m_ui.infoLabel->setText("The public key path is invalid.");
    }
}

void MaemoSshConfigDialog::handleDeployThreadFinished()
{
    if (!m_keyDeployer)
        return;

    if (m_keyDeployer->hasError()) {
        m_ui.infoLabel->setText(tr("Key deployment failed: %1")
            .arg(m_keyDeployer->error()));
    } else {
        m_ui.infoLabel->setText(tr("Key was successfully deployed."));
    }
    stopDeploying();
}

void MaemoSshConfigDialog::stopDeploying()
{
    if (m_keyDeployer) {
        m_ui.deployButton->disconnect();
        const bool buttonWasEnabled = m_ui.deployButton->isEnabled();
        m_keyDeployer->disconnect();
        m_keyDeployer->stop();
        delete m_keyDeployer;
        m_keyDeployer = 0;
        m_ui.deployButton->setText(tr("Deploy Public Key"));
        connect(m_ui.deployButton, SIGNAL(clicked()), this, SLOT(deployKey()));
        m_ui.deployButton->setEnabled(buttonWasEnabled);
    }
}

/**************************************************************************
**
** Copyright (c) 2013 Brian McGillion
** Contact: http://www.qt-project.org/legal
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

#include "authenticationdialog.h"
#include "mercurialplugin.h"
#include "srcdestdialog.h"
#include "ui_srcdestdialog.h"

#include <QSettings>
#include <QUrl>

using namespace VcsBase;
using namespace Mercurial::Internal;

SrcDestDialog::SrcDestDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::SrcDestDialog)
{
    m_ui->setupUi(this);
    m_ui->localPathChooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    QUrl repoUrl(getRepoUrl());
    if (repoUrl.isEmpty())
        return;
    if (!repoUrl.password().isEmpty())
        repoUrl.setPassword(QLatin1String("***"));
    m_ui->defaultPath->setText(repoUrl.toString());
    m_ui->promptForCredentials->setChecked(!repoUrl.scheme().isEmpty() && repoUrl.scheme() != QLatin1String("file"));
}

SrcDestDialog::~SrcDestDialog()
{
    delete m_ui;
}

void SrcDestDialog::setPathChooserKind(Utils::PathChooser::Kind kind)
{
    m_ui->localPathChooser->setExpectedKind(kind);
}

QString SrcDestDialog::getRepositoryString() const
{
    if (m_ui->defaultButton->isChecked()) {
        QUrl repoUrl(getRepoUrl());
        if (m_ui->promptForCredentials->isChecked() && !repoUrl.scheme().isEmpty() && repoUrl.scheme() != QLatin1String("file")) {
            QScopedPointer<AuthenticationDialog> authDialog(new AuthenticationDialog(repoUrl.userName(), repoUrl.password()));
            authDialog->setPasswordEnabled(repoUrl.scheme() != QLatin1String("ssh"));
            if (authDialog->exec()== 0)
                return repoUrl.toString();

            QString user = authDialog->getUserName();
            if (user.isEmpty())
                return repoUrl.toString();
            if (user != repoUrl.userName())
                repoUrl.setUserName(user);

            QString pass = authDialog->getPassword();
            if (!pass.isEmpty() && pass != repoUrl.password())
                repoUrl.setPassword(pass);
        }
        return repoUrl.toString();
    }
    if (m_ui->localButton->isChecked())
        return m_ui->localPathChooser->path();
    return m_ui->urlLineEdit->text();
}

QUrl SrcDestDialog::getRepoUrl() const
{
    MercurialPlugin *plugin = MercurialPlugin::instance();
    const VcsBasePluginState state = plugin->currentState();
    QSettings settings(QString(QLatin1String("%1/.hg/hgrc")).arg(state.currentProjectPath()), QSettings::IniFormat);
    return settings.value(QLatin1String("paths/default")).toUrl();
}

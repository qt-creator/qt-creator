/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "authenticationdialog.h"
#include "mercurialplugin.h"
#include "srcdestdialog.h"
#include "ui_srcdestdialog.h"

#include <QSettings>
#include <QUrl>

using namespace VcsBase;

namespace Mercurial {
namespace Internal  {

SrcDestDialog::SrcDestDialog(Direction dir, QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::SrcDestDialog),
    m_direction(dir)
{
    m_ui->setupUi(this);
    m_ui->localPathChooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_ui->localPathChooser->setHistoryCompleter(QLatin1String("Hg.SourceDir.History"));
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

QString SrcDestDialog::workingDir() const
{
    return m_workingdir;
}

QUrl SrcDestDialog::getRepoUrl() const
{
    MercurialPlugin *plugin = MercurialPlugin::instance();
    const VcsBasePluginState state = plugin->currentState();
    // Repo to use: Default to the project repo, but use the current
    const QString projectLoc = state.currentProjectPath();
    const QString fileLoc = state.currentFileTopLevel();
    m_workingdir = projectLoc;
    if (!fileLoc.isEmpty())
        m_workingdir = fileLoc;
    if (!projectLoc.isEmpty() && fileLoc.startsWith(projectLoc + QLatin1Char('/')))
        m_workingdir = projectLoc;
    QSettings settings(QString::fromLatin1("%1/.hg/hgrc").arg(m_workingdir), QSettings::IniFormat);
    QUrl url;
    if (m_direction == outgoing)
        url = settings.value(QLatin1String("paths/default-push")).toUrl();
    if (url.isEmpty())
        url = settings.value(QLatin1String("paths/default")).toUrl();
    return url;
}

} // namespace Internal
} // namespace Mercurial

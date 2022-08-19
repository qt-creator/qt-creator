// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "authenticationdialog.h"
#include "srcdestdialog.h"
#include "ui_srcdestdialog.h"

#include <QSettings>
#include <QUrl>

using namespace VcsBase;

namespace Mercurial {
namespace Internal  {

SrcDestDialog::SrcDestDialog(const VcsBasePluginState &state, Direction dir, QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::SrcDestDialog),
    m_direction(dir),
    m_state(state)
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
        return m_ui->localPathChooser->filePath().toString();
    return m_ui->urlLineEdit->text();
}

Utils::FilePath SrcDestDialog::workingDir() const
{
    return Utils::FilePath::fromString(m_workingdir);
}

QUrl SrcDestDialog::getRepoUrl() const
{
    // Repo to use: Default to the project repo, but use the current
    const QString projectLoc = m_state.currentProjectPath().toString();
    const QString fileLoc = m_state.currentFileTopLevel().toString();
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

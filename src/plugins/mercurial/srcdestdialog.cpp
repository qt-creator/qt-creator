// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "authenticationdialog.h"

#include "mercurialtr.h"
#include "srcdestdialog.h"

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QSettings>
#include <QUrl>

using namespace Utils;
using namespace VcsBase;

namespace Mercurial::Internal {

SrcDestDialog::SrcDestDialog(const VcsBasePluginState &state, Direction dir, QWidget *parent) :
    QDialog(parent),
    m_direction(dir),
    m_state(state)
{
    resize(400, 187);

    m_defaultButton = new QRadioButton(Tr::tr("Default Location"));
    m_defaultButton->setChecked(true);

    m_localButton = new QRadioButton(Tr::tr("Local filesystem:"));

    auto urlButton = new QRadioButton(Tr::tr("Specify URL:"));
    urlButton->setToolTip(Tr::tr("For example: \"https://[user[:pass]@]host[:port]/[path]\"."));

    m_localPathChooser = new Utils::PathChooser;
    m_localPathChooser->setEnabled(false);
    m_localPathChooser->setExpectedKind(PathChooser::ExistingDirectory);
    m_localPathChooser->setHistoryCompleter("Hg.SourceDir.History");

    m_urlLineEdit = new QLineEdit;
    m_urlLineEdit->setToolTip(
        Tr::tr("For example: \"https://[user[:pass]@]host[:port]/[path]\".", nullptr));
    m_urlLineEdit->setEnabled(false);

    QUrl repoUrl = getRepoUrl();
    if (!repoUrl.password().isEmpty())
        repoUrl.setPassword(QLatin1String("***"));

    m_promptForCredentials = new QCheckBox(Tr::tr("Prompt for credentials"));
    m_promptForCredentials->setChecked(!repoUrl.scheme().isEmpty() && repoUrl.scheme() != QLatin1String("file"));

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    using namespace Layouting;

    Column {
        Form {
            m_defaultButton, Column { repoUrl.toString(), m_promptForCredentials }, br,
            m_localButton, m_localPathChooser, br,
            urlButton, m_urlLineEdit, br,
        },
        st,
        buttonBox
    }.attachTo(this);

    connect(urlButton, &QRadioButton::toggled, m_urlLineEdit, &QLineEdit::setEnabled);
    connect(m_localButton, &QAbstractButton::toggled, m_localPathChooser, &QWidget::setEnabled);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

SrcDestDialog::~SrcDestDialog() = default;

void SrcDestDialog::setPathChooserKind(Utils::PathChooser::Kind kind)
{
    m_localPathChooser->setExpectedKind(kind);
}

QString SrcDestDialog::getRepositoryString() const
{
    if (m_defaultButton->isChecked()) {
        QUrl repoUrl(getRepoUrl());
        if (m_promptForCredentials->isChecked() && !repoUrl.scheme().isEmpty() && repoUrl.scheme() != QLatin1String("file")) {
            QScopedPointer<AuthenticationDialog> authDialog(new AuthenticationDialog(repoUrl.userName(), repoUrl.password()));
            authDialog->setPasswordEnabled(repoUrl.scheme() != QLatin1String("ssh"));
            if (authDialog->exec()== 0)
                return repoUrl.toString();

            const QString user = authDialog->getUserName();
            if (user.isEmpty())
                return repoUrl.toString();
            if (user != repoUrl.userName())
                repoUrl.setUserName(user);

            const QString pass = authDialog->getPassword();
            if (!pass.isEmpty() && pass != repoUrl.password())
                repoUrl.setPassword(pass);
        }
        return repoUrl.toString();
    }
    if (m_localButton->isChecked())
        return m_localPathChooser->filePath().toString();
    return m_urlLineEdit->text();
}

FilePath SrcDestDialog::workingDir() const
{
    return FilePath::fromString(m_workingdir);
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

} // Mercurial::Internal

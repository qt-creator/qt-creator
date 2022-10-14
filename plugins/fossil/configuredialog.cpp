/****************************************************************************
**
** Copyright (c) 2018 Artur Shepilko
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

#include "configuredialog.h"
#include "ui_configuredialog.h"

#include "fossilsettings.h"

#include <utils/pathchooser.h>

#include <QDir>

namespace Fossil {
namespace Internal {

class ConfigureDialogPrivate {
public:
    RepositorySettings settings() const
    {
        return {m_ui.userLineEdit->text().trimmed(),
                m_ui.sslIdentityFilePathChooser->filePath().toString(),
                m_ui.disableAutosyncCheckBox->isChecked()
                    ? RepositorySettings::AutosyncOff : RepositorySettings::AutosyncOn};
    }

    void updateUi() {
        m_ui.userLineEdit->setText(m_settings.user.trimmed());
        m_ui.userLineEdit->selectAll();
        m_ui.sslIdentityFilePathChooser->setPath(QDir::toNativeSeparators(m_settings.sslIdentityFile));
        m_ui.disableAutosyncCheckBox->setChecked(m_settings.autosync == RepositorySettings::AutosyncOff);
    }

    Ui::ConfigureDialog m_ui;
    RepositorySettings m_settings;
};

ConfigureDialog::ConfigureDialog(QWidget *parent) : QDialog(parent),
    d(new ConfigureDialogPrivate)
{
    d->m_ui.setupUi(this);
    d->m_ui.sslIdentityFilePathChooser->setExpectedKind(Utils::PathChooser::File);
    d->m_ui.sslIdentityFilePathChooser->setPromptDialogTitle(tr("SSL/TLS Identity Key"));
    setWindowTitle(tr("Configure Repository"));
    d->updateUi();
}

ConfigureDialog::~ConfigureDialog()
{
    delete d;
}

const RepositorySettings ConfigureDialog::settings() const
{
    return d->settings();
}

void ConfigureDialog::setSettings(const RepositorySettings &settings)
{
    d->m_settings = settings;
    d->updateUi();
}

void ConfigureDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        d->m_ui.retranslateUi(this);
        break;
    default:
        break;
    }
}

} // namespace Internal
} // namespace Fossil

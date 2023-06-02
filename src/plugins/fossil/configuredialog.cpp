// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "configuredialog.h"

#include "fossilsettings.h"
#include "fossiltr.h"

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QDialogButtonBox>
#include <QCheckBox>
#include <QDir>
#include <QLineEdit>

namespace Fossil {
namespace Internal {

class ConfigureDialogPrivate {
public:
    RepositorySettings settings() const
    {
        return {m_userLineEdit->text().trimmed(),
                m_sslIdentityFilePathChooser->filePath().toString(),
                m_disableAutosyncCheckBox->isChecked()
                    ? RepositorySettings::AutosyncOff : RepositorySettings::AutosyncOn};
    }

    void updateUi() {
        m_userLineEdit->setText(m_settings.user.trimmed());
        m_userLineEdit->selectAll();
        m_sslIdentityFilePathChooser->setPath(QDir::toNativeSeparators(m_settings.sslIdentityFile));
        m_disableAutosyncCheckBox->setChecked(m_settings.autosync == RepositorySettings::AutosyncOff);
    }

    QLineEdit *m_userLineEdit;
    Utils::PathChooser *m_sslIdentityFilePathChooser;
    QCheckBox *m_disableAutosyncCheckBox;

    RepositorySettings m_settings;
};

ConfigureDialog::ConfigureDialog(QWidget *parent) : QDialog(parent),
    d(new ConfigureDialogPrivate)
{
    setWindowTitle(Tr::tr("Configure Repository"));
    resize(600, 0);

    d->m_userLineEdit = new QLineEdit;
    d->m_userLineEdit->setToolTip(
        Tr::tr("Existing user to become an author of changes made to the repository."));

    d->m_sslIdentityFilePathChooser = new Utils::PathChooser;
    d->m_sslIdentityFilePathChooser->setExpectedKind(Utils::PathChooser::File);
    d->m_sslIdentityFilePathChooser->setPromptDialogTitle(Tr::tr("SSL/TLS Identity Key"));
    d->m_sslIdentityFilePathChooser->setToolTip(
        Tr::tr("SSL/TLS client identity key to use if requested by the server."));

    d->m_disableAutosyncCheckBox = new QCheckBox(Tr::tr("Disable auto-sync"));
    d->m_disableAutosyncCheckBox->setToolTip(
        Tr::tr("Disable automatic pull prior to commit or update and automatic push "
           "after commit or tag or branch creation."));

    auto buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    using namespace Layouting;
    Column {
        Group {
            title(Tr::tr("Repository User")),
            Form { Tr::tr("User:"), d->m_userLineEdit, },
        },
        Group {
            title(Tr::tr("Repository Settings")),
            Form {
                Tr::tr("SSL/TLS identity:"), d->m_sslIdentityFilePathChooser, br,
                d->m_disableAutosyncCheckBox,
            },
        },
        buttonBox,
    }.attachTo(this);

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

} // namespace Internal
} // namespace Fossil

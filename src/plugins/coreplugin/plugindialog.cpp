// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plugindialog.h"

#include "coreplugintr.h"
#include "dialogs/restartdialog.h"
#include "icore.h"
#include "plugininstallwizard.h"

#include <extensionsystem/plugindetailsview.h>
#include <extensionsystem/pluginerrorview.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginview.h>

#include <utils/fancylineedit.h>
#include <utils/layoutbuilder.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>

using namespace Utils;

namespace Core {
namespace Internal {

PluginDialog::PluginDialog(QWidget *parent)
    : QDialog(parent),
      m_view(new ExtensionSystem::PluginView(this))
{
    auto filterEdit = new Utils::FancyLineEdit(this);
    filterEdit->setFocus();
    filterEdit->setFiltering(true);
    connect(filterEdit, &Utils::FancyLineEdit::filterChanged,
            m_view, &ExtensionSystem::PluginView::setFilter);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_detailsButton = buttonBox->addButton(Tr::tr("Details"), QDialogButtonBox::ActionRole);
    m_detailsButton->setEnabled(false);
    m_errorDetailsButton = buttonBox->addButton(Tr::tr("Error Details"),
                                                QDialogButtonBox::ActionRole);
    m_errorDetailsButton->setEnabled(false);
    m_installButton = buttonBox->addButton(Tr::tr("Install Plugin..."),
                                           QDialogButtonBox::ActionRole);

    using namespace Layouting;
    Column {
        filterEdit,
        m_view,
        buttonBox,
    }.attachTo(this);

    resize(760, 440);
    setWindowTitle(Tr::tr("Installed Plugins"));

    connect(m_view, &ExtensionSystem::PluginView::currentPluginChanged,
            this, &PluginDialog::updateButtons);
    connect(m_view, &ExtensionSystem::PluginView::pluginActivated,
            this, &PluginDialog::openDetails);
    connect(m_view, &ExtensionSystem::PluginView::pluginSettingsChanged, this, [this] {
        m_isRestartRequired = true;
    });
    connect(m_detailsButton, &QAbstractButton::clicked, this,
            [this]  { openDetails(m_view->currentPlugin()); });
    connect(m_errorDetailsButton, &QAbstractButton::clicked,
            this, &PluginDialog::openErrorDetails);
    connect(m_installButton, &QAbstractButton::clicked, this, &PluginDialog::showInstallWizard);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &PluginDialog::closeDialog);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(this, &QDialog::rejected, m_view, &ExtensionSystem::PluginView::cancelChanges);
    updateButtons();
}

void PluginDialog::closeDialog()
{
    ExtensionSystem::PluginManager::writeSettings();
    if (m_isRestartRequired) {
        RestartDialog restartDialog(ICore::dialogParent(),
                                    Tr::tr("Plugin changes will take effect after restart."));
        restartDialog.exec();
    }
    accept();
}

void PluginDialog::showInstallWizard()
{
    if (PluginInstallWizard::exec())
        m_isRestartRequired = true;
}

void PluginDialog::updateButtons()
{
    ExtensionSystem::PluginSpec *selectedSpec = m_view->currentPlugin();
    if (selectedSpec) {
        m_detailsButton->setEnabled(true);
        m_errorDetailsButton->setEnabled(selectedSpec->hasError());
    } else {
        m_detailsButton->setEnabled(false);
        m_errorDetailsButton->setEnabled(false);
    }
}

void PluginDialog::openDetails(ExtensionSystem::PluginSpec *spec)
{
    if (!spec)
        return;
    QDialog dialog(this);
    dialog.setWindowTitle(Tr::tr("Plugin Details of %1").arg(spec->name()));
    auto details = new ExtensionSystem::PluginDetailsView(&dialog);
    details->update(spec);
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, &dialog);

    using namespace Layouting;
    Column {
        details,
        buttons,
    }.attachTo(&dialog);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.resize(400, 500);
    dialog.exec();
}

void PluginDialog::openErrorDetails()
{
    ExtensionSystem::PluginSpec *spec = m_view->currentPlugin();
    if (!spec)
        return;
    QDialog dialog(this);
    dialog.setWindowTitle(Tr::tr("Plugin Errors of %1").arg(spec->name()));
    auto errors = new ExtensionSystem::PluginErrorView(&dialog);
    errors->update(spec);
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, &dialog);

    using namespace Layouting;
    Column {
        errors,
        buttons,
    }.attachTo(&dialog);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.resize(500, 300);
    dialog.exec();
}

} // namespace Internal
} // namespace Core

/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "plugindialog.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginview.h>
#include <extensionsystem/plugindetailsview.h>
#include <extensionsystem/pluginerrorview.h>
#include <extensionsystem/pluginspec.h>

#include <utils/fancylineedit.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QDebug>

namespace Core {
namespace Internal {

static bool s_isRestartRequired = false;

PluginDialog::PluginDialog(QWidget *parent)
    : QDialog(parent),
      m_view(new ExtensionSystem::PluginView(this))
{
    QVBoxLayout *vl = new QVBoxLayout(this);

    auto filterEdit = new Utils::FancyLineEdit(this);
    filterEdit->setFiltering(true);
    connect(filterEdit, &Utils::FancyLineEdit::filterChanged,
            m_view, &ExtensionSystem::PluginView::setFilter);
    vl->addWidget(filterEdit);

    vl->addWidget(m_view);

    m_detailsButton = new QPushButton(tr("Details"), this);
    m_errorDetailsButton = new QPushButton(tr("Error Details"), this);
    m_closeButton = new QPushButton(tr("Close"), this);
    m_detailsButton->setEnabled(false);
    m_errorDetailsButton->setEnabled(false);
    m_closeButton->setEnabled(true);
    m_closeButton->setDefault(true);

    m_restartRequired = new QLabel(tr("Restart required."), this);
    if (!s_isRestartRequired)
        m_restartRequired->setVisible(false);

    QHBoxLayout *hl = new QHBoxLayout;
    hl->addWidget(m_detailsButton);
    hl->addWidget(m_errorDetailsButton);
    hl->addSpacing(10);
    hl->addWidget(m_restartRequired);
    hl->addStretch(5);
    hl->addWidget(m_closeButton);

    vl->addLayout(hl);

    resize(650, 400);
    setWindowTitle(tr("Installed Plugins"));

    connect(m_view, &ExtensionSystem::PluginView::currentPluginChanged,
            this, &PluginDialog::updateButtons);
    connect(m_view, &ExtensionSystem::PluginView::pluginActivated,
            this, &PluginDialog::openDetails);
    connect(m_view, &ExtensionSystem::PluginView::pluginSettingsChanged,
            this, &PluginDialog::updateRestartRequired);
    connect(m_detailsButton, &QAbstractButton::clicked,
            [this]  { openDetails(m_view->currentPlugin()); });
    connect(m_errorDetailsButton, &QAbstractButton::clicked,
            this, &PluginDialog::openErrorDetails);
    connect(m_closeButton, &QAbstractButton::clicked,
            this, &PluginDialog::closeDialog);
    updateButtons();
}

void PluginDialog::closeDialog()
{
    ExtensionSystem::PluginManager::writeSettings();
    accept();
}

void PluginDialog::updateRestartRequired()
{
    // just display the notice all the time after once changing something
    s_isRestartRequired = true;
    m_restartRequired->setVisible(true);
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
    dialog.setWindowTitle(tr("Plugin Details of %1").arg(spec->name()));
    QVBoxLayout *layout = new QVBoxLayout;
    dialog.setLayout(layout);
    ExtensionSystem::PluginDetailsView *details = new ExtensionSystem::PluginDetailsView(&dialog);
    layout->addWidget(details);
    details->update(spec);
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, &dialog);
    layout->addWidget(buttons);
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
    dialog.setWindowTitle(tr("Plugin Errors of %1").arg(spec->name()));
    QVBoxLayout *layout = new QVBoxLayout;
    dialog.setLayout(layout);
    ExtensionSystem::PluginErrorView *errors = new ExtensionSystem::PluginErrorView(&dialog);
    layout->addWidget(errors);
    errors->update(spec);
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.resize(500, 300);
    dialog.exec();
}

} // namespace Internal
} // namespace Core

/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "plugindialog.h"

#include <extensionsystem/plugindetailsview.h>
#include <extensionsystem/pluginerrorview.h>
#include <extensionsystem/pluginspec.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QApplication>
#include <QDebug>

PluginDialog::PluginDialog(ExtensionSystem::PluginManager *manager)
    : m_view(new ExtensionSystem::PluginView(manager, this))
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    vl->setMargin(0);
    vl->setSpacing(0);
    vl->addWidget(m_view);

    QHBoxLayout *hl = new QHBoxLayout;
    vl->addLayout(hl);
    hl->setMargin(0);
    hl->setSpacing(6);
    m_detailsButton = new QPushButton(tr("Details"), this);
    m_errorDetailsButton = new QPushButton(tr("Error Details"), this);
    m_detailsButton->setEnabled(false);
    m_errorDetailsButton->setEnabled(false);
    hl->addWidget(m_detailsButton);
    hl->addWidget(m_errorDetailsButton);
    hl->addStretch(5);
    resize(650, 300);
    setWindowTitle(tr("Installed Plugins"));

    connect(m_view, SIGNAL(currentPluginChanged(ExtensionSystem::PluginSpec*)),
                this, SLOT(updateButtons()));
    connect(m_view, SIGNAL(pluginActivated(ExtensionSystem::PluginSpec*)),
                this, SLOT(openDetails(ExtensionSystem::PluginSpec*)));
    connect(m_detailsButton, SIGNAL(clicked()), this, SLOT(openDetails()));
    connect(m_errorDetailsButton, SIGNAL(clicked()), this, SLOT(openErrorDetails()));
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


void PluginDialog::openDetails()
{
        openDetails(m_view->currentPlugin());
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
    connect(buttons, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), &dialog, SLOT(reject()));
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
    connect(buttons, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), &dialog, SLOT(reject()));
    dialog.resize(500, 300);
    dialog.exec();
}

int main(int argc, char *argv[])
{
    ExtensionSystem::PluginManager manager;
    QApplication app(argc, argv);
    PluginDialog dialog(&manager);
    manager.setPluginPaths(QStringList() << "plugins");
    manager.loadPlugins();
    dialog.show();
    app.exec();
}

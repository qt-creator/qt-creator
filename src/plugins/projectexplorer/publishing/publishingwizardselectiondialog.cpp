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
#include "publishingwizardselectiondialog.h"
#include "ui_publishingwizardselectiondialog.h"

#include "ipublishingwizardfactory.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/project.h>
#include <utils/qtcassert.h>

#include <QPushButton>

namespace ProjectExplorer {
namespace Internal {

PublishingWizardSelectionDialog::PublishingWizardSelectionDialog(const Project *project,
        QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PublishingWizardSelectionDialog),
    m_project(project)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Start Wizard"));
    const QList<IPublishingWizardFactory *> &factories
        = ExtensionSystem::PluginManager::getObjects<IPublishingWizardFactory>();
    foreach (const IPublishingWizardFactory * const factory, factories) {
        if (factory->canCreateWizard(project)) {
            m_factories << factory;
            ui->serviceComboBox->addItem(factory->displayName());
        }
    }
    if (!m_factories.isEmpty()) {
        connect(ui->serviceComboBox, SIGNAL(currentIndexChanged(int)),
            SLOT(handleWizardIndexChanged(int)));
        ui->serviceComboBox->setCurrentIndex(0);
        handleWizardIndexChanged(ui->serviceComboBox->currentIndex());
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->descriptionTextArea->appendHtml(QLatin1String("<font color=\"red\">")
            + tr("Publishing is currently not possible for project '%1'.")
                  .arg(project->displayName())
            + QLatin1String("</font>"));
    }
}

PublishingWizardSelectionDialog::~PublishingWizardSelectionDialog()
{
    delete ui;
}

QWizard *PublishingWizardSelectionDialog::createSelectedWizard() const
{
    return m_factories.at(ui->serviceComboBox->currentIndex())->createWizard(m_project);
}

void PublishingWizardSelectionDialog::handleWizardIndexChanged(int index)
{
    ui->descriptionTextArea->setPlainText(m_factories.at(index)->description());
}

} // namespace Internal
} // namespace ProjectExplorer

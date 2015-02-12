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
#include "devicefactoryselectiondialog.h"
#include "ui_devicefactoryselectiondialog.h"

#include "idevice.h"
#include "idevicefactory.h"

#include <coreplugin/id.h>

#include <extensionsystem/pluginmanager.h>

#include <QPushButton>

namespace ProjectExplorer {
namespace Internal {

DeviceFactorySelectionDialog::DeviceFactorySelectionDialog(QWidget *parent) :
    QDialog(parent), ui(new Ui::DeviceFactorySelectionDialog)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Start Wizard"));

    const QList<IDeviceFactory *> &factories
        = ExtensionSystem::PluginManager::getObjects<IDeviceFactory>();
    foreach (const IDeviceFactory * const factory, factories) {
        if (!factory->canCreate())
            continue;
        foreach (Core::Id id, factory->availableCreationIds()) {
            QListWidgetItem *item = new QListWidgetItem(factory->displayNameForId(id));
            item->setData(Qt::UserRole, QVariant::fromValue(id));
            ui->listWidget->addItem(item);
        }
    }

    connect(ui->listWidget, SIGNAL(itemSelectionChanged()), SLOT(handleItemSelectionChanged()));
    connect(ui->listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            SLOT(handleItemDoubleClicked()));
    handleItemSelectionChanged();
}

DeviceFactorySelectionDialog::~DeviceFactorySelectionDialog()
{
    delete ui;
}

void DeviceFactorySelectionDialog::handleItemSelectionChanged()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!ui->listWidget->selectedItems().isEmpty());
}

void DeviceFactorySelectionDialog::handleItemDoubleClicked()
{
    accept();
}

Core::Id DeviceFactorySelectionDialog::selectedId() const
{
    QList<QListWidgetItem *> selected = ui->listWidget->selectedItems();
    if (selected.isEmpty())
        return Core::Id();
    return selected.at(0)->data(Qt::UserRole).value<Core::Id>();
}

} // namespace Internal
} // namespace ProjectExplorer

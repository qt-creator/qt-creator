/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "devicefactoryselectiondialog.h"
#include "ui_devicefactoryselectiondialog.h"

#include "idevice.h"
#include "idevicefactory.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

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

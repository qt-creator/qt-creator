// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicefactoryselectiondialog.h"

#include "idevicefactory.h"
#include "../projectexplorertr.h"

#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>

#include <QDialogButtonBox>
#include <QListWidget>
#include <QPushButton>

namespace ProjectExplorer {
namespace Internal {

DeviceFactorySelectionDialog::DeviceFactorySelectionDialog(QWidget *parent) :
    QDialog(parent)
{
    resize(420, 330);
    m_listWidget = new QListWidget;
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText(Tr::tr("Start Wizard"));

    using namespace Layouting;
    Column {
        Tr::tr("Available device types:"),
        m_listWidget,
        m_buttonBox,
    }.attachTo(this);

    for (const IDeviceFactory * const factory : IDeviceFactory::allDeviceFactories()) {
        if (!factory->canCreate())
            continue;
        QListWidgetItem *item = new QListWidgetItem(factory->displayName());
        item->setData(Qt::UserRole, QVariant::fromValue(factory->deviceType()));
        m_listWidget->addItem(item);
    }

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &DeviceFactorySelectionDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &DeviceFactorySelectionDialog::reject);
    connect(m_listWidget, &QListWidget::itemSelectionChanged,
            this, &DeviceFactorySelectionDialog::handleItemSelectionChanged);
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, &DeviceFactorySelectionDialog::handleItemDoubleClicked);
    handleItemSelectionChanged();
}

void DeviceFactorySelectionDialog::handleItemSelectionChanged()
{
    m_buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!m_listWidget->selectedItems().isEmpty());
}

void DeviceFactorySelectionDialog::handleItemDoubleClicked()
{
    accept();
}

Utils::Id DeviceFactorySelectionDialog::selectedId() const
{
    QList<QListWidgetItem *> selected = m_listWidget->selectedItems();
    if (selected.isEmpty())
        return Utils::Id();
    return selected.at(0)->data(Qt::UserRole).value<Utils::Id>();
}

} // namespace Internal
} // namespace ProjectExplorer

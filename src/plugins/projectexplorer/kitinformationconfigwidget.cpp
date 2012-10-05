/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "kitinformationconfigwidget.h"

#include "devicesupport/devicemanager.h"
#include "devicesupport/devicemanagermodel.h"
#include "devicesupport/idevicefactory.h"
#include "projectexplorerconstants.h"
#include "kit.h"
#include "kitinformation.h"
#include "toolchain.h"
#include "toolchainmanager.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/pathchooser.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// SysRootInformationConfigWidget:
// --------------------------------------------------------------------------

SysRootInformationConfigWidget::SysRootInformationConfigWidget(Kit *k, QWidget *parent) :
    KitConfigWidget(parent),
    m_kit(k)
{
    setToolTip(tr("The root directory of the system image to use.<br>"
                  "Leave empty when building for the desktop."));
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    m_chooser = new Utils::PathChooser;
    m_chooser->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_chooser);
    m_chooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);

    m_chooser->setFileName(SysRootKitInformation::sysRoot(k));

    connect(m_chooser, SIGNAL(changed(QString)), this, SIGNAL(dirty()));
}

QString SysRootInformationConfigWidget::displayName() const
{
    return tr("Sysroot:");
}

void SysRootInformationConfigWidget::apply()
{
    SysRootKitInformation::setSysRoot(m_kit, m_chooser->fileName());
}

void SysRootInformationConfigWidget::discard()
{
    m_chooser->setFileName(SysRootKitInformation::sysRoot(m_kit));
}

bool SysRootInformationConfigWidget::isDirty() const
{
    return SysRootKitInformation::sysRoot(m_kit) != m_chooser->fileName();
}

void SysRootInformationConfigWidget::makeReadOnly()
{
    m_chooser->setEnabled(false);
}

QWidget *SysRootInformationConfigWidget::buttonWidget() const
{
    return m_chooser->buttonAtIndex(0);
}

// --------------------------------------------------------------------------
// ToolChainInformationConfigWidget:
// --------------------------------------------------------------------------

ToolChainInformationConfigWidget::ToolChainInformationConfigWidget(Kit *k, QWidget *parent) :
    KitConfigWidget(parent),
    m_isReadOnly(false), m_kit(k),
    m_comboBox(new QComboBox), m_manageButton(new QPushButton(this))
{
    setToolTip(tr("The compiler to use for building.<br>"
                  "Make sure the compiler will produce binaries compatible with the target device, "
                  "Qt version and other libraries used."));
    ToolChainManager *tcm = ToolChainManager::instance();

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    m_comboBox->setContentsMargins(0, 0, 0, 0);
    m_comboBox->setEnabled(false);
    m_comboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    layout->addWidget(m_comboBox);

    foreach (ToolChain *tc, tcm->toolChains())
        toolChainAdded(tc);

    updateComboBox();

    discard();
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(dirty()));

    m_manageButton->setContentsMargins(0, 0, 0, 0);
    m_manageButton->setText(tr("Manage..."));
    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(manageToolChains()));

    connect(tcm, SIGNAL(toolChainAdded(ProjectExplorer::ToolChain*)),
            this, SLOT(toolChainAdded(ProjectExplorer::ToolChain*)));
    connect(tcm, SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
            this, SLOT(toolChainRemoved(ProjectExplorer::ToolChain*)));
    connect(tcm, SIGNAL(toolChainUpdated(ProjectExplorer::ToolChain*)),
            this, SLOT(toolChainUpdated(ProjectExplorer::ToolChain*)));
}

QString ToolChainInformationConfigWidget::displayName() const
{
    return tr("Compiler:");
}

void ToolChainInformationConfigWidget::apply()
{
    const QString id = m_comboBox->itemData(m_comboBox->currentIndex()).toString();
    ToolChain *tc = ToolChainManager::instance()->findToolChain(id);
    ToolChainKitInformation::setToolChain(m_kit, tc);
}

void ToolChainInformationConfigWidget::discard()
{
    m_comboBox->setCurrentIndex(indexOf(ToolChainKitInformation::toolChain(m_kit)));
}

bool ToolChainInformationConfigWidget::isDirty() const
{
    ToolChain *tc = ToolChainKitInformation::toolChain(m_kit);
    return (m_comboBox->itemData(m_comboBox->currentIndex()).toString())
            != (tc ? tc->id() : QString());
}

void ToolChainInformationConfigWidget::makeReadOnly()
{
    m_comboBox->setEnabled(false);
}

QWidget *ToolChainInformationConfigWidget::buttonWidget() const
{
    return m_manageButton;
}

void ToolChainInformationConfigWidget::toolChainAdded(ProjectExplorer::ToolChain *tc)
{
    m_comboBox->addItem(tc->displayName(), tc->id());
    updateComboBox();
}

void ToolChainInformationConfigWidget::toolChainRemoved(ProjectExplorer::ToolChain *tc)
{
    const int pos = indexOf(tc);
    if (pos < 0)
        return;
    m_comboBox->removeItem(pos);
    updateComboBox();
}
void ToolChainInformationConfigWidget::toolChainUpdated(ProjectExplorer::ToolChain *tc)
{
    const int pos = indexOf(tc);
    if (pos < 0)
        return;
    m_comboBox->setItemText(pos, tc->displayName());
}

void ToolChainInformationConfigWidget::manageToolChains()
{
    Core::ICore::showOptionsDialog(QLatin1String(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY),
                                   QLatin1String(ProjectExplorer::Constants::TOOLCHAIN_SETTINGS_PAGE_ID));
}

void ToolChainInformationConfigWidget::updateComboBox()
{
    // remove unavailable tool chain:
    int pos = indexOf(0);
    if (pos >= 0)
        m_comboBox->removeItem(pos);

    if (m_comboBox->count() == 0) {
        m_comboBox->addItem(tr("<No compiler available>"), QString());
        m_comboBox->setEnabled(false);
    } else {
        m_comboBox->setEnabled(!m_isReadOnly);
    }
}

int ToolChainInformationConfigWidget::indexOf(const ToolChain *tc)
{
    const QString id = tc ? tc->id() : QString();
    for (int i = 0; i < m_comboBox->count(); ++i) {
        if (id == m_comboBox->itemData(i).toString())
            return i;
    }
    return -1;
}

// --------------------------------------------------------------------------
// DeviceTypeInformationConfigWidget:
// --------------------------------------------------------------------------

DeviceTypeInformationConfigWidget::DeviceTypeInformationConfigWidget(Kit *k, QWidget *parent) :
    KitConfigWidget(parent),
    m_isReadOnly(false), m_kit(k),
    m_comboBox(new QComboBox)
{
    setToolTip(tr("The type of device to run applications on."));
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    m_comboBox->setContentsMargins(0, 0, 0, 0);
    m_comboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    layout->addWidget(m_comboBox);

    QList<IDeviceFactory *> factories
            = ExtensionSystem::PluginManager::instance()->getObjects<IDeviceFactory>();
    foreach (IDeviceFactory *factory, factories) {
        foreach (Core::Id id, factory->availableCreationIds()) {
            m_comboBox->addItem(factory->displayNameForId(id), QVariant::fromValue(id));
        }
    }

    discard();
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(dirty()));
}

QString DeviceTypeInformationConfigWidget::displayName() const
{
    return tr("Device type:");
}

void DeviceTypeInformationConfigWidget::apply()
{
    Core::Id devType;
    if (m_comboBox->currentIndex() >= 0)
        devType = m_comboBox->itemData(m_comboBox->currentIndex()).value<Core::Id>();
    DeviceTypeKitInformation::setDeviceTypeId(m_kit, devType);
}

void DeviceTypeInformationConfigWidget::discard()
{
    Core::Id devType = DeviceTypeKitInformation::deviceTypeId(m_kit);
    if (!devType.isValid())
        m_comboBox->setCurrentIndex(-1);
    for (int i = 0; i < m_comboBox->count(); ++i) {
        if (m_comboBox->itemData(i).value<Core::Id>() == devType) {
            m_comboBox->setCurrentIndex(i);
            break;
        }
    }
}

bool DeviceTypeInformationConfigWidget::isDirty() const
{
    Core::Id devType;
    if (m_comboBox->currentIndex() >= 0)
        devType = m_comboBox->itemData(m_comboBox->currentIndex()).value<Core::Id>();
    return DeviceTypeKitInformation::deviceTypeId(m_kit) != devType;
}

void DeviceTypeInformationConfigWidget::makeReadOnly()
{
    m_comboBox->setEnabled(false);
}

// --------------------------------------------------------------------------
// DeviceInformationConfigWidget:
// --------------------------------------------------------------------------

DeviceInformationConfigWidget::DeviceInformationConfigWidget(Kit *k, QWidget *parent) :
    KitConfigWidget(parent),
    m_isReadOnly(false), m_kit(k),
    m_comboBox(new QComboBox), m_manageButton(new QPushButton(this)),
    m_model(new DeviceManagerModel(DeviceManager::instance()))
{
    connect(m_model, SIGNAL(modelAboutToBeReset()), SLOT(modelAboutToReset()));
    connect(m_model, SIGNAL(modelReset()), SLOT(modelReset()));

    setToolTip(tr("The device to run the applications on."));

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    m_comboBox->setContentsMargins(0, 0, 0, 0);
    m_comboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    layout->addWidget(m_comboBox);

    m_comboBox->setModel(m_model);

    m_manageButton->setContentsMargins(0, 0, 0, 0);
    m_manageButton->setText(tr("Manage..."));

    discard();
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(dirty()));

    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(manageDevices()));
}

QString DeviceInformationConfigWidget::displayName() const
{
    return tr("Device:");
}

void DeviceInformationConfigWidget::apply()
{
    int idx = m_comboBox->currentIndex();
    if (idx >= 0)
        DeviceKitInformation::setDeviceId(m_kit, m_model->deviceId(idx));
    else
        DeviceKitInformation::setDeviceId(m_kit, IDevice::invalidId());
}

void DeviceInformationConfigWidget::discard()
{
    m_comboBox->setCurrentIndex(m_model->indexOf(DeviceKitInformation::device(m_kit)));
}

bool DeviceInformationConfigWidget::isDirty() const
{
    Core::Id devId = DeviceKitInformation::deviceId(m_kit);
    return devId != m_model->deviceId(m_comboBox->currentIndex());
}

void DeviceInformationConfigWidget::makeReadOnly()
{
    m_comboBox->setEnabled(false);
}

QWidget *DeviceInformationConfigWidget::buttonWidget() const
{
    return m_manageButton;
}

void DeviceInformationConfigWidget::manageDevices()
{
    Core::ICore::showOptionsDialog(QLatin1String(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY),
                                   QLatin1String(ProjectExplorer::Constants::DEVICE_SETTINGS_PAGE_ID));
}

void DeviceInformationConfigWidget::modelAboutToReset()
{
    m_selectedId = m_model->deviceId(m_comboBox->currentIndex());
}

void DeviceInformationConfigWidget::modelReset()
{
    m_comboBox->setCurrentIndex(m_model->indexForId(m_selectedId));
}

} // namespace Internal
} // namespace ProjectExplorer

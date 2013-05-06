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
#include <utils/fancylineedit.h>

#include <QComboBox>
#include <QPushButton>

using namespace Core;

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// SysRootInformationConfigWidget:
// --------------------------------------------------------------------------

SysRootInformationConfigWidget::SysRootInformationConfigWidget(Kit *k, bool sticky) :
    KitConfigWidget(k, sticky),
    m_ignoreChange(false)
{
    m_chooser = new Utils::PathChooser;
    m_chooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_chooser->setFileName(SysRootKitInformation::sysRoot(k));
    connect(m_chooser, SIGNAL(changed(QString)), this, SLOT(pathWasChanged()));
}

QString SysRootInformationConfigWidget::displayName() const
{
    return tr("Sysroot:");
}

QString SysRootInformationConfigWidget::toolTip() const
{
    return tr("The root directory of the system image to use.<br>"
                  "Leave empty when building for the desktop.");
}

void SysRootInformationConfigWidget::refresh()
{
    if (!m_ignoreChange)
        m_chooser->setFileName(SysRootKitInformation::sysRoot(m_kit));
}

void SysRootInformationConfigWidget::makeReadOnly()
{
    m_chooser->setReadOnly(true);
}

QWidget *SysRootInformationConfigWidget::mainWidget() const
{
    return m_chooser->lineEdit();
}

QWidget *SysRootInformationConfigWidget::buttonWidget() const
{
    return m_chooser->buttonAtIndex(0);
}

void SysRootInformationConfigWidget::pathWasChanged()
{
    m_ignoreChange = true;
    SysRootKitInformation::setSysRoot(m_kit, m_chooser->fileName());
    m_ignoreChange = false;
}

// --------------------------------------------------------------------------
// ToolChainInformationConfigWidget:
// --------------------------------------------------------------------------

ToolChainInformationConfigWidget::ToolChainInformationConfigWidget(Kit *k, bool sticky) :
    KitConfigWidget(k, sticky), m_isReadOnly(false)
{
    ToolChainManager *tcm = ToolChainManager::instance();

    m_comboBox = new QComboBox;
    m_comboBox->setEnabled(false);

    foreach (ToolChain *tc, tcm->toolChains())
        toolChainAdded(tc);

    updateComboBox();

    refresh();
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentToolChainChanged(int)));

    m_manageButton = new QPushButton(tr("Manage..."));
    m_manageButton->setContentsMargins(0, 0, 0, 0);
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

QString ToolChainInformationConfigWidget::toolTip() const
{
    return tr("The compiler to use for building.<br>"
              "Make sure the compiler will produce binaries compatible with the target device, "
              "Qt version and other libraries used.");
}

void ToolChainInformationConfigWidget::refresh()
{
    m_comboBox->setCurrentIndex(indexOf(ToolChainKitInformation::toolChain(m_kit)));
}

void ToolChainInformationConfigWidget::makeReadOnly()
{
    m_comboBox->setEnabled(false);
}

QWidget *ToolChainInformationConfigWidget::mainWidget() const
{
    return m_comboBox;
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
    Core::ICore::showOptionsDialog(Constants::PROJECTEXPLORER_SETTINGS_CATEGORY,
                                   Constants::TOOLCHAIN_SETTINGS_PAGE_ID);
}

void ToolChainInformationConfigWidget::currentToolChainChanged(int idx)
{
    const QString id = m_comboBox->itemData(idx).toString();
    ToolChain *tc = ToolChainManager::instance()->findToolChain(id);
    ToolChainKitInformation::setToolChain(m_kit, tc);
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

DeviceTypeInformationConfigWidget::DeviceTypeInformationConfigWidget(Kit *workingCopy, bool sticky) :
    KitConfigWidget(workingCopy, sticky), m_isReadOnly(false), m_comboBox(new QComboBox)
{
    QList<IDeviceFactory *> factories
            = ExtensionSystem::PluginManager::instance()->getObjects<IDeviceFactory>();
    foreach (IDeviceFactory *factory, factories) {
        foreach (Core::Id id, factory->availableCreationIds())
            m_comboBox->addItem(factory->displayNameForId(id), id.uniqueIdentifier());
    }

    refresh();
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentTypeChanged(int)));
}

QWidget *DeviceTypeInformationConfigWidget::mainWidget() const
{
    return m_comboBox;
}

QString DeviceTypeInformationConfigWidget::displayName() const
{
    return tr("Device type:");
}

QString DeviceTypeInformationConfigWidget::toolTip() const
{
    return tr("The type of device to run applications on.");
}

void DeviceTypeInformationConfigWidget::refresh()
{
    Core::Id devType = DeviceTypeKitInformation::deviceTypeId(m_kit);
    if (!devType.isValid())
        m_comboBox->setCurrentIndex(-1);
    for (int i = 0; i < m_comboBox->count(); ++i) {
        if (m_comboBox->itemData(i).toInt() == devType.uniqueIdentifier()) {
            m_comboBox->setCurrentIndex(i);
            break;
        }
    }
}

void DeviceTypeInformationConfigWidget::makeReadOnly()
{
    m_comboBox->setEnabled(false);
}

void DeviceTypeInformationConfigWidget::currentTypeChanged(int idx)
{
    Core::Id type = idx < 0 ? Core::Id() : Core::Id::fromUniqueIdentifier(m_comboBox->itemData(idx).toInt());
    DeviceTypeKitInformation::setDeviceTypeId(m_kit, type);
}

// --------------------------------------------------------------------------
// DeviceInformationConfigWidget:
// --------------------------------------------------------------------------

DeviceInformationConfigWidget::DeviceInformationConfigWidget(Kit *workingCopy, bool sticky) :
    KitConfigWidget(workingCopy, sticky),
    m_isReadOnly(false),
    m_ignoreChange(false),
    m_comboBox(new QComboBox),
    m_model(new DeviceManagerModel(DeviceManager::instance()))
{
    m_comboBox->setModel(m_model);

    m_manageButton = new QPushButton(tr("Manage"));

    refresh();
    connect(m_model, SIGNAL(modelAboutToBeReset()), SLOT(modelAboutToReset()));
    connect(m_model, SIGNAL(modelReset()), SLOT(modelReset()));
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentDeviceChanged()));
    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(manageDevices()));
}

QWidget *DeviceInformationConfigWidget::mainWidget() const
{
    return m_comboBox;
}

QString DeviceInformationConfigWidget::displayName() const
{
    return tr("Device:");
}

QString DeviceInformationConfigWidget::toolTip() const
{
    return tr("The device to run the applications on.");
}

void DeviceInformationConfigWidget::refresh()
{
    m_model->setTypeFilter(DeviceTypeKitInformation::deviceTypeId(m_kit));
    m_comboBox->setCurrentIndex(m_model->indexOf(DeviceKitInformation::device(m_kit)));
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
    ICore::showOptionsDialog(Constants::DEVICE_SETTINGS_CATEGORY,
                             Constants::DEVICE_SETTINGS_PAGE_ID);
}

void DeviceInformationConfigWidget::modelAboutToReset()
{
    m_selectedId = m_model->deviceId(m_comboBox->currentIndex());
    m_ignoreChange = true;
}

void DeviceInformationConfigWidget::modelReset()
{
    m_comboBox->setCurrentIndex(m_model->indexForId(m_selectedId));
    m_ignoreChange = false;
}

void DeviceInformationConfigWidget::currentDeviceChanged()
{
    if (m_ignoreChange)
        return;
    DeviceKitInformation::setDeviceId(m_kit, m_model->deviceId(m_comboBox->currentIndex()));
}

} // namespace Internal
} // namespace ProjectExplorer

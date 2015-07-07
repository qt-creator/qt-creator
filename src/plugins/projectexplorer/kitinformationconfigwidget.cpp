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

#include "kitinformationconfigwidget.h"

#include "devicesupport/devicemanager.h"
#include "devicesupport/devicemanagermodel.h"
#include "devicesupport/idevicefactory.h"
#include "projectexplorerconstants.h"
#include "kit.h"
#include "kitinformation.h"
#include "toolchain.h"
#include "toolchainmanager.h"
#include "environmentwidget.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontMetrics>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Core;

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// SysRootInformationConfigWidget:
// --------------------------------------------------------------------------

SysRootInformationConfigWidget::SysRootInformationConfigWidget(Kit *k, const KitInformation *ki) :
    KitConfigWidget(k, ki),
    m_ignoreChange(false)
{
    m_chooser = new Utils::PathChooser;
    m_chooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_chooser->setHistoryCompleter(QLatin1String("PE.SysRoot.History"));
    m_chooser->setFileName(SysRootKitInformation::sysRoot(k));
    connect(m_chooser, SIGNAL(changed(QString)), this, SLOT(pathWasChanged()));
}

SysRootInformationConfigWidget::~SysRootInformationConfigWidget()
{
    delete m_chooser;
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

ToolChainInformationConfigWidget::ToolChainInformationConfigWidget(Kit *k, const KitInformation *ki) :
    KitConfigWidget(k, ki),
    m_ignoreChanges(false),
    m_isReadOnly(false)
{
    m_comboBox = new QComboBox;
    m_comboBox->setToolTip(toolTip());

    refresh();
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentToolChainChanged(int)));

    m_manageButton = new QPushButton(KitConfigWidget::msgManage());
    m_manageButton->setContentsMargins(0, 0, 0, 0);
    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(manageToolChains()));
}

ToolChainInformationConfigWidget::~ToolChainInformationConfigWidget()
{
    delete m_comboBox;
    delete m_manageButton;
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
    m_ignoreChanges = true;
    m_comboBox->clear();
    foreach (ToolChain *tc, ToolChainManager::toolChains())
        m_comboBox->addItem(tc->displayName(), tc->id());

    if (m_comboBox->count() == 0) {
        m_comboBox->addItem(tr("<No compiler available>"), QString());
        m_comboBox->setEnabled(false);
    } else {
        m_comboBox->setEnabled(m_comboBox->count() > 1 && !m_isReadOnly);
    }

    m_comboBox->setCurrentIndex(indexOf(ToolChainKitInformation::toolChain(m_kit)));
    m_ignoreChanges = false;
}

void ToolChainInformationConfigWidget::makeReadOnly()
{
    m_isReadOnly = true;
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

void ToolChainInformationConfigWidget::manageToolChains()
{
    ICore::showOptionsDialog(Constants::TOOLCHAIN_SETTINGS_PAGE_ID, buttonWidget());
}

void ToolChainInformationConfigWidget::currentToolChainChanged(int idx)
{
    if (m_ignoreChanges)
        return;

    const QByteArray id = m_comboBox->itemData(idx).toByteArray();
    ToolChainKitInformation::setToolChain(m_kit, ToolChainManager::findToolChain(id));
}

int ToolChainInformationConfigWidget::indexOf(const ToolChain *tc)
{
    const QByteArray id = tc ? tc->id() : QByteArray();
    for (int i = 0; i < m_comboBox->count(); ++i) {
        if (id == m_comboBox->itemData(i).toByteArray())
            return i;
    }
    return -1;
}

// --------------------------------------------------------------------------
// DeviceTypeInformationConfigWidget:
// --------------------------------------------------------------------------

DeviceTypeInformationConfigWidget::DeviceTypeInformationConfigWidget(Kit *workingCopy, const KitInformation *ki) :
    KitConfigWidget(workingCopy, ki), m_comboBox(new QComboBox)
{
    QList<IDeviceFactory *> factories
            = ExtensionSystem::PluginManager::getObjects<IDeviceFactory>();
    foreach (IDeviceFactory *factory, factories) {
        foreach (Id id, factory->availableCreationIds())
            m_comboBox->addItem(factory->displayNameForId(id), id.toSetting());
    }

    m_comboBox->setToolTip(toolTip());

    refresh();
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentTypeChanged(int)));
}

DeviceTypeInformationConfigWidget::~DeviceTypeInformationConfigWidget()
{
    delete m_comboBox;
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
    Id devType = DeviceTypeKitInformation::deviceTypeId(m_kit);
    if (!devType.isValid())
        m_comboBox->setCurrentIndex(-1);
    for (int i = 0; i < m_comboBox->count(); ++i) {
        if (m_comboBox->itemData(i) == devType.toSetting()) {
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
    Id type = idx < 0 ? Id() : Id::fromSetting(m_comboBox->itemData(idx));
    DeviceTypeKitInformation::setDeviceTypeId(m_kit, type);
}

// --------------------------------------------------------------------------
// DeviceInformationConfigWidget:
// --------------------------------------------------------------------------

DeviceInformationConfigWidget::DeviceInformationConfigWidget(Kit *workingCopy, const KitInformation *ki) :
    KitConfigWidget(workingCopy, ki),
    m_isReadOnly(false),
    m_ignoreChange(false),
    m_comboBox(new QComboBox),
    m_model(new DeviceManagerModel(DeviceManager::instance()))
{
    m_comboBox->setModel(m_model);

    m_manageButton = new QPushButton(KitConfigWidget::msgManage());

    refresh();
    m_comboBox->setToolTip(toolTip());

    connect(m_model, SIGNAL(modelAboutToBeReset()), SLOT(modelAboutToReset()));
    connect(m_model, SIGNAL(modelReset()), SLOT(modelReset()));
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentDeviceChanged()));
    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(manageDevices()));
}

DeviceInformationConfigWidget::~DeviceInformationConfigWidget()
{
    delete m_comboBox;
    delete m_model;
    delete m_manageButton;
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
    ICore::showOptionsDialog(Constants::DEVICE_SETTINGS_PAGE_ID, buttonWidget());
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

// --------------------------------------------------------------------
// KitEnvironmentConfigWidget:
// --------------------------------------------------------------------

KitEnvironmentConfigWidget::KitEnvironmentConfigWidget(Kit *workingCopy, const KitInformation *ki) :
    KitConfigWidget(workingCopy, ki),
    m_summaryLabel(new QLabel),
    m_manageButton(new QPushButton),
    m_dialog(0),
    m_editor(0)
{
    refresh();
    m_manageButton->setText(tr("Change..."));
    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(editEnvironmentChanges()));
}

QWidget *KitEnvironmentConfigWidget::mainWidget() const
{
    return m_summaryLabel;
}

QString KitEnvironmentConfigWidget::displayName() const
{
    return tr("Environment:");
}

QString KitEnvironmentConfigWidget::toolTip() const
{
    return tr("Additional environment settings when using this kit.");
}

void KitEnvironmentConfigWidget::refresh()
{
    QList<Utils::EnvironmentItem> changes = EnvironmentKitInformation::environmentChanges(m_kit);
    Utils::sort(changes, [](const Utils::EnvironmentItem &lhs, const Utils::EnvironmentItem &rhs)
                         { return QString::localeAwareCompare(lhs.name, rhs.name) < 0; });
    QString shortSummary = Utils::EnvironmentItem::toStringList(changes).join(QLatin1String("; "));
    QFontMetrics fm(m_summaryLabel->font());
    shortSummary = fm.elidedText(shortSummary, Qt::ElideRight, m_summaryLabel->width());
    m_summaryLabel->setText(shortSummary.isEmpty() ? tr("No changes to apply.") : shortSummary);
    if (m_editor)
        m_editor->setPlainText(Utils::EnvironmentItem::toStringList(changes).join(QLatin1Char('\n')));
}

void KitEnvironmentConfigWidget::makeReadOnly()
{
    m_manageButton->setEnabled(false);
    if (m_dialog)
        m_dialog->reject();
}

void KitEnvironmentConfigWidget::editEnvironmentChanges()
{
    if (m_dialog) {
        m_dialog->activateWindow();
        m_dialog->raise();
        return;
    }

    QTC_ASSERT(!m_editor, return);

    m_dialog = new QDialog(m_summaryLabel);
    m_dialog->setWindowTitle(tr("Edit Environment Changes"));
    QVBoxLayout *layout = new QVBoxLayout(m_dialog);
    m_editor = new QPlainTextEdit;
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Apply|QDialogButtonBox::Cancel);

    layout->addWidget(m_editor);
    layout->addWidget(buttons);

    connect(buttons, SIGNAL(accepted()), m_dialog, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), m_dialog, SLOT(reject()));
    connect(m_dialog, SIGNAL(accepted()), this, SLOT(acceptChangesDialog()));
    connect(m_dialog, SIGNAL(rejected()), this, SLOT(closeChangesDialog()));
    connect(buttons->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(applyChanges()));

    refresh();
    m_dialog->show();
}

void KitEnvironmentConfigWidget::applyChanges()
{
    QTC_ASSERT(m_editor, return);
    auto changes = Utils::EnvironmentItem::fromStringList(m_editor->toPlainText().split(QLatin1Char('\n')));
    EnvironmentKitInformation::setEnvironmentChanges(m_kit, changes);
}

void KitEnvironmentConfigWidget::closeChangesDialog()
{
    m_dialog->deleteLater();
    m_dialog = 0;
    m_editor = 0;
}

void KitEnvironmentConfigWidget::acceptChangesDialog()
{
    applyChanges();
    closeChangesDialog();
}

QWidget *KitEnvironmentConfigWidget::buttonWidget() const
{
    return m_manageButton;
}

} // namespace Internal
} // namespace ProjectExplorer

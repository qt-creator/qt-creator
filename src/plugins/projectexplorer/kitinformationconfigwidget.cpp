/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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
#include <utils/environmentdialog.h>

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
    KitConfigWidget(k, ki)
{
    m_chooser = new Utils::PathChooser;
    m_chooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_chooser->setHistoryCompleter(QLatin1String("PE.SysRoot.History"));
    m_chooser->setFileName(SysRootKitInformation::sysRoot(k));
    connect(m_chooser, &Utils::PathChooser::pathChanged,
            this, &SysRootInformationConfigWidget::pathWasChanged);
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

void SysRootInformationConfigWidget::setPalette(const QPalette &p)
{
    KitConfigWidget::setPalette(p);
    m_chooser->setOkColor(p.color(QPalette::Active, QPalette::Text));
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
    KitConfigWidget(k, ki)
{
    m_mainWidget = new QWidget;
    m_mainWidget->setContentsMargins(0, 0, 0, 0);

    auto layout = new QGridLayout(m_mainWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setColumnStretch(1, 2);

    int row = 0;
    QList<Core::Id> languageList = ToolChainManager::allLanguages().toList();
    Utils::sort(languageList, [](Core::Id l1, Core::Id l2) {
        return ToolChainManager::displayNameOfLanguageId(l1) < ToolChainManager::displayNameOfLanguageId(l2);
    });

    QTC_ASSERT(!languageList.isEmpty(), return);

    foreach (Core::Id l, languageList) {
        layout->addWidget(new QLabel(ToolChainManager::displayNameOfLanguageId(l) + ':'), row, 0);
        auto cb = new QComboBox;
        cb->setToolTip(toolTip());

        m_languageComboboxMap.insert(l, cb);
        layout->addWidget(cb, row, 1);
        ++row;

        connect(cb, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, [this, l](int idx) { currentToolChainChanged(l, idx); });
    }

    refresh();

    m_manageButton = new QPushButton(KitConfigWidget::msgManage());
    m_manageButton->setContentsMargins(0, 0, 0, 0);
    connect(m_manageButton, &QAbstractButton::clicked,
            this, &ToolChainInformationConfigWidget::manageToolChains);
}

ToolChainInformationConfigWidget::~ToolChainInformationConfigWidget()
{
    delete m_mainWidget;
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

    foreach (Core::Id l, m_languageComboboxMap.keys()) {
        const QList<ToolChain *> ltcList
                = ToolChainManager::toolChains(Utils::equal(&ToolChain::language, l));

        QComboBox *cb = m_languageComboboxMap.value(l);
        cb->clear();
        cb->addItem(tr("<No compiler>"), QByteArray());

        foreach (ToolChain *tc, ltcList)
            cb->addItem(tc->displayName(), tc->id());

        cb->setEnabled(cb->count() > 1 && !m_isReadOnly);
        const int index = indexOf(cb, ToolChainKitInformation::toolChain(m_kit, l));
        cb->setCurrentIndex(index);
    }
    m_ignoreChanges = false;
}

void ToolChainInformationConfigWidget::makeReadOnly()
{
    m_isReadOnly = true;
    foreach (Core::Id l, m_languageComboboxMap.keys()) {
        m_languageComboboxMap.value(l)->setEnabled(false);
    }
}

QWidget *ToolChainInformationConfigWidget::mainWidget() const
{
    return m_mainWidget;
}

QWidget *ToolChainInformationConfigWidget::buttonWidget() const
{
    return m_manageButton;
}

void ToolChainInformationConfigWidget::manageToolChains()
{
    ICore::showOptionsDialog(Constants::TOOLCHAIN_SETTINGS_PAGE_ID, buttonWidget());
}

void ToolChainInformationConfigWidget::currentToolChainChanged(Id language, int idx)
{
    if (m_ignoreChanges || idx < 0)
        return;

    const QByteArray id = m_languageComboboxMap.value(language)->itemData(idx).toByteArray();
    ToolChain *tc = ToolChainManager::findToolChain(id);
    QTC_ASSERT(!tc || tc->language() == language, return);
    if (tc)
        ToolChainKitInformation::setToolChain(m_kit, tc);
    else
        ToolChainKitInformation::clearToolChain(m_kit, language);
}

int ToolChainInformationConfigWidget::indexOf(QComboBox *cb, const ToolChain *tc)
{
    const QByteArray id = tc ? tc->id() : QByteArray();
    for (int i = 0; i < cb->count(); ++i) {
        if (id == cb->itemData(i).toByteArray())
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
    connect(m_comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &DeviceTypeInformationConfigWidget::currentTypeChanged);
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
    m_comboBox(new QComboBox),
    m_model(new DeviceManagerModel(DeviceManager::instance()))
{
    m_comboBox->setModel(m_model);

    m_manageButton = new QPushButton(KitConfigWidget::msgManage());

    refresh();
    m_comboBox->setToolTip(toolTip());

    connect(m_model, &QAbstractItemModel::modelAboutToBeReset,
            this, &DeviceInformationConfigWidget::modelAboutToReset);
    connect(m_model, &QAbstractItemModel::modelReset,
            this, &DeviceInformationConfigWidget::modelReset);
    connect(m_comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &DeviceInformationConfigWidget::currentDeviceChanged);
    connect(m_manageButton, &QAbstractButton::clicked,
            this, &DeviceInformationConfigWidget::manageDevices);
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
    m_manageButton(new QPushButton)
{
    refresh();
    m_manageButton->setText(tr("Change..."));
    connect(m_manageButton, &QAbstractButton::clicked,
            this, &KitEnvironmentConfigWidget::editEnvironmentChanges);
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
    return tr("Additional build environment settings when using this kit.");
}

void KitEnvironmentConfigWidget::refresh()
{
    const QList<Utils::EnvironmentItem> changes = currentEnvironment();
    QString shortSummary = Utils::EnvironmentItem::toStringList(changes).join(QLatin1String("; "));
    QFontMetrics fm(m_summaryLabel->font());
    shortSummary = fm.elidedText(shortSummary, Qt::ElideRight, m_summaryLabel->width());
    m_summaryLabel->setText(shortSummary.isEmpty() ? tr("No changes to apply.") : shortSummary);
}

void KitEnvironmentConfigWidget::makeReadOnly()
{
    m_manageButton->setEnabled(false);
}

QList<Utils::EnvironmentItem> KitEnvironmentConfigWidget::currentEnvironment() const
{
    QList<Utils::EnvironmentItem> changes = EnvironmentKitInformation::environmentChanges(m_kit);
    Utils::sort(changes, [](const Utils::EnvironmentItem &lhs, const Utils::EnvironmentItem &rhs)
                         { return QString::localeAwareCompare(lhs.name, rhs.name) < 0; });
    return changes;
}

void KitEnvironmentConfigWidget::editEnvironmentChanges()
{
    bool ok;
    const QList<Utils::EnvironmentItem> changes = Utils::EnvironmentDialog::getEnvironmentItems(&ok,
                                                                 m_summaryLabel,
                                                                 currentEnvironment());
    if (!ok)
        return;

    EnvironmentKitInformation::setEnvironmentChanges(m_kit, changes);
}

QWidget *KitEnvironmentConfigWidget::buttonWidget() const
{
    return m_manageButton;
}

} // namespace Internal
} // namespace ProjectExplorer

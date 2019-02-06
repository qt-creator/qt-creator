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
#include <coreplugin/variablechooser.h>

#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>
#include <utils/environmentdialog.h>

#include <QCheckBox>
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
// SysRootKitAspectWidget:
// --------------------------------------------------------------------------

SysRootKitAspectWidget::SysRootKitAspectWidget(Kit *k, const KitAspect *ki) :
    KitAspectWidget(k, ki)
{
    m_chooser = new Utils::PathChooser;
    m_chooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_chooser->setHistoryCompleter(QLatin1String("PE.SysRoot.History"));
    m_chooser->setFileName(SysRootKitAspect::sysRoot(k));
    connect(m_chooser, &Utils::PathChooser::pathChanged,
            this, &SysRootKitAspectWidget::pathWasChanged);
}

SysRootKitAspectWidget::~SysRootKitAspectWidget()
{
    delete m_chooser;
}

QString SysRootKitAspectWidget::displayName() const
{
    return tr("Sysroot");
}

QString SysRootKitAspectWidget::toolTip() const
{
    return tr("The root directory of the system image to use.<br>"
              "Leave empty when building for the desktop.");
}

void SysRootKitAspectWidget::setPalette(const QPalette &p)
{
    KitAspectWidget::setPalette(p);
    m_chooser->setOkColor(p.color(QPalette::Active, QPalette::Text));
}

void SysRootKitAspectWidget::refresh()
{
    if (!m_ignoreChange)
        m_chooser->setFileName(SysRootKitAspect::sysRoot(m_kit));
}

void SysRootKitAspectWidget::makeReadOnly()
{
    m_chooser->setReadOnly(true);
}

QWidget *SysRootKitAspectWidget::mainWidget() const
{
    return m_chooser->lineEdit();
}

QWidget *SysRootKitAspectWidget::buttonWidget() const
{
    return m_chooser->buttonAtIndex(0);
}

void SysRootKitAspectWidget::pathWasChanged()
{
    m_ignoreChange = true;
    SysRootKitAspect::setSysRoot(m_kit, m_chooser->fileName());
    m_ignoreChange = false;
}

// --------------------------------------------------------------------------
// ToolChainKitAspectWidget:
// --------------------------------------------------------------------------

ToolChainKitAspectWidget::ToolChainKitAspectWidget(Kit *k, const KitAspect *ki) :
    KitAspectWidget(k, ki)
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
        cb->setSizePolicy(QSizePolicy::Ignored, cb->sizePolicy().verticalPolicy());
        cb->setToolTip(toolTip());

        m_languageComboboxMap.insert(l, cb);
        layout->addWidget(cb, row, 1);
        ++row;

        connect(cb, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, [this, l](int idx) { currentToolChainChanged(l, idx); });
    }

    refresh();

    m_manageButton = new QPushButton(KitAspectWidget::msgManage());
    m_manageButton->setContentsMargins(0, 0, 0, 0);
    connect(m_manageButton, &QAbstractButton::clicked,
            this, &ToolChainKitAspectWidget::manageToolChains);
}

ToolChainKitAspectWidget::~ToolChainKitAspectWidget()
{
    delete m_mainWidget;
    delete m_manageButton;
}

QString ToolChainKitAspectWidget::displayName() const
{
    return tr("Compiler");
}

QString ToolChainKitAspectWidget::toolTip() const
{
    return tr("The compiler to use for building.<br>"
              "Make sure the compiler will produce binaries compatible with the target device, "
              "Qt version and other libraries used.");
}

void ToolChainKitAspectWidget::refresh()
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
        const int index = indexOf(cb, ToolChainKitAspect::toolChain(m_kit, l));
        cb->setCurrentIndex(index);
    }
    m_ignoreChanges = false;
}

void ToolChainKitAspectWidget::makeReadOnly()
{
    m_isReadOnly = true;
    foreach (Core::Id l, m_languageComboboxMap.keys()) {
        m_languageComboboxMap.value(l)->setEnabled(false);
    }
}

QWidget *ToolChainKitAspectWidget::mainWidget() const
{
    return m_mainWidget;
}

QWidget *ToolChainKitAspectWidget::buttonWidget() const
{
    return m_manageButton;
}

void ToolChainKitAspectWidget::manageToolChains()
{
    ICore::showOptionsDialog(Constants::TOOLCHAIN_SETTINGS_PAGE_ID, buttonWidget());
}

void ToolChainKitAspectWidget::currentToolChainChanged(Id language, int idx)
{
    if (m_ignoreChanges || idx < 0)
        return;

    const QByteArray id = m_languageComboboxMap.value(language)->itemData(idx).toByteArray();
    ToolChain *tc = ToolChainManager::findToolChain(id);
    QTC_ASSERT(!tc || tc->language() == language, return);
    if (tc)
        ToolChainKitAspect::setToolChain(m_kit, tc);
    else
        ToolChainKitAspect::clearToolChain(m_kit, language);
}

int ToolChainKitAspectWidget::indexOf(QComboBox *cb, const ToolChain *tc)
{
    const QByteArray id = tc ? tc->id() : QByteArray();
    for (int i = 0; i < cb->count(); ++i) {
        if (id == cb->itemData(i).toByteArray())
            return i;
    }
    return -1;
}

// --------------------------------------------------------------------------
// DeviceTypeKitAspectWidget:
// --------------------------------------------------------------------------

DeviceTypeKitAspectWidget::DeviceTypeKitAspectWidget(Kit *workingCopy, const KitAspect *ki) :
    KitAspectWidget(workingCopy, ki), m_comboBox(new QComboBox)
{
    for (IDeviceFactory *factory : IDeviceFactory::allDeviceFactories())
        m_comboBox->addItem(factory->displayName(), factory->deviceType().toSetting());

    m_comboBox->setToolTip(toolTip());

    refresh();
    connect(m_comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &DeviceTypeKitAspectWidget::currentTypeChanged);
}

DeviceTypeKitAspectWidget::~DeviceTypeKitAspectWidget()
{
    delete m_comboBox;
}

QWidget *DeviceTypeKitAspectWidget::mainWidget() const
{
    return m_comboBox;
}

QString DeviceTypeKitAspectWidget::displayName() const
{
    return tr("Device type");
}

QString DeviceTypeKitAspectWidget::toolTip() const
{
    return tr("The type of device to run applications on.");
}

void DeviceTypeKitAspectWidget::refresh()
{
    Id devType = DeviceTypeKitAspect::deviceTypeId(m_kit);
    if (!devType.isValid())
        m_comboBox->setCurrentIndex(-1);
    for (int i = 0; i < m_comboBox->count(); ++i) {
        if (m_comboBox->itemData(i) == devType.toSetting()) {
            m_comboBox->setCurrentIndex(i);
            break;
        }
    }
}

void DeviceTypeKitAspectWidget::makeReadOnly()
{
    m_comboBox->setEnabled(false);
}

void DeviceTypeKitAspectWidget::currentTypeChanged(int idx)
{
    Id type = idx < 0 ? Id() : Id::fromSetting(m_comboBox->itemData(idx));
    DeviceTypeKitAspect::setDeviceTypeId(m_kit, type);
}

// --------------------------------------------------------------------------
// DeviceKitAspectWidget:
// --------------------------------------------------------------------------

DeviceKitAspectWidget::DeviceKitAspectWidget(Kit *workingCopy, const KitAspect *ki) :
    KitAspectWidget(workingCopy, ki),
    m_comboBox(new QComboBox),
    m_model(new DeviceManagerModel(DeviceManager::instance()))
{
    m_comboBox->setSizePolicy(QSizePolicy::Ignored, m_comboBox->sizePolicy().verticalPolicy());
    m_comboBox->setModel(m_model);

    m_manageButton = new QPushButton(KitAspectWidget::msgManage());

    refresh();
    m_comboBox->setToolTip(toolTip());

    connect(m_model, &QAbstractItemModel::modelAboutToBeReset,
            this, &DeviceKitAspectWidget::modelAboutToReset);
    connect(m_model, &QAbstractItemModel::modelReset,
            this, &DeviceKitAspectWidget::modelReset);
    connect(m_comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &DeviceKitAspectWidget::currentDeviceChanged);
    connect(m_manageButton, &QAbstractButton::clicked,
            this, &DeviceKitAspectWidget::manageDevices);
}

DeviceKitAspectWidget::~DeviceKitAspectWidget()
{
    delete m_comboBox;
    delete m_model;
    delete m_manageButton;
}

QWidget *DeviceKitAspectWidget::mainWidget() const
{
    return m_comboBox;
}

QString DeviceKitAspectWidget::displayName() const
{
    return tr("Device");
}

QString DeviceKitAspectWidget::toolTip() const
{
    return tr("The device to run the applications on.");
}

void DeviceKitAspectWidget::refresh()
{
    m_model->setTypeFilter(DeviceTypeKitAspect::deviceTypeId(m_kit));
    m_comboBox->setCurrentIndex(m_model->indexOf(DeviceKitAspect::device(m_kit)));
}

void DeviceKitAspectWidget::makeReadOnly()
{
    m_comboBox->setEnabled(false);
}

QWidget *DeviceKitAspectWidget::buttonWidget() const
{
    return m_manageButton;
}

void DeviceKitAspectWidget::manageDevices()
{
    ICore::showOptionsDialog(Constants::DEVICE_SETTINGS_PAGE_ID, buttonWidget());
}

void DeviceKitAspectWidget::modelAboutToReset()
{
    m_selectedId = m_model->deviceId(m_comboBox->currentIndex());
    m_ignoreChange = true;
}

void DeviceKitAspectWidget::modelReset()
{
    m_comboBox->setCurrentIndex(m_model->indexForId(m_selectedId));
    m_ignoreChange = false;
}

void DeviceKitAspectWidget::currentDeviceChanged()
{
    if (m_ignoreChange)
        return;
    DeviceKitAspect::setDeviceId(m_kit, m_model->deviceId(m_comboBox->currentIndex()));
}

// --------------------------------------------------------------------
// EnvironmentKitAspectWidget:
// --------------------------------------------------------------------

EnvironmentKitAspectWidget::EnvironmentKitAspectWidget(Kit *workingCopy, const KitAspect *ki) :
    KitAspectWidget(workingCopy, ki),
    m_summaryLabel(new QLabel),
    m_manageButton(new QPushButton),
    m_mainWidget(new QWidget)
{
    auto *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_summaryLabel);
    if (Utils::HostOsInfo::isWindowsHost())
        initMSVCOutputSwitch(layout);

    m_mainWidget->setLayout(layout);

    refresh();
    m_manageButton->setText(tr("Change..."));
    connect(m_manageButton, &QAbstractButton::clicked,
            this, &EnvironmentKitAspectWidget::editEnvironmentChanges);
}

QWidget *EnvironmentKitAspectWidget::mainWidget() const
{
    return m_mainWidget;
}

QString EnvironmentKitAspectWidget::displayName() const
{
    return tr("Environment");
}

QString EnvironmentKitAspectWidget::toolTip() const
{
    return tr("Additional build environment settings when using this kit.");
}

void EnvironmentKitAspectWidget::refresh()
{
    const QList<Utils::EnvironmentItem> changes = currentEnvironment();
    QString shortSummary = Utils::EnvironmentItem::toStringList(changes).join(QLatin1String("; "));
    QFontMetrics fm(m_summaryLabel->font());
    shortSummary = fm.elidedText(shortSummary, Qt::ElideRight, m_summaryLabel->width());
    m_summaryLabel->setText(shortSummary.isEmpty() ? tr("No changes to apply.") : shortSummary);
}

void EnvironmentKitAspectWidget::makeReadOnly()
{
    m_manageButton->setEnabled(false);
}

QList<Utils::EnvironmentItem> EnvironmentKitAspectWidget::currentEnvironment() const
{
    QList<Utils::EnvironmentItem> changes = EnvironmentKitAspect::environmentChanges(m_kit);

    if (Utils::HostOsInfo::isWindowsHost()) {
        const Utils::EnvironmentItem forceMSVCEnglishItem("VSLANG", "1033");
        if (changes.indexOf(forceMSVCEnglishItem) >= 0) {
            m_vslangCheckbox->setCheckState(Qt::Checked);
            changes.removeAll(forceMSVCEnglishItem);
        }
    }

    Utils::sort(changes, [](const Utils::EnvironmentItem &lhs, const Utils::EnvironmentItem &rhs)
                         { return QString::localeAwareCompare(lhs.name, rhs.name) < 0; });
    return changes;
}

void EnvironmentKitAspectWidget::editEnvironmentChanges()
{
    bool ok;
    Utils::MacroExpander *expander = m_kit->macroExpander();
    Utils::EnvironmentDialog::Polisher polisher = [expander](QWidget *w) {
        Core::VariableChooser::addSupportForChildWidgets(w, expander);
    };
    QList<Utils::EnvironmentItem>
            changes = Utils::EnvironmentDialog::getEnvironmentItems(&ok,
                                                                    m_summaryLabel,
                                                                    currentEnvironment(),
                                                                    QString(),
                                                                    polisher);
    if (!ok)
        return;

    if (Utils::HostOsInfo::isWindowsHost()) {
        const Utils::EnvironmentItem forceMSVCEnglishItem("VSLANG", "1033");
        if (m_vslangCheckbox->isChecked() && changes.indexOf(forceMSVCEnglishItem) < 0)
            changes.append(forceMSVCEnglishItem);
    }

    EnvironmentKitAspect::setEnvironmentChanges(m_kit, changes);
}

QWidget *EnvironmentKitAspectWidget::buttonWidget() const
{
    return m_manageButton;
}

void EnvironmentKitAspectWidget::initMSVCOutputSwitch(QVBoxLayout *layout)
{
    m_vslangCheckbox = new QCheckBox(tr("Force UTF-8 MSVC compiler output"));
    layout->addWidget(m_vslangCheckbox);
    m_vslangCheckbox->setToolTip(tr("Either switches MSVC to English or keeps the language and "
                                    "just forces UTF-8 output (may vary depending on the used MSVC "
                                    "compiler)."));
    connect(m_vslangCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
        QList<Utils::EnvironmentItem> changes
                = EnvironmentKitAspect::environmentChanges(m_kit);
        const Utils::EnvironmentItem forceMSVCEnglishItem("VSLANG", "1033");
        if (!checked && changes.indexOf(forceMSVCEnglishItem) >= 0)
            changes.removeAll(forceMSVCEnglishItem);
        if (checked && changes.indexOf(forceMSVCEnglishItem) < 0)
            changes.append(forceMSVCEnglishItem);
        EnvironmentKitAspect::setEnvironmentChanges(m_kit, changes);
    });
}

} // namespace Internal
} // namespace ProjectExplorer

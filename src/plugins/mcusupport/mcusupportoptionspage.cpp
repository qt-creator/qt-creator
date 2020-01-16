/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com)
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

#include "mcusupportconstants.h"
#include "mcusupportoptionspage.h"
#include "mcusupportoptions.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>
#include <coreplugin/icore.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/algorithm.h>
#include <utils/infolabel.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QComboBox>
#include <QDir>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

namespace McuSupport {
namespace Internal {

static bool cMakeAvailable()
{
    return !CMakeProjectManager::CMakeToolManager::cmakeTools().isEmpty();
}

class McuSupportOptionsWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(McuSupport::Internal::McuSupportOptionsWidget)

public:
    McuSupportOptionsWidget();

    void updateStatus();
    void showMcuTargetPackages();
    McuTarget *currentMcuTarget() const;

private:
    void apply() final;

    void showEvent(QShowEvent *event) final;

    QString m_armGccPath;
    McuSupportOptions m_options;
    QMap <McuPackage*, QWidget*> m_packageWidgets;
    QMap <McuTarget*, QWidget*> m_mcuTargetPacketWidgets;
    QFormLayout *m_packagesLayout = nullptr;
    Utils::InfoLabel *m_infoLabel = nullptr;
    QComboBox *m_mcuTargetComboBox = nullptr;
};

McuSupportOptionsWidget::McuSupportOptionsWidget()
{
    auto mainLayout = new QVBoxLayout(this);

    auto mcuTargetChooserlayout = new QHBoxLayout;
    auto mcuTargetChooserLabel = new QLabel(tr("Target:"));
    mcuTargetChooserlayout->addWidget(mcuTargetChooserLabel);
    m_mcuTargetComboBox = new QComboBox;
    mcuTargetChooserLabel->setBuddy(m_mcuTargetComboBox);
    mcuTargetChooserLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    m_mcuTargetComboBox->addItems(
                Utils::transform<QStringList>(m_options.mcuTargets, [this](McuTarget *t){
                    return m_options.kitName(t);
                }));
    mcuTargetChooserlayout->addWidget(m_mcuTargetComboBox);
    mainLayout->addLayout(mcuTargetChooserlayout);

    auto m_packagesGroupBox = new QGroupBox(tr("Packages"));
    mainLayout->addWidget(m_packagesGroupBox);
    m_packagesLayout = new QFormLayout;
    m_packagesGroupBox->setLayout(m_packagesLayout);

    mainLayout->addStretch();

    m_infoLabel = new Utils::InfoLabel;
    m_infoLabel->setOpenExternalLinks(false);
    m_infoLabel->setElideMode(Qt::ElideNone);
    m_infoLabel->setWordWrap(true);
    mainLayout->addWidget(m_infoLabel);

    connect(&m_options, &McuSupportOptions::changed, this, &McuSupportOptionsWidget::updateStatus);
    connect(m_mcuTargetComboBox, &QComboBox::currentTextChanged,
            this, &McuSupportOptionsWidget::showMcuTargetPackages);
    connect(m_infoLabel, &QLabel::linkActivated, this, []{
                Core::ICore::showOptionsDialog(
                            CMakeProjectManager::Constants::CMAKE_SETTINGSPAGE_ID,
                            Core::ICore::mainWindow());
            });

    showMcuTargetPackages();
}

void McuSupportOptionsWidget::updateStatus()
{
    const McuTarget *mcuTarget = currentMcuTarget();
    if (!mcuTarget)
        return;

    m_infoLabel->setType(cMakeAvailable() && mcuTarget->isValid()
                         ? Utils::InfoLabel::Ok : Utils::InfoLabel::NotOk);

    QStringList errorStrings;
    if (!mcuTarget->isValid())
        errorStrings << "Provide the package paths in order to create a kit for your target.";
    if (!cMakeAvailable())
        errorStrings << "No CMake tool was detected. Add a CMake tool in the "
                        "<a href=\"cmake\">CMake options</a> and press Apply.";

    m_infoLabel->setText(errorStrings.isEmpty()
                ? QString::fromLatin1("A kit <b>%1</b> for the selected target can be generated. "
                                      "Press Apply to generate it.").arg(m_options.kitName(
                                                                             mcuTarget))
                : errorStrings.join("<br/>"));
}

void McuSupportOptionsWidget::showMcuTargetPackages()
{
    const McuTarget *mcuTarget = currentMcuTarget();
    if (!mcuTarget)
        return;

    while (m_packagesLayout->rowCount() > 0) {
        QFormLayout::TakeRowResult row = m_packagesLayout->takeRow(0);
        row.labelItem->widget()->hide();
        row.fieldItem->widget()->hide();
    }

    for (auto package : m_options.packages) {
        QWidget *packageWidget = package->widget();
        if (!mcuTarget->packages().contains(package))
            continue;
        m_packagesLayout->addRow(package->label(), packageWidget);
        packageWidget->show();
    }

    updateStatus();
}

McuTarget *McuSupportOptionsWidget::currentMcuTarget() const
{
    const int mcuTargetIndex = m_mcuTargetComboBox->currentIndex();
    return m_options.mcuTargets.isEmpty() ? nullptr : m_options.mcuTargets.at(mcuTargetIndex);
}

void McuSupportOptionsWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    updateStatus();
}

void McuSupportOptionsWidget::apply()
{
    for (auto package : m_options.packages)
        package->writeToSettings();

    QTC_ASSERT(m_options.armGccPackage, return);
    QTC_ASSERT(m_options.qtForMCUsSdkPackage, return);

    if (!isVisible() || !cMakeAvailable())
        return;

    const McuTarget *mcuTarget = currentMcuTarget();
    if (!mcuTarget)
        return;

    for (auto existingKit : m_options.existingKits(mcuTarget))
        ProjectExplorer::KitManager::deregisterKit(existingKit);
    m_options.newKit(mcuTarget);
}

McuSupportOptionsPage::McuSupportOptionsPage()
{
    setId(Core::Id(Constants::SETTINGS_ID));
    setDisplayName(McuSupportOptionsWidget::tr("MCU"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new McuSupportOptionsWidget; });
}

} // Internal
} // McuSupport

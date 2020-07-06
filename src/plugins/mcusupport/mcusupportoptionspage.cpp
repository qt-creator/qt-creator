/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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
#include <QPushButton>
#include <QVBoxLayout>

namespace McuSupport {
namespace Internal {

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

    void populateMcuTargetsComboBox();
    void showEvent(QShowEvent *event) final;

    QString m_armGccPath;
    McuSupportOptions m_options;
    QMap <McuPackage*, QWidget*> m_packageWidgets;
    QMap <McuTarget*, QWidget*> m_mcuTargetPacketWidgets;
    QFormLayout *m_packagesLayout = nullptr;
    QGroupBox *m_qtForMCUsSdkGroupBox = nullptr;
    QGroupBox *m_packagesGroupBox = nullptr;
    QGroupBox *m_mcuTargetsGroupBox = nullptr;
    QComboBox *m_mcuTargetsComboBox = nullptr;
    QGroupBox *m_kitCreationGroupBox = nullptr;
    Utils::InfoLabel *m_kitCreationInfoLabel = nullptr;
    Utils::InfoLabel *m_statusInfoLabel = nullptr;
    QPushButton *m_kitCreationPushButton = nullptr;
    QPushButton *m_kitRemovalPushButton = nullptr;
};

McuSupportOptionsWidget::McuSupportOptionsWidget()
{
    auto mainLayout = new QVBoxLayout(this);

    {
        m_statusInfoLabel = new Utils::InfoLabel;
        m_statusInfoLabel->setElideMode(Qt::ElideNone);
        m_statusInfoLabel->setOpenExternalLinks(false);
        mainLayout->addWidget(m_statusInfoLabel);
        connect(m_statusInfoLabel, &QLabel::linkActivated, this, [] {
            Core::ICore::showOptionsDialog(CMakeProjectManager::Constants::CMAKE_SETTINGS_PAGE_ID);
        });
    }

    {
        m_qtForMCUsSdkGroupBox = new QGroupBox(m_options.qtForMCUsSdkPackage->label());
        m_qtForMCUsSdkGroupBox->setFlat(true);
        auto layout = new QVBoxLayout(m_qtForMCUsSdkGroupBox);
        layout->addWidget(m_options.qtForMCUsSdkPackage->widget());
        mainLayout->addWidget(m_qtForMCUsSdkGroupBox);
    }

    {
        m_mcuTargetsGroupBox = new QGroupBox(tr("Targets supported by the %1")
                                             .arg(m_qtForMCUsSdkGroupBox->title()));
        m_mcuTargetsGroupBox->setFlat(true);
        mainLayout->addWidget(m_mcuTargetsGroupBox);
        m_mcuTargetsComboBox = new QComboBox;
        auto layout = new QVBoxLayout(m_mcuTargetsGroupBox);
        layout->addWidget(m_mcuTargetsComboBox);
        connect(m_mcuTargetsComboBox, &QComboBox::currentTextChanged,
                this, &McuSupportOptionsWidget::showMcuTargetPackages);
        connect(m_options.qtForMCUsSdkPackage, &McuPackage::changed,
                this, &McuSupportOptionsWidget::populateMcuTargetsComboBox);
    }

    {
        m_packagesGroupBox = new QGroupBox(tr("Requirements"));
        m_packagesGroupBox->setFlat(true);
        mainLayout->addWidget(m_packagesGroupBox);
        m_packagesLayout = new QFormLayout;
        m_packagesGroupBox->setLayout(m_packagesLayout);
    }

    {
        m_kitCreationGroupBox = new QGroupBox(tr("Create a Kit"));
        m_kitCreationGroupBox->setFlat(true);
        mainLayout->addWidget(m_kitCreationGroupBox);
        m_kitCreationInfoLabel = new Utils::InfoLabel;
        auto vLayout = new QHBoxLayout(m_kitCreationGroupBox);
        vLayout->addWidget(m_kitCreationInfoLabel);
        m_kitCreationPushButton = new QPushButton(tr("Create Kit"));
        m_kitCreationPushButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        connect(m_kitCreationPushButton, &QPushButton::clicked, this, [this] {
            McuSupportOptions::newKit(currentMcuTarget(), m_options.qtForMCUsSdkPackage);
            McuSupportOptions::registerQchFiles();
            updateStatus();
        });
        m_kitRemovalPushButton = new QPushButton(tr("Remove Kit"));
        m_kitRemovalPushButton->setSizePolicy(m_kitCreationPushButton->sizePolicy());
        connect(m_kitRemovalPushButton, &QPushButton::clicked, this, [this] {
            for (auto existingKit : McuSupportOptions::existingKits(currentMcuTarget()))
                ProjectExplorer::KitManager::deregisterKit(existingKit);
            updateStatus();
        });
        vLayout->addWidget(m_kitCreationPushButton);
        vLayout->addWidget(m_kitRemovalPushButton);
    }

    mainLayout->addStretch();

    connect(&m_options, &McuSupportOptions::changed, this, &McuSupportOptionsWidget::updateStatus);

    showMcuTargetPackages();
}

void McuSupportOptionsWidget::updateStatus()
{
    const McuTarget *mcuTarget = currentMcuTarget();

    const bool cMakeAvailable = !CMakeProjectManager::CMakeToolManager::cmakeTools().isEmpty();

    // Page elements
    {
        m_qtForMCUsSdkGroupBox->setVisible(cMakeAvailable);
        const bool ready = cMakeAvailable && mcuTarget &&
                m_options.qtForMCUsSdkPackage->status() == McuPackage::ValidPackage;
        m_mcuTargetsGroupBox->setVisible(ready);
        m_packagesGroupBox->setVisible(ready && !mcuTarget->packages().isEmpty());
        m_kitCreationGroupBox->setVisible(ready);
    }

    // Kit creation status
    if (mcuTarget) {
        const bool mcuTargetValid = mcuTarget->isValid();
        m_kitCreationPushButton->setVisible(mcuTargetValid);
        m_kitRemovalPushButton->setVisible(mcuTargetValid);
        if (mcuTargetValid) {
            const bool mcuTargetKitExists = !McuSupportOptions::existingKits(mcuTarget).isEmpty();
            m_kitCreationInfoLabel->setType(mcuTargetKitExists
                                            ? Utils::InfoLabel::Information
                                            : Utils::InfoLabel::Ok);
            m_kitCreationInfoLabel->setText(mcuTargetKitExists
                                            ? tr("A kit for the selected target exists.")
                                            : tr("A kit for the selected target can be created."));
            m_kitCreationPushButton->setEnabled(!mcuTargetKitExists);
            m_kitRemovalPushButton->setEnabled(mcuTargetKitExists);
        } else {
            m_kitCreationInfoLabel->setType(Utils::InfoLabel::NotOk);
            m_kitCreationInfoLabel->setText("Provide the package paths in order to create a kit "
                                            "for your target.");
        }
    }

    // Status label in the bottom
    {
        m_statusInfoLabel->setVisible(!cMakeAvailable);
        if (m_statusInfoLabel->isVisible()) {
            m_statusInfoLabel->setType(Utils::InfoLabel::NotOk);
            m_statusInfoLabel->setText("No CMake tool was detected. Add a CMake tool in the "
                                       "<a href=\"cmake\">CMake options</a> and press Apply.");
        }
    }
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
    const int mcuTargetIndex = m_mcuTargetsComboBox->currentIndex();
    return (mcuTargetIndex == -1 || m_options.mcuTargets.isEmpty())
            ? nullptr
            : m_options.mcuTargets.at(mcuTargetIndex);
}

void McuSupportOptionsWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    populateMcuTargetsComboBox();
}

void McuSupportOptionsWidget::apply()
{
    m_options.qtForMCUsSdkPackage->writeToSettings();
    for (auto package : m_options.packages)
        package->writeToSettings();
}

void McuSupportOptionsWidget::populateMcuTargetsComboBox()
{
    m_options.populatePackagesAndTargets();
    m_mcuTargetsComboBox->clear();
    m_mcuTargetsComboBox->addItems(
                Utils::transform<QStringList>(m_options.mcuTargets, [](McuTarget *t) {
                    return McuSupportOptions::kitName(t);
                }));
    updateStatus();
}

McuSupportOptionsPage::McuSupportOptionsPage()
{
    setId(Utils::Id(Constants::SETTINGS_ID));
    setDisplayName(McuSupportOptionsWidget::tr("MCU"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new McuSupportOptionsWidget; });
}

} // Internal
} // McuSupport

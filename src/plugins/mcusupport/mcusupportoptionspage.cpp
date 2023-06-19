// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcusupportoptionspage.h"
#include "mcukitmanager.h"
#include "mcupackage.h"
#include "mcusupportconstants.h"
#include "mcusupportoptions.h"
#include "mcusupportsdk.h"
#include "mcusupporttr.h"
#include "mcutarget.h"
#include "mcutargetfactory.h"
#include "settingshandler.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>
#include <coreplugin/icore.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/algorithm.h>
#include <utils/infolabel.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace McuSupport::Internal {

class McuSupportOptionsWidget : public Core::IOptionsPageWidget
{
public:
    McuSupportOptionsWidget(McuSupportOptions &, const SettingsHandler::Ptr &);

    void updateStatus();
    void showMcuTargetPackages();
    [[nodiscard]] McuTargetPtr currentMcuTarget() const;

private:
    void apply() final;

    void populateMcuTargetsComboBox();
    void showEvent(QShowEvent *event) final;

    QString m_armGccPath;
    McuSupportOptions &m_options;
    SettingsHandler::Ptr m_settingsHandler;
    QMap<McuPackagePtr, QWidget *> m_packageWidgets;
    QMap<McuTargetPtr, QWidget *> m_mcuTargetPacketWidgets;
    QFormLayout *m_packagesLayout = nullptr;
    QGroupBox *m_qtForMCUsSdkGroupBox = nullptr;
    QGroupBox *m_packagesGroupBox = nullptr;
    QGroupBox *m_mcuTargetsGroupBox = nullptr;
    QComboBox *m_mcuTargetsComboBox = nullptr;
    QGroupBox *m_kitCreationGroupBox = nullptr;
    QCheckBox *m_kitAutomaticCreationCheckBox = nullptr;
    Utils::InfoLabel *m_kitCreationInfoLabel = nullptr;
    Utils::InfoLabel *m_statusInfoLabel = nullptr;
    Utils::InfoLabel *m_mcuTargetsInfoLabel = nullptr;
    QPushButton *m_kitCreationPushButton = nullptr;
    QPushButton *m_kitUpdatePushButton = nullptr;
};

McuSupportOptionsWidget::McuSupportOptionsWidget(McuSupportOptions &options,
                                                 const SettingsHandler::Ptr &settingsHandler)
    : m_options{options}
    , m_settingsHandler(settingsHandler)
{
    auto *mainLayout = new QVBoxLayout(this);

    {
        m_statusInfoLabel = new Utils::InfoLabel;
        m_statusInfoLabel->setElideMode(Qt::ElideNone);
        m_statusInfoLabel->setOpenExternalLinks(false);
        mainLayout->addWidget(m_statusInfoLabel);
        connect(m_statusInfoLabel, &QLabel::linkActivated, this, [] {
            Core::ICore::showOptionsDialog(CMakeProjectManager::Constants::Settings::TOOLS_ID);
        });
    }

    {
        m_qtForMCUsSdkGroupBox = new QGroupBox(Tr::tr("Qt for MCUs SDK"));
        m_qtForMCUsSdkGroupBox->setFlat(true);
        auto *layout = new QVBoxLayout(m_qtForMCUsSdkGroupBox);
        // Re-read the qtForMCUs package from settings to discard un-applied changes from previous sessions
        m_options.qtForMCUsSdkPackage->readFromSettings();

        layout->addWidget(m_options.qtForMCUsSdkPackage->widget());
        mainLayout->addWidget(m_qtForMCUsSdkGroupBox);
    }

    {
        m_mcuTargetsGroupBox = new QGroupBox(
            Tr::tr("Targets supported by the %1").arg(m_qtForMCUsSdkGroupBox->title()));
        m_mcuTargetsGroupBox->setFlat(true);
        mainLayout->addWidget(m_mcuTargetsGroupBox);
        m_mcuTargetsComboBox = new QComboBox;
        auto *layout = new QVBoxLayout(m_mcuTargetsGroupBox);
        layout->addWidget(m_mcuTargetsComboBox);
        connect(m_mcuTargetsComboBox,
                &QComboBox::currentTextChanged,
                this,
                &McuSupportOptionsWidget::showMcuTargetPackages);
        connect(m_options.qtForMCUsSdkPackage.get(),
                &McuAbstractPackage::changed,
                this,
                &McuSupportOptionsWidget::populateMcuTargetsComboBox);
    }

    {
        m_packagesGroupBox = new QGroupBox(Tr::tr("Requirements"));
        m_packagesGroupBox->setFlat(true);
        mainLayout->addWidget(m_packagesGroupBox);
        m_packagesLayout = new QFormLayout;
        m_packagesGroupBox->setLayout(m_packagesLayout);
    }

    {
        m_mcuTargetsInfoLabel = new Utils::InfoLabel;
        mainLayout->addWidget(m_mcuTargetsInfoLabel);
    }

    {
        m_kitAutomaticCreationCheckBox = new QCheckBox(
            Tr::tr("Automatically create kits for all available targets on start"));
        connect(m_kitAutomaticCreationCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
            m_options.setAutomaticKitCreationEnabled(state == Qt::CheckState::Checked);
        });
        mainLayout->addWidget(m_kitAutomaticCreationCheckBox);
    }

    {
        m_kitCreationGroupBox = new QGroupBox(Tr::tr("Create a Kit"));
        m_kitCreationGroupBox->setFlat(true);
        mainLayout->addWidget(m_kitCreationGroupBox);
        m_kitCreationInfoLabel = new Utils::InfoLabel;
        auto *vLayout = new QHBoxLayout(m_kitCreationGroupBox);
        vLayout->addWidget(m_kitCreationInfoLabel);
        m_kitCreationPushButton = new QPushButton(Tr::tr("Create Kit"));
        m_kitCreationPushButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        connect(m_kitCreationPushButton, &QPushButton::clicked, this, [this] {
            McuKitManager::newKit(currentMcuTarget().get(), m_options.qtForMCUsSdkPackage);
            m_options.registerQchFiles();
            updateStatus();
        });
        m_kitUpdatePushButton = new QPushButton(Tr::tr("Update Kit"));
        m_kitUpdatePushButton->setSizePolicy(m_kitCreationPushButton->sizePolicy());
        connect(m_kitUpdatePushButton, &QPushButton::clicked, this, [this] {
            for (auto *kit : McuKitManager::upgradeableKits(currentMcuTarget().get(),
                                                            m_options.qtForMCUsSdkPackage))
                McuKitManager::upgradeKitInPlace(kit,
                                                 currentMcuTarget().get(),
                                                 m_options.qtForMCUsSdkPackage);
            updateStatus();
        });
        vLayout->addWidget(m_kitCreationPushButton);
        vLayout->addWidget(m_kitUpdatePushButton);
    }

    mainLayout->addStretch();

    connect(&m_options,
            &McuSupportOptions::packagesChanged,
            this,
            &McuSupportOptionsWidget::updateStatus);

    showMcuTargetPackages();
}

void McuSupportOptionsWidget::updateStatus()
{
    const McuTargetPtr mcuTarget = currentMcuTarget();

    const bool cMakeAvailable = !CMakeProjectManager::CMakeToolManager::cmakeTools().isEmpty();

    // Page elements
    {
        m_qtForMCUsSdkGroupBox->setVisible(cMakeAvailable);
        const bool valid = cMakeAvailable && m_options.qtForMCUsSdkPackage->isValidStatus();
        const bool ready = valid && mcuTarget;
        m_mcuTargetsGroupBox->setVisible(ready);
        m_packagesGroupBox->setVisible(ready && !mcuTarget->packages().isEmpty());
        m_kitCreationGroupBox->setVisible(ready);
        m_mcuTargetsInfoLabel->setVisible(valid && m_options.sdkRepository.mcuTargets.isEmpty());
        if (m_mcuTargetsInfoLabel->isVisible()) {
            m_mcuTargetsInfoLabel->setType(Utils::InfoLabel::NotOk);
            const Utils::FilePath sdkPath = m_options.qtForMCUsSdkPackage->basePath();
            QString deprecationMessage;
            if (checkDeprecatedSdkError(sdkPath, deprecationMessage))
                m_mcuTargetsInfoLabel->setText(deprecationMessage);
            else
                m_mcuTargetsInfoLabel->setText(Tr::tr("No valid kit descriptions found at %1.")
                                                   .arg(kitsPath(sdkPath).toUserOutput()));
        }
    }

    // Kit creation status
    if (mcuTarget) {
        const bool mcuTargetValid = mcuTarget->isValid();
        m_kitCreationPushButton->setVisible(mcuTargetValid);
        m_kitUpdatePushButton->setVisible(mcuTargetValid);
        if (mcuTargetValid) {
            const bool hasMatchingKits = !McuKitManager::matchingKits(mcuTarget.get(),
                                                                      m_options.qtForMCUsSdkPackage)
                                              .isEmpty();
            const bool hasUpgradeableKits
                = !hasMatchingKits
                  && !McuKitManager::upgradeableKits(mcuTarget.get(), m_options.qtForMCUsSdkPackage)
                          .isEmpty();

            m_kitCreationPushButton->setEnabled(!hasMatchingKits);
            m_kitUpdatePushButton->setEnabled(hasUpgradeableKits);

            m_kitCreationInfoLabel->setType(!hasMatchingKits ? Utils::InfoLabel::Information
                                                             : Utils::InfoLabel::Ok);

            m_kitCreationInfoLabel->setText(
                hasMatchingKits
                    ? Tr::tr("A kit for the selected target and SDK version already exists.")
                : hasUpgradeableKits ? Tr::tr("Kits for a different SDK version exist.")
                                     : Tr::tr("A kit for the selected target can be created."));
        } else {
            m_kitCreationInfoLabel->setType(Utils::InfoLabel::NotOk);
            m_kitCreationInfoLabel->setText(Tr::tr("Provide the package paths to create a kit "
                                            "for your target."));
        }
    }

    // Automatic Kit creation
    m_kitAutomaticCreationCheckBox->setChecked(m_options.automaticKitCreationEnabled());

    // Status label in the bottom
    {
        m_statusInfoLabel->setVisible(!cMakeAvailable);
        if (m_statusInfoLabel->isVisible()) {
            m_statusInfoLabel->setType(Utils::InfoLabel::NotOk);
            m_statusInfoLabel->setText(Tr::tr("No CMake tool was detected. Add a CMake tool in the "
                                       "<a href=\"cmake\">CMake options</a> and select Apply."));
        }
    }
}

struct McuPackageSort {
    bool operator()(McuPackagePtr a, McuPackagePtr b) const {
        if (a->cmakeVariableName() != b->cmakeVariableName())
            return a->cmakeVariableName() > b->cmakeVariableName();
        else
            return a->environmentVariableName() > b->environmentVariableName();
    }
};

void McuSupportOptionsWidget::showMcuTargetPackages()
{
    McuTargetPtr mcuTarget = currentMcuTarget();
    if (!mcuTarget)
        return;

    while (m_packagesLayout->rowCount() > 0) {
        m_packagesLayout->removeRow(0);
    }

    std::set<McuPackagePtr, McuPackageSort> packages;

    for (const auto &package : mcuTarget->packages()) {
        if (package->label().isEmpty())
            continue;
        packages.insert(package);
    }

    for (const auto &package : packages) {
        QWidget *packageWidget = package->widget();
        QWeakPointer packagePtr(package);
        connect(package.get(), &McuPackage::reset, this, [this, packagePtr] (){
            McuPackagePtr package = packagePtr.lock();
            if (package) {
                MacroExpanderPtr macroExpander
                    = m_options.sdkRepository.getMacroExpander(*currentMcuTarget());
                package->setPath(macroExpander->expand(package->defaultPath()));
            }
        });
        m_packagesLayout->addRow(package->label(), packageWidget);
        packageWidget->show();
    }

    updateStatus();
}

McuTargetPtr McuSupportOptionsWidget::currentMcuTarget() const
{
    const int mcuTargetIndex = m_mcuTargetsComboBox->currentIndex();
    McuTargetPtr target{nullptr};
    if (mcuTargetIndex != -1 && !m_options.sdkRepository.mcuTargets.isEmpty())
        target = m_options.sdkRepository.mcuTargets.at(mcuTargetIndex);

    return target;
}

void McuSupportOptionsWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    populateMcuTargetsComboBox();
}

void McuSupportOptionsWidget::apply()
{
    bool pathsChanged = false;

    m_settingsHandler->setAutomaticKitCreation(m_options.automaticKitCreationEnabled());
    m_options.sdkRepository.expandVariablesAndWildcards();

    if (m_mcuTargetsComboBox->count() == 0)
        return;

    QMessageBox warningPopup(QMessageBox::Icon::Warning,
                             Tr::tr("Warning"),
                             Tr::tr("Unable to apply changes in Devices > MCU."),
                             QMessageBox::Ok,
                             this);

    auto target = currentMcuTarget();
    if (!target) {
        warningPopup.setInformativeText(Tr::tr("No target selected."));
        warningPopup.exec();
        return;
    }
    if (!target->isValid()) {
        warningPopup.setInformativeText(
            Tr::tr("Invalid paths present for target\n%1")
                .arg(McuKitManager::generateKitNameFromTarget(target.get())));
        warningPopup.exec();
        return;
    }

    pathsChanged |= m_options.qtForMCUsSdkPackage->writeToSettings();
    for (const auto &package : target->packages())
        pathsChanged |= package->writeToSettings();

    if (pathsChanged) {
        m_options.checkUpgradeableKits();
        McuKitManager::updatePathsInExistingKits(m_settingsHandler);
    }
}

void McuSupportOptionsWidget::populateMcuTargetsComboBox()
{
    m_options.populatePackagesAndTargets();
    m_mcuTargetsComboBox->clear();
    int initialPlatformIndex = 0;
    int targetsCounter = -1;
    m_mcuTargetsComboBox->addItems(
        Utils::transform<QStringList>(m_options.sdkRepository.mcuTargets, [&](const McuTargetPtr &t) {
            if (t->platform().name == m_settingsHandler->initialPlatformName())
                initialPlatformIndex = m_options.sdkRepository.mcuTargets.indexOf(t);
            targetsCounter++;
            return McuKitManager::generateKitNameFromTarget(t.get());
        }));
    if (targetsCounter != -1)
        m_mcuTargetsComboBox->setCurrentIndex(initialPlatformIndex);
    updateStatus();
}

McuSupportOptionsPage::McuSupportOptionsPage(McuSupportOptions &options,
                                             const SettingsHandler::Ptr &settingsHandler)
{
    setId(Utils::Id(Constants::SETTINGS_ID));
    setDisplayName(Tr::tr("MCU"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setWidgetCreator([&options, &settingsHandler] {
        return new McuSupportOptionsWidget(options, settingsHandler);
    });
}

} // namespace McuSupport::Internal

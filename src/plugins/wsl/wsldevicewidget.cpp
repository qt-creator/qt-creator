// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "wsldevicewidget.h"

#include "wslapi.h"
#include "wsldevice.h"
#include "wsltr.h"

#include <projectexplorer/kitaspect.h>

#include <utils/async.h>
#include <utils/futuresynchronizer.h>
#include <utils/guiutils.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>

using namespace ProjectExplorer;
using namespace Utils;

namespace Wsl::Internal {

struct DistributionState
{
    InfoLabelType type = InfoLabelType::Information;
    QString text;
};

// Two round trips to the WSL service, which is why this is asked off the main
// thread: a wedged service would hold the settings page up for as long as it
// takes them to time out.
static DistributionState distributionState(const QString &distribution)
{
    const Result<QStringList> installed = installedDistributions();
    if (!installed)
        return {InfoLabelType::Error, installed.error()};

    if (!installed->contains(distribution)) {
        return {
            InfoLabelType::Warning,
            Tr::tr("The distribution \"%1\" is not installed.").arg(distribution)};
    }

    const Result<QStringList> running = runningDistributions();
    const bool isRunning = running && running->contains(distribution);
    // A stopped distribution is not a problem: WSL starts it with the first
    // command sent to it.
    return {
        InfoLabelType::Ok,
        isRunning ? Tr::tr("The distribution is running.")
                  : Tr::tr("The distribution is stopped. It starts with the first command.")};
}

WslDeviceWidget::WslDeviceWidget(const IDevice::Ptr &device)
    : IDeviceWidget(device)
{
    const auto wslDevice = std::dynamic_pointer_cast<WslDevice>(device);
    QTC_ASSERT(wslDevice, return);

    auto stateLabel = new InfoLabel;
    stateLabel->setElideMode(Qt::ElideNone);
    stateLabel->setType(InfoLabelType::Information);
    stateLabel->setText(Tr::tr("Asking WSL about the distribution..."));

    QFuture<DistributionState> stateFuture
        = Utils::asyncRun(&distributionState, wslDevice->distribution());
    stateFuture.then(this, [stateLabel](const DistributionState &state) {
        stateLabel->setType(state.type);
        stateLabel->setText(state.text);
    });
    Utils::futureSynchronizer()->addFuture(stateFuture);

    auto commandLineLabel = new QLabel(wslDevice->createCommandLineForDisplay().toUserOutput());
    commandLineLabel->setWordWrap(true);
    commandLineLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto logView = new QTextBrowser;

    auto autoDetectButton = new QPushButton(Tr::tr("Auto-detect Kit Items"));
    auto undoAutoDetectButton = new QPushButton(Tr::tr("Remove Auto-Detected Kit Items"));
    auto listAutoDetectedButton = new QPushButton(Tr::tr("List Auto-Detected Kit Items"));
    const QList<QWidget *> tempDisabledWidgets
        = {autoDetectButton, undoAutoDetectButton, listAutoDetectedButton};

    connect(
        autoDetectButton,
        &QPushButton::clicked,
        this,
        [this, logView, wslDevice, tempDisabledWidgets] {
            logView->clear();

            const auto log = [logView](const QString &msg) { logView->append(msg); };
            // clang-format off
            const QtTaskTree::Group recipe {
                wslDevice->autoDetectDeviceToolsRecipe(),
                ProjectExplorer::removeDetectedKitsRecipe(wslDevice, log),
                ProjectExplorer::kitDetectionRecipe(wslDevice, DetectionSource::FromSystem, log)
            };
            // clang-format on

            const auto onSetup = [logView, tempDisabledWidgets] {
                for (QWidget *widget : tempDisabledWidgets)
                    widget->setEnabled(false);
                logView->append(Tr::tr("Starting auto-detection..."));
            };

            const auto onDone = [logView, tempDisabledWidgets] {
                for (QWidget *widget : tempDisabledWidgets)
                    widget->setEnabled(true);
                logView->append(Tr::tr("Done."));
            };

            m_detectionRunner.start(recipe, onSetup, onDone);
        });

    connect(undoAutoDetectButton, &QPushButton::clicked, this, [this, logView, device] {
        logView->clear();
        m_detectionRunner.start(
            ProjectExplorer::removeDetectedKitsRecipe(device, [logView](const QString &msg) {
                logView->append(msg);
            }));
    });

    connect(listAutoDetectedButton, &QPushButton::clicked, this, [logView, device] {
        logView->clear();
        listAutoDetected(device, [logView](const QString &msg) { logView->append(msg); });
    });

    using namespace Layouting;

    // clang-format off
    Column {
        noMargin,
        Form {
            noMargin,
            wslDevice->distribution, br,
            Tr::tr("State:"), stateLabel, br,
            wslDevice->userName, br,
            wslDevice->mountRoot, br,
            wslDevice->appendWindowsPath, br,
            wslDevice->freePortsAspect, br,
            Tr::tr("Command line:"), commandLineLabel, br,
            wslDevice->deviceToolsGui(), br,
            Span(2, Row {
                autoDetectButton,
                undoAutoDetectButton,
                listAutoDetectedButton,
                st,
            }), br,
            Tr::tr("Detection log:"), logView
        }, br,
    }.attachTo(this);
    // clang-format on

    connect(wslDevice.get(), &BaseAspect::volatileValueChanged, this, [commandLineLabel, wslDevice] {
        commandLineLabel->setText(wslDevice->createCommandLineForDisplay().toUserOutput());
    });

    installMarkSettingsDirtyTriggerRecursively(this);
}

} // namespace Wsl::Internal

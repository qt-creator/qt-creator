// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockerdevicewidget.h"

#include "dockerapi.h"
#include "dockerdevice.h"
#include "dockertr.h"

#include <cppeditor/cppeditorconstants.h>

#include <projectexplorer/kitaspect.h>

#include <utils/algorithm.h>
#include <utils/clangutils.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QToolButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace Docker::Internal {

DockerDeviceWidget::DockerDeviceWidget(const IDevice::Ptr &device)
    : IDeviceWidget(device)
{
    auto dockerDevice = std::dynamic_pointer_cast<DockerDevice>(device);
    QTC_ASSERT(dockerDevice, return);

    using namespace Layouting;

    auto daemonStateLabel = new QLabel(Tr::tr("Daemon state:"));
    m_daemonReset = new QToolButton;
    m_daemonReset->setToolTip(Tr::tr("Clears detected daemon state. "
        "It will be automatically re-evaluated next time access is needed."));

    m_daemonState = new QLabel;

    connect(DockerApi::instance(), &DockerApi::dockerDaemonAvailableChanged, this, [this]{
        updateDaemonStateTexts();
    });

    updateDaemonStateTexts();

    connect(m_daemonReset, &QToolButton::clicked, this, [] {
        DockerApi::recheckDockerDaemon();
    });

    auto pathListLabel = new InfoLabel(Tr::tr("Paths to mount:"));
    pathListLabel->setElideMode(Qt::ElideNone);
    pathListLabel->setAdditionalToolTip(Tr::tr("Source directory list should not be empty."));

    auto markupMounts = [dockerDevice, pathListLabel] {
        const bool isEmpty = dockerDevice->mounts.volatileValue().isEmpty();
        pathListLabel->setType(isEmpty ? InfoLabel::Warning : InfoLabel::None);
    };
    markupMounts();

    connect(&dockerDevice->mounts, &FilePathListAspect::volatileValueChanged, this, markupMounts);

    auto logView = new QTextBrowser;

    auto autoDetectButton = new QPushButton(Tr::tr("Auto-detect Kit Items"));
    auto undoAutoDetectButton = new QPushButton(Tr::tr("Remove Auto-Detected Kit Items"));
    auto listAutoDetectedButton = new QPushButton(Tr::tr("List Auto-Detected Kit Items"));

    connect(&m_detectionRunner, &QSingleTaskTreeRunner::aboutToStart, [=] {
        autoDetectButton->setEnabled(false);
        undoAutoDetectButton->setEnabled(false);
        listAutoDetectedButton->setEnabled(false);
        logView->append(Tr::tr("Starting auto-detection..."));
    });
    connect(&m_detectionRunner, &QSingleTaskTreeRunner::done, [=] {
        autoDetectButton->setEnabled(true);
        undoAutoDetectButton->setEnabled(true);
        listAutoDetectedButton->setEnabled(true);
        logView->append(Tr::tr("Done."));
    });

    connect(autoDetectButton,
            &QPushButton::clicked,
            this,
            [this, logView, dockerDevice] {
                logView->clear();
                Result<> startResult = dockerDevice->updateContainerAccess();

                if (!startResult) {
                    logView->append(Tr::tr("Failed to start container."));
                    logView->append(startResult.error());
                    return;
                }

                const auto log = [logView](const QString &msg) { logView->append(msg); };
                // clang-format off
                const QtTaskTree::Group recipe {
                    dockerDevice->autoDetectDeviceToolsRecipe().recipe,
                    ProjectExplorer::removeDetectedKitsRecipe(dockerDevice, log),
                    ProjectExplorer::kitDetectionRecipe(dockerDevice, DetectionSource::FromSystem, log)
                };
                // clang-format on

                m_detectionRunner.start(recipe);

                if (DockerApi::instance()->dockerDaemonAvailable().value_or(false) == false)
                    logView->append(Tr::tr("Docker daemon appears to be stopped."));
                else
                    logView->append(Tr::tr("Docker daemon appears to be running."));
                updateDaemonStateTexts();
            });

    connect(undoAutoDetectButton, &QPushButton::clicked, this, [this, logView, device] {
        logView->clear();
        m_detectionRunner.start(
            ProjectExplorer::removeDetectedKitsRecipe(device, [logView](const QString &msg) {
                logView->append(msg);
            })
        );
    });

    connect(listAutoDetectedButton, &QPushButton::clicked, this, [logView, device] {
        logView->clear();
        listAutoDetected(device, [logView](const QString &msg) { logView->append(msg); });
    });

    auto createLineLabel = new QLabel(dockerDevice->createCommandLine().toUserOutput());
    createLineLabel->setWordWrap(true);

    using namespace Layouting;

    // clang-format off
    Column {
        noMargin,
        Form {
            noMargin,
            dockerDevice->repo, br,
            dockerDevice->tag, br,
            dockerDevice->imageId, br,
            daemonStateLabel, m_daemonReset, m_daemonState, br,
            dockerDevice->useLocalUidGid, br,
            dockerDevice->keepEntryPoint, br,
            dockerDevice->enableLldbFlags, br,
            dockerDevice->mountCmdBridge, br,
            dockerDevice->network, br,
            dockerDevice->extraArgs, br,
            dockerDevice->environment, br,
            pathListLabel, dockerDevice->mounts, br,
            Tr::tr("Port mappings:"), dockerDevice->portMappings, br,
            Tr::tr("Command line:"), createLineLabel, br,
            dockerDevice->deviceToolsGui(), br,
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

    connect(&*dockerDevice, &AspectContainer::applied, this, [createLineLabel, dockerDevice] {
        createLineLabel->setText(dockerDevice->createCommandLine().toUserOutput());
    });
}

void DockerDeviceWidget::updateDaemonStateTexts()
{
    std::optional<bool> daemonState = DockerApi::instance()->dockerDaemonAvailable();
    if (!daemonState.has_value()) {
        m_daemonReset->setIcon(Icons::INFO.icon());
        m_daemonState->setText(Tr::tr("Daemon state not evaluated."));
    } else if (daemonState.value()) {
        m_daemonReset->setIcon(Icons::OK.icon());
        m_daemonState->setText(Tr::tr("Docker daemon running."));
    } else {
        m_daemonReset->setIcon(Icons::CRITICAL.icon());
        m_daemonState->setText(Tr::tr("Docker daemon not running."));
    }
}

} // namespace Docker::Internal

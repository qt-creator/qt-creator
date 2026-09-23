// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockerdevicewidget.h"

#include "dockerapi.h"
#include "dockerdevice.h"
#include "dockertr.h"

#include <utils/commandline.h>
#include <utils/guiutils.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QToolButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace Docker::Internal {

DockerDeviceWidget::DockerDeviceWidget(const IDevice::Ptr &device)
    : IDeviceWidget(device)
{
    auto dockerDevice = std::dynamic_pointer_cast<DockerDevice>(device);
    QTC_ASSERT(dockerDevice, return);

    m_api = DockerApi::instance(dockerDevice->type());
    QTC_ASSERT(m_api, return);

    using namespace Layouting;

    auto daemonStateLabel = new QLabel(Tr::tr("Daemon state:"));
    m_daemonReset = new QToolButton;
    m_daemonReset->setToolTip(Tr::tr("Clears detected daemon state. "
        "It will be automatically re-evaluated next time access is needed."));

    m_daemonState = new QLabel;

    connect(m_api, &DockerApi::dockerDaemonAvailableChanged, this, [this]{
        updateDaemonStateTexts();
    });

    updateDaemonStateTexts();

    connect(m_daemonReset, &QToolButton::clicked, this, [dockerDevice] {
        DockerApi::recheckDaemon(dockerDevice->type());
    });

    onFirstShow(this, [this, dockerDevice] {
        const FilePath dockerExe = m_api->dockerClient();
        if (dockerExe.isEmpty())
            return;

        const auto onSetup = [dockerExe, dockerDevice](Process &process) {
            process.setCommand({dockerExe, {"images", "-q", dockerDevice->repoAndTag()}});
        };
        const auto onDone = [dockerDevice](const Process &process) {
            const QString imageId = process.cleanedStdOut().trimmed();
            if (process.exitCode() == 0 && !imageId.isEmpty())
                dockerDevice->imageId.setValue(imageId);
        };
        m_imageIdRunner.start({ProcessTask(onSetup, onDone)});
    });

    auto pathListLabel = new InfoLabel(Tr::tr("Paths to mount:"));
    pathListLabel->setElideMode(Qt::ElideNone);

    auto markupMounts = [dockerDevice, pathListLabel] {
        const QStringList entries = dockerDevice->mounts.volatileValue();
        QStringList warnings;
        if (entries.isEmpty()) {
            warnings.append(Tr::tr("Source directory list should not be empty."));
        } else {
            const QList<MountPair> mounts
                = parseMounts(entries, dockerDevice->mounts.macroExpander());
            for (int i = 0; i < entries.size(); ++i) {
                const Result<> res = validateMount(mounts.at(i));
                if (!res) {
                    warnings.append(
                        Tr::tr("\"%1\" is not mounted: %2").arg(entries.at(i), res.error()));
                }
            }
        }
        pathListLabel->setType(warnings.isEmpty() ? InfoLabelType::None : InfoLabelType::Warning);
        pathListLabel->setAdditionalToolTip(warnings.join('\n'));
    };
    markupMounts();

    connect(&dockerDevice->mounts, &FilePathListAspect::volatileValueChanged, this, markupMounts);

    auto createLineLabel = new QLabel(dockerDevice->createCommandLineForDisplay().toUserOutput());
    createLineLabel->setWordWrap(true);
    createLineLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto refreshNetworksButton = new QToolButton();
    setIgnoreForDirtyHook(refreshNetworksButton);
    refreshNetworksButton->setIcon(Icons::RELOAD_TOOLBAR.icon());
    refreshNetworksButton->setToolTip(Tr::tr("Refresh %1 networks").arg(m_api->displayType()));
    connect(refreshNetworksButton, &QPushButton::clicked, this, [this] {
        m_api->refreshNetworks();
    });

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
            dockerDevice->enableX11Forwarding, br,
            dockerDevice->x11Display, br,
            dockerDevice->network, refreshNetworksButton,br,
            dockerDevice->extraArgs, br,
            dockerDevice->environment, br,
            pathListLabel, dockerDevice->mounts, br,
            Tr::tr("Port mappings:"), dockerDevice->portMappings, br,
            Tr::tr("Command line:"), createLineLabel, br,
            dockerDevice->deviceToolsGui(),
            dockerDevice->autoDetectGui(),
        }, br,
    }.attachTo(this);
    // clang-format on

    connect(dockerDevice.get(), &BaseAspect::volatileValueChanged, this, [createLineLabel, dockerDevice] {
        createLineLabel->setText(dockerDevice->createCommandLineForDisplay().toUserOutput());
    });

    connect(&dockerDevice->mounts, &FilePathListAspect::volatileValueChanged,
            this, checkSettingsDirty);

    installMarkSettingsDirtyTriggerRecursively(this);
}

void DockerDeviceWidget::updateDaemonStateTexts()
{
    std::optional<bool> daemonState = m_api->dockerDaemonAvailable();
    if (!daemonState.has_value()) {
        m_daemonReset->setIcon(Icons::INFO.icon());
        m_daemonState->setText(Tr::tr("Daemon state not evaluated."));
    } else if (*daemonState) {
        m_daemonReset->setIcon(Icons::OK.icon());
        m_daemonState->setText(Tr::tr("%1 daemon running.").arg(m_api->displayType()));
    } else {
        m_daemonReset->setIcon(Icons::CRITICAL.icon());
        m_daemonState->setText(Tr::tr("%1 daemon not running.").arg(m_api->displayType()));
    }
}

} // namespace Docker::Internal

// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockerdevicewidget.h"

#include "dockerapi.h"
#include "dockerdevice.h"
#include "dockertr.h"

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
    : IDeviceWidget(device), m_kitItemDetector(device)
{
    auto dockerDevice = device.dynamicCast<DockerDevice>();
    QTC_ASSERT(dockerDevice, return);

    DockerDeviceSettings *deviceSettings = static_cast<DockerDeviceSettings *>(device->settings());

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
    pathListLabel->setAdditionalToolTip(Tr::tr("Source directory list should not be empty."));

    auto markupMounts = [deviceSettings, pathListLabel] {
        const bool isEmpty = deviceSettings->mounts.volatileValue().isEmpty();
        pathListLabel->setType(isEmpty ? InfoLabel::Warning : InfoLabel::None);
    };
    markupMounts();

    connect(&deviceSettings->mounts, &FilePathListAspect::volatileValueChanged, this, markupMounts);

    auto logView = new QTextBrowser;
    connect(&m_kitItemDetector, &KitDetector::logOutput,
            logView, &QTextBrowser::append);

    auto autoDetectButton = new QPushButton(Tr::tr("Auto-detect Kit Items"));
    auto undoAutoDetectButton = new QPushButton(Tr::tr("Remove Auto-Detected Kit Items"));
    auto listAutoDetectedButton = new QPushButton(Tr::tr("List Auto-Detected Kit Items"));

    auto searchDirsComboBox = new QComboBox;
    searchDirsComboBox->addItem(Tr::tr("Search in PATH"));
    searchDirsComboBox->addItem(Tr::tr("Search in Selected Directories"));
    searchDirsComboBox->addItem(Tr::tr("Search in PATH and Additional Directories"));

    auto searchDirsLineEdit = new FancyLineEdit;

    searchDirsLineEdit->setPlaceholderText(Tr::tr("Semicolon-separated list of directories"));
    searchDirsLineEdit->setToolTip(
        Tr::tr("Select the paths in the Docker image that should be scanned for kit entries."));
    searchDirsLineEdit->setHistoryCompleter("DockerMounts", true);

    auto searchPaths = [searchDirsComboBox, searchDirsLineEdit, dockerDevice] {
        FilePaths paths;
        const int idx = searchDirsComboBox->currentIndex();
        if (idx == 0 || idx == 2)
            paths += dockerDevice->systemEnvironment().path();
        if (idx == 1 || idx == 2) {
            for (const QString &path : searchDirsLineEdit->text().split(';'))
                paths.append(FilePath::fromString(path.trimmed()));
        }
        paths = Utils::transform(paths, [dockerDevice](const FilePath &path) {
            return dockerDevice->filePath(path.path());
        });
        return paths;
    };

    connect(autoDetectButton,
            &QPushButton::clicked,
            this,
            [this, logView, dockerDevice, searchPaths, deviceSettings] {
                logView->clear();
                expected_str<void> startResult = dockerDevice->updateContainerAccess();

                if (!startResult) {
                    logView->append(Tr::tr("Failed to start container."));
                    logView->append(startResult.error());
                    return;
                }

                const FilePath clangdPath
                    = dockerDevice->filePath("clangd")
                          .searchInPath({}, FilePath::AppendToPath, [](const FilePath &clangd) {
                              return Utils::checkClangdVersion(clangd);
                          });

                if (!clangdPath.isEmpty())
                    deviceSettings->clangdExecutable.setValue(clangdPath);

                m_kitItemDetector.autoDetect(dockerDevice->id().toString(), searchPaths());

                if (DockerApi::instance()->dockerDaemonAvailable().value_or(false) == false)
                    logView->append(Tr::tr("Docker daemon appears to be stopped."));
                else
                    logView->append(Tr::tr("Docker daemon appears to be running."));

                logView->append(Tr::tr("Detection complete."));
                updateDaemonStateTexts();
            });

    connect(undoAutoDetectButton, &QPushButton::clicked, this, [this, logView, device] {
        logView->clear();
        m_kitItemDetector.undoAutoDetect(device->id().toString());
    });

    connect(listAutoDetectedButton, &QPushButton::clicked, this, [this, logView, device] {
        logView->clear();
        m_kitItemDetector.listAutoDetected(device->id().toString());
    });

    auto createLineLabel = new QLabel(dockerDevice->createCommandLine().toUserOutput());
    createLineLabel->setWordWrap(true);

    using namespace Layouting;

    // clang-format off
    Column detectionControls {
        Space(20),
        Row {
            Tr::tr("Search Locations:"),
            searchDirsComboBox,
            searchDirsLineEdit
        },
        Row {
            autoDetectButton,
            undoAutoDetectButton,
            listAutoDetectedButton,
            st,
        },
        Tr::tr("Detection log:"),
        logView
    };
    Column {
        noMargin,
        Form {
            deviceSettings->repo, br,
            deviceSettings->tag, br,
            deviceSettings->imageId, br,
            daemonStateLabel, m_daemonReset, m_daemonState, br,
            Tr::tr("Container state:"), deviceSettings->containerStatus, br,
            deviceSettings->useLocalUidGid, br,
            deviceSettings->keepEntryPoint, br,
            deviceSettings->enableLldbFlags, br,
            deviceSettings->clangdExecutable, br,
            deviceSettings->network, br,
            deviceSettings->extraArgs, br,
            Column {
                pathListLabel,
                deviceSettings->mounts,
            }, br,
            If { dockerDevice->isAutoDetected(), {}, {detectionControls} },
            noMargin,
        },br,
        Tr::tr("Command line:"), createLineLabel, br,
    }.attachTo(this);
    // clang-format on

    searchDirsLineEdit->setVisible(false);
    auto updateDirectoriesLineEdit = [searchDirsLineEdit](int index) {
        searchDirsLineEdit->setVisible(index == 1 || index == 2);
        if (index == 1 || index == 2)
            searchDirsLineEdit->setFocus();
    };
    QObject::connect(searchDirsComboBox, &QComboBox::activated, this, updateDirectoriesLineEdit);

    connect(deviceSettings, &AspectContainer::applied, this, [createLineLabel, dockerDevice] {
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

} // Docker::Internal

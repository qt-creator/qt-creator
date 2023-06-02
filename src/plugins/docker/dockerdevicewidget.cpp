// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockerdevicewidget.h"

#include "dockerapi.h"
#include "dockerdevice.h"
#include "dockertr.h"

#include <utils/algorithm.h>
#include <utils/clangutils.h>
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

    m_data = dockerDevice->data();

    auto repoLabel = new QLabel(Tr::tr("Repository:"));
    m_repoLineEdit = new QLineEdit;
    m_repoLineEdit->setText(m_data.repo);
    m_repoLineEdit->setEnabled(false);

    auto tagLabel = new QLabel(Tr::tr("Tag:"));
    m_tagLineEdit = new QLineEdit;
    m_tagLineEdit->setText(m_data.tag);
    m_tagLineEdit->setEnabled(false);

    auto idLabel = new QLabel(Tr::tr("Image ID:"));
    m_idLineEdit = new QLineEdit;
    m_idLineEdit->setText(m_data.imageId);
    m_idLineEdit->setEnabled(false);

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

    m_keepEntryPoint = new QCheckBox(Tr::tr("Do not modify entry point"));
    m_keepEntryPoint->setToolTip(
        Tr::tr("Prevents modifying the entry point of the image. Enable only if "
               "the image starts into a shell."));
    m_keepEntryPoint->setChecked(m_data.keepEntryPoint);
    m_keepEntryPoint->setEnabled(true);

    connect(m_keepEntryPoint, &QCheckBox::toggled, this, [this, dockerDevice](bool on) {
        m_data.keepEntryPoint = on;
        dockerDevice->setData(m_data);
    });

    m_enableLldbFlags = new QCheckBox(Tr::tr("Enable flags needed for LLDB"));
    m_enableLldbFlags->setToolTip(Tr::tr("Adds the following flags to the container: "
                                         "--cap-add=SYS_PTRACE --security-opt seccomp=unconfined, "
                                         "this is necessary to allow lldb to run"));
    m_enableLldbFlags->setChecked(m_data.enableLldbFlags);
    m_enableLldbFlags->setEnabled(true);

    connect(m_enableLldbFlags, &QCheckBox::toggled, this, [this, dockerDevice](bool on) {
        m_data.enableLldbFlags = on;
        dockerDevice->setData(m_data);
    });

    m_runAsOutsideUser = new QCheckBox(Tr::tr("Run as outside user"));
    m_runAsOutsideUser->setToolTip(Tr::tr("Uses user ID and group ID of the user running Qt Creator "
                                          "in the docker container."));
    m_runAsOutsideUser->setChecked(m_data.useLocalUidGid);
    m_runAsOutsideUser->setEnabled(HostOsInfo::isAnyUnixHost());

    connect(m_runAsOutsideUser, &QCheckBox::toggled, this, [this, dockerDevice](bool on) {
        m_data.useLocalUidGid = on;
        dockerDevice->setData(m_data);
    });

    auto clangDLabel = new QLabel(Tr::tr("Clangd Executable:"));

    m_clangdExecutable = new PathChooser();
    m_clangdExecutable->setExpectedKind(PathChooser::ExistingCommand);
    m_clangdExecutable->setHistoryCompleter("Docker.ClangdExecutable.History");
    m_clangdExecutable->setAllowPathFromDevice(true);
    m_clangdExecutable->setFilePath(m_data.clangdExecutable);
    m_clangdExecutable->setValidationFunction(
        [chooser = m_clangdExecutable](FancyLineEdit *, QString *error) {
            return Utils::checkClangdVersion(chooser->filePath(), error);
        });

    connect(m_clangdExecutable, &PathChooser::rawPathChanged, this, [this, dockerDevice] {
        m_data.clangdExecutable = m_clangdExecutable->filePath();
        dockerDevice->setData(m_data);
    });

    auto pathListLabel = new InfoLabel(Tr::tr("Paths to mount:"));
    pathListLabel->setAdditionalToolTip(Tr::tr("Source directory list should not be empty."));

    m_pathsListEdit = new PathListEditor;
    m_pathsListEdit->setPlaceholderText(Tr::tr("Host directories to mount into the container"));
    m_pathsListEdit->setToolTip(Tr::tr("Maps paths in this list one-to-one to the "
                                       "docker container."));
    m_pathsListEdit->setPathList(m_data.mounts);
    m_pathsListEdit->setMaximumHeight(100);
    m_pathsListEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto markupMounts = [this, pathListLabel] {
        const bool isEmpty = m_pathsListEdit->pathList().isEmpty();
        pathListLabel->setType(isEmpty ? InfoLabel::Warning : InfoLabel::None);
    };
    markupMounts();

    connect(m_pathsListEdit, &PathListEditor::changed, this, [this, dockerDevice, markupMounts] {
        m_data.mounts = m_pathsListEdit->pathList();
        dockerDevice->setData(m_data);
        markupMounts();
    });

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
        Tr::tr("Select the paths in the docker image that should be scanned for kit entries."));
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

    connect(autoDetectButton, &QPushButton::clicked, this,
            [this, logView, dockerDevice, searchPaths] {
        logView->clear();
        dockerDevice->updateContainerAccess();

        const FilePath clangdPath = dockerDevice->filePath("clangd")
                                        .searchInPath({},
                                                      FilePath::AppendToPath,
                                                      [](const FilePath &clangd) {
                                                          return Utils::checkClangdVersion(clangd);
                                                      });

        if (!clangdPath.isEmpty())
            m_clangdExecutable->setFilePath(clangdPath);

        m_kitItemDetector.autoDetect(dockerDevice->id().toString(), searchPaths());

        if (DockerApi::instance()->dockerDaemonAvailable().value_or(false) == false)
            logView->append(Tr::tr("Docker daemon appears to be not running."));
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

    Form {
        repoLabel, m_repoLineEdit, br,
        tagLabel, m_tagLineEdit, br,
        idLabel, m_idLineEdit, br,
        daemonStateLabel, m_daemonReset, m_daemonState, br,
        m_runAsOutsideUser, br,
        m_keepEntryPoint, br,
        m_enableLldbFlags, br,
        clangDLabel, m_clangdExecutable, br,
        Column {
            pathListLabel,
            m_pathsListEdit,
        }, br,
        (dockerDevice->isAutoDetected() ? Column {} : std::move(detectionControls)),
        noMargin,
    }.attachTo(this);
    // clang-format on

    searchDirsLineEdit->setVisible(false);
    auto updateDirectoriesLineEdit = [searchDirsLineEdit](int index) {
        searchDirsLineEdit->setVisible(index == 1 || index == 2);
        if (index == 1 || index == 2)
            searchDirsLineEdit->setFocus();
    };
    QObject::connect(searchDirsComboBox, &QComboBox::activated, this, updateDirectoriesLineEdit);
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

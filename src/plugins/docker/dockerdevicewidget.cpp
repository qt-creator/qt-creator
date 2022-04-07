/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "dockerdevicewidget.h"

#include <utils/utilsicons.h>
#include <utils/hostosinfo.h>
#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>

#include <QCoreApplication>
#include <QToolButton>
#include <QTextBrowser>
#include <QPushButton>
#include <QComboBox>

using namespace ProjectExplorer;
using namespace Utils;

namespace Docker {
namespace Internal {

DockerDeviceWidget::DockerDeviceWidget(const IDevice::Ptr &device)
    : IDeviceWidget(device), m_kitItemDetector(device)
{
    auto dockerDevice = device.dynamicCast<DockerDevice>();
    QTC_ASSERT(dockerDevice, return);

    DockerDeviceData &data = dockerDevice->data();

    auto repoLabel = new QLabel(tr("Repository:"));
    m_repoLineEdit = new QLineEdit;
    m_repoLineEdit->setText(data.repo);
    m_repoLineEdit->setEnabled(false);

    auto tagLabel = new QLabel(tr("Tag:"));
    m_tagLineEdit = new QLineEdit;
    m_tagLineEdit->setText(data.tag);
    m_tagLineEdit->setEnabled(false);

    auto idLabel = new QLabel(tr("Image ID:"));
    m_idLineEdit = new QLineEdit;
    m_idLineEdit->setText(data.imageId);
    m_idLineEdit->setEnabled(false);

    auto daemonStateLabel = new QLabel(tr("Daemon state:"));
    m_daemonReset = new QToolButton;
    m_daemonReset->setToolTip(tr("Clears detected daemon state. "
        "It will be automatically re-evaluated next time access is needed."));

    m_daemonState = new QLabel;

    connect(DockerApi::instance(), &DockerApi::dockerDaemonAvailableChanged, this, [this]{
        updateDaemonStateTexts();
    });

    updateDaemonStateTexts();

    connect(m_daemonReset, &QToolButton::clicked, this, [] {
        DockerApi::recheckDockerDaemon();
    });

    m_runAsOutsideUser = new QCheckBox(tr("Run as outside user"));
    m_runAsOutsideUser->setToolTip(tr("Uses user ID and group ID of the user running Qt Creator "
                                      "in the docker container."));
    m_runAsOutsideUser->setChecked(data.useLocalUidGid);
    m_runAsOutsideUser->setEnabled(HostOsInfo::isLinuxHost());

    connect(m_runAsOutsideUser, &QCheckBox::toggled, this, [&data](bool on) {
        data.useLocalUidGid = on;
    });

    auto pathListLabel = new InfoLabel(tr("Paths to mount:"));
    pathListLabel->setAdditionalToolTip(tr("Source directory list should not be empty."));

    m_pathsListEdit = new PathListEditor;
    m_pathsListEdit->setPlaceholderText(tr("Host directories to mount into the container"));
    m_pathsListEdit->setToolTip(tr("Maps paths in this list one-to-one to the "
                                   "docker container."));
    m_pathsListEdit->setPathList(data.mounts);

    auto markupMounts = [this, pathListLabel] {
        const bool isEmpty = m_pathsListEdit->pathList().isEmpty();
        pathListLabel->setType(isEmpty ? InfoLabel::Warning : InfoLabel::None);
    };
    markupMounts();

    connect(m_pathsListEdit, &PathListEditor::changed, this, [dockerDevice, markupMounts, this] {
        dockerDevice->setMounts(m_pathsListEdit->pathList());
        markupMounts();
    });

    auto logView = new QTextBrowser;
    connect(&m_kitItemDetector, &KitDetector::logOutput,
            logView, &QTextBrowser::append);

    auto autoDetectButton = new QPushButton(tr("Auto-detect Kit Items"));
    auto undoAutoDetectButton = new QPushButton(tr("Remove Auto-Detected Kit Items"));
    auto listAutoDetectedButton = new QPushButton(tr("List Auto-Detected Kit Items"));

    auto searchDirsComboBox = new QComboBox;
    searchDirsComboBox->addItem(tr("Search in PATH"));
    searchDirsComboBox->addItem(tr("Search in Selected Directories"));

    auto searchDirsLineEdit = new FancyLineEdit;

    searchDirsLineEdit->setPlaceholderText(tr("Semicolon-separated list of directories"));
    searchDirsLineEdit->setToolTip(
        tr("Select the paths in the docker image that should be scanned for kit entries."));
    searchDirsLineEdit->setHistoryCompleter("DockerMounts", true);

    auto searchPaths = [searchDirsComboBox, searchDirsLineEdit, dockerDevice] {
        FilePaths paths;
        if (searchDirsComboBox->currentIndex() == 0) {
            paths = dockerDevice->systemEnvironment().path();
        } else {
            for (const QString &path : searchDirsLineEdit->text().split(';'))
                paths.append(FilePath::fromString(path.trimmed()));
        }
        paths = Utils::transform(paths, [dockerDevice](const FilePath &path) {
            return dockerDevice->mapToGlobalPath(path);
        });
        return paths;
    };

    connect(autoDetectButton, &QPushButton::clicked, this,
            [this, logView, dockerDevice, searchPaths] {
        logView->clear();
        dockerDevice->updateContainerAccess();

        m_kitItemDetector.autoDetect(dockerDevice->id().toString(), searchPaths());

        if (DockerApi::instance()->dockerDaemonAvailable().value_or(false) == false)
            logView->append(tr("Docker daemon appears to be not running."));
        else
            logView->append(tr("Docker daemon appears to be running."));
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

    Form {
        repoLabel, m_repoLineEdit, Break(),
        tagLabel, m_tagLineEdit, Break(),
        idLabel, m_idLineEdit, Break(),
        daemonStateLabel, m_daemonReset, m_daemonState, Break(),
        m_runAsOutsideUser, Break(),
        Column {
            pathListLabel,
            m_pathsListEdit,
        }, Break(),
        Column {
            Space(20),
            Row {
                searchDirsComboBox,
                searchDirsLineEdit
            },
            Row {
                autoDetectButton,
                undoAutoDetectButton,
                listAutoDetectedButton,
                Stretch(),
            },
            new QLabel(tr("Detection log:")),
            logView
        }
    }.attachTo(this);

    searchDirsLineEdit->setVisible(false);
    auto updateDirectoriesLineEdit = [searchDirsLineEdit](int index) {
        searchDirsLineEdit->setVisible(index == 1);
        if (index == 1)
            searchDirsLineEdit->setFocus();
    };
    QObject::connect(searchDirsComboBox, qOverload<int>(&QComboBox::activated),
                     this, updateDirectoriesLineEdit);
}

void DockerDeviceWidget::updateDaemonStateTexts()
{
    Utils::optional<bool> daemonState = DockerApi::instance()->dockerDaemonAvailable();
    if (!daemonState.has_value()) {
        m_daemonReset->setIcon(Icons::INFO.icon());
        m_daemonState->setText(tr("Daemon state not evaluated."));
    } else if (daemonState.value()) {
        m_daemonReset->setIcon(Icons::OK.icon());
        m_daemonState->setText(tr("Docker daemon running."));
    } else {
        m_daemonReset->setIcon(Icons::CRITICAL.icon());
        m_daemonState->setText(tr("Docker daemon not running."));
    }
}

} // Internal
} // Docker

// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(dockerDeviceLog)

namespace Docker::Constants {

inline constexpr char DOCKER[] = "docker";
inline constexpr char DOCKER_DEVICE_TYPE[] = "DockerDeviceType";

const char16_t DOCKER_DEVICE_SCHEME[] = u"docker";

inline constexpr char DOCKER_SETTINGS_ID[] = "Docker.Settings";

} // Docker::Constants

namespace Podman::Constants {

const char PODMAN[] = "podman";
const char PODMAN_DEVICE_TYPE[] = "PodmanDeviceType";
const char16_t PODMAN_DEVICE_SCHEME[] = u"podman";
const char PODMAN_SETTINGS_ID[] = "Podman.Settings";

} // Podman::Constants

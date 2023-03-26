// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(dockerDeviceLog)

namespace Docker::Constants {

const char DOCKER[] = "docker";
const char16_t DOCKER_DEVICE_SCHEME[] = u"docker";

const char DOCKER_SETTINGS_ID[] = "Docker.Settings";
const char DOCKER_DEVICE_TYPE[] = "DockerDeviceType";

} // Docker::Constants

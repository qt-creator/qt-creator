// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dashboard/dto.h"

#include <memory>

QT_BEGIN_NAMESPACE
class QIcon;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace Axivion::Internal {

void fetchProjectInfo(const QString &projectName);
std::optional<Dto::ProjectInfoDto> projectInfo();
bool handleCertificateIssue();

QIcon iconForIssue(const QString &prefix);

} // Axivion::Internal


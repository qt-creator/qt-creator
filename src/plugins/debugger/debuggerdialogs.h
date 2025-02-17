// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include <optional>

namespace ProjectExplorer { class Kit; }

namespace Debugger::Internal {

void runAttachToRemoteServerDialog();
void runStartAndDebugApplicationDialog();
void runStartRemoteCdbSessionDialog(ProjectExplorer::Kit *kit);
void runAttachToQmlPortDialog();

std::optional<quint64> runAddressDialog(quint64 initialAddress);

} // Debugger::Internal

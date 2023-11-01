// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>

namespace WebAssembly::Internal {

using WebBrowserEntry = QPair<QString, QString>; // first: id, second: display name
using WebBrowserEntries = QList<WebBrowserEntry>;

WebBrowserEntries parseEmrunOutput(const QByteArray &output);


class EmrunRunConfigurationFactory final : public ProjectExplorer::RunConfigurationFactory
{
public:
    EmrunRunConfigurationFactory();
};

class EmrunRunWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    EmrunRunWorkerFactory();
};

} // Webassembly::Internal

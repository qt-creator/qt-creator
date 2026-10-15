// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef WITH_TESTS
class QObject;
#endif

namespace ProjectExplorer::Internal {

void setupDesktopRunConfigurations();
void setupDesktopRunWorker();

#ifdef WITH_TESTS
QObject *createDesktopRunConfigurationTest();
#endif

} // ProjectExplorer::Internal

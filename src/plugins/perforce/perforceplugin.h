// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Perforce::Internal {

class PerforcePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Perforce.json")

    ~PerforcePlugin() final;

    void initialize() final;
    void extensionsInitialized() final;

public:
    // Map a perforce name "//xx" to its real name in the file system
    static QString fileNameFromPerforceName(const QString& perforceName,
                                            bool quiet,
                                            QString *errorMessage);

#ifdef WITH_TESTS
private slots:
    void testLogResolving();
#endif
};

} // Perforce::Internal

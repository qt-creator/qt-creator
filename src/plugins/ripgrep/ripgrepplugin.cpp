// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findinfilesripgrep.h"
#include "ripgrepengine_test.h"
#include "ripgrepparser_test.h"

#include <extensionsystem/iplugin.h>

namespace Ripgrep::Internal {

class RipgrepPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Ripgrep.json")

    void initialize() final
    {
        setupFindInFilesRipgrep();

#ifdef WITH_TESTS
        addTest<EngineTest>();
        addTest<OutputParserTest>();
#endif
    }
};

} // Ripgrep::Internal

#include "ripgrepplugin.moc"

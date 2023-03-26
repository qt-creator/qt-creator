// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace SilverSearcher::Internal {

class SilverSearcherPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "SilverSearcher.json")

public:
    void initialize() override;
};

} // SilverSearcher::Internal

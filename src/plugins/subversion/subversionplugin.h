// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Subversion {
namespace Internal {

const char FileAddedC[]      = "A";
const char FileConflictedC[] = "C";
const char FileDeletedC[]    = "D";
const char FileModifiedC[]   = "M";

class SubversionPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Subversion.json")

    ~SubversionPlugin() final;

    void initialize() final;
    void extensionsInitialized() final;

#ifdef WITH_TESTS
private slots:
    void testLogResolving();
#endif

};

} // namespace Subversion
} // namespace Internal

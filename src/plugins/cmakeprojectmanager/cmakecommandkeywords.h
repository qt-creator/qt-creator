// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakebuildsystem.h"
#include "cmaketool.h"

#include <cmakelang/cmakesignature.h>

#include <QHash>
#include <QPointer>
#include <QSet>

namespace CMakeProjectManager::Internal {

// The keywords a command takes: from the documentation of CMake for the
// commands CMake itself brings, and from the cmake_parse_arguments() calls of
// the project for the ones it defines.
class CommandKeywords
{
public:
    // Reads them anew where the project has been configured since.
    void refresh();

    bool contains(const QString &command, const QString &argument);

private:
    CMakeKeywords m_cmakeKeywords;
    CMakeLang::SignatureTable m_signatures;
    QHash<QString, QSet<QString>> m_perCommand;
    QPointer<CMakeBuildSystem> m_buildSystem;
    int m_generation = -1;
};

} // namespace CMakeProjectManager::Internal

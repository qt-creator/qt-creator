// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace GenericProjectManager::Constants {

inline constexpr char GENERICMIMETYPE[]    = "text/x-generic-project"; // ### FIXME

inline constexpr char GENERIC_MS_ID[] = "GenericProjectManager.GenericMakeStep";

// Contexts
inline constexpr char FILES_EDITOR_ID[]    = "QT4.FilesEditor";

// Project
inline constexpr char GENERICPROJECT_ID[]  = "GenericProjectManager.GenericProject";

// File Templates
inline constexpr char GENERICPROJECT_CXXFLAGS_FILE_TEMPLATE[] = "-std=c++17";
inline constexpr char GENERICPROJECT_CFLAGS_FILE_TEMPLATE[] = "-std=c17";

} // namespace GenericProjectManager::Constants

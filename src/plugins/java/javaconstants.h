// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Java::Constants {

// The ids are the ones the Android plugin registered these with, so that the
// editors and the language servers a user has set up keep working.
inline constexpr char JAVA_EDITOR_ID[] = "java.editor";
inline constexpr char JLS_SETTINGS_ID[] = "Java::JLSSettingsID";

// What a build system tells the Java support about a target of its, through
// the extra data of that target. This way a build system says where the Java
// of a project is without depending on the Java plugin, and the Java plugin
// reads it without knowing which build system wrote it.
inline constexpr char JavaSourceFiles[] = "JavaSourceFiles"; // QStringList
inline constexpr char JavaClassPath[] = "JavaClassPath";     // QStringList

} // namespace Java::Constants

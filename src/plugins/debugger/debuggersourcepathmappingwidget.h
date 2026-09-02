// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QStringList>

namespace Debugger { class DebuggerRunParameters; }
namespace QtSupport { class QtVersion; }

namespace Debugger::Internal {

using SourcePathMap = QMap<QString, QString>;

/* The Qt source roots (the directories containing qtbase/) recorded in the
 * debug information of the kit's Qt. Reads from disk, so this is meant to be
 * called once per run and the result passed along in the run parameters. */
QStringList qtBuildSourceRoots(const DebuggerRunParameters &sp, const QtSupport::QtVersion *qt);

/* The same extraction on the raw contents of a .debug_str section. */
QStringList qtBuildSourceRoots(const QByteArray &debugStrings);

/* Gdb's debug-file-directory for the given run: the location set for the run,
 * or gdb's own default below the sysroot. */
Utils::FilePath debugInfoDirectory(const DebuggerRunParameters &sp);

/* The file carrying the debug information of an ELF library: the library itself,
 * or the companion its .gnu_debuglink or its build id points to. debugInfoDir is
 * gdb's debug-file-directory, normally <sysroot>/usr/lib/debug. */
Utils::FilePath debugInfoFile(const Utils::FilePath &library, const QByteArray &debugLink,
                              const QByteArray &buildId, const Utils::FilePath &debugInfoDir);

/* Merge settings for an installed Qt (unless another setting
 * is already in the map. */
SourcePathMap mergePlatformQtPath(const DebuggerRunParameters &sp, const SourcePathMap &in);

/* The same merge on plain values. */
SourcePathMap mergePlatformQtPath(const QString &qtSourceLocation,
                                  const QStringList &qtBuildSourceRoots,
                                  const SourcePathMap &in);

/* Merge the run parameters' own mappings over the given ones, expanding
 * macros. User settings win. */
SourcePathMap mergeStartParametersSourcePathMap(const DebuggerRunParameters &sp,
                                                const SourcePathMap &in);

} // Debugger::Internal

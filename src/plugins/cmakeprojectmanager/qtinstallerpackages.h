// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringList>

namespace CMakeProjectManager::Internal {

// The platform of the Qt Online Installer that a Qt build flavor - the
// directory a Qt installation sits in - belongs to.
QString qtInstallerPlatform(const QString &buildFlavor, bool crossCompiled);

// The packages of the Qt Online Installer that ship the Qt components, each of
// them once.
QStringList qtInstallerPackages(const QStringList &components,
                                const QString &qtVersion,
                                const QString &platform);

} // CMakeProjectManager::Internal

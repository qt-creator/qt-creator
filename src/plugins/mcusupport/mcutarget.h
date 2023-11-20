// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mcusupport_global.h"

#include <QObject>
#include <QVersionNumber>

namespace ProjectExplorer {
class ToolChain;
}

namespace Utils {
class PathChooser;
class InfoLabel;
} // namespace Utils

namespace McuSupport::Internal {

class McuToolChainPackage;

class McuTarget : public QObject
{
    Q_OBJECT

public:
    enum class OS { Desktop, BareMetal, FreeRTOS };

    struct Platform
    {
        QString name;
        QString displayName;
        QString vendor;
    };

    enum { UnspecifiedColorDepth = -1 };

    McuTarget(const QVersionNumber &qulVersion,
              const Platform &platform,
              OS os,
              const Packages &packages,
              const McuToolChainPackagePtr &toolChainPackage,
              const McuPackagePtr &toolChainFilePackage,
              int colorDepth = UnspecifiedColorDepth);

    QVersionNumber qulVersion() const;
    Packages packages() const;
    McuToolChainPackagePtr toolChainPackage() const;
    McuPackagePtr toolChainFilePackage() const;
    Platform platform() const;
    OS os() const;
    int colorDepth() const;
    bool isValid() const;
    QString desktopCompilerId() const;
    void handlePackageProblems(MessagesList &messages) const;

    // Used when updating to new version of QtMCUs
    // Paths that is not valid in the new version,
    // and were valid in the old version. have the possibility be valid if
    // reset to the default value without user intervention
    void resetInvalidPathsToDefault();

private:
    const QVersionNumber m_qulVersion;
    const Platform m_platform;
    const OS m_os;
    const Packages m_packages;
    McuToolChainPackagePtr m_toolChainPackage;
    McuPackagePtr m_toolChainFilePackage;
    const int m_colorDepth;
}; // class McuTarget

} // namespace McuSupport::Internal

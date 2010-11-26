/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef RVCTTOOLCHAIN_H
#define RVCTTOOLCHAIN_H

#include "s60devices.h"

#include <projectexplorer/toolchain.h>

namespace Qt4ProjectManager {
namespace Internal {

// ==========================================================================
// RVCTToolChain
// ==========================================================================

class RVCTToolChain : public ProjectExplorer::ToolChain
{
public:
    explicit RVCTToolChain(const S60Devices::Device &device,
                           ProjectExplorer::ToolChainType type);
    QByteArray predefinedMacros();
    QList<ProjectExplorer::HeaderPath> systemHeaderPaths();
    void addToEnvironment(Utils::Environment &env);
    ProjectExplorer::ToolChainType type() const;
    QString makeCommand() const;
    ProjectExplorer::IOutputParser *outputParser() const;

    static QSet<QPair<int, int> > configuredRvctVersions();

    // Return the environment variable indicating the RVCT version
    // 'RVCT2<minor>BIN' and its setting
    virtual QByteArray rvctBinEnvironmentVariable() = 0;

    QString rvctBinPath();
    QString rvctBinary();

protected:
    bool equals(const ToolChain *other) const = 0;

    QStringList configuredEnvironment();

    QByteArray rvctBinEnvironmentVariableForVersion(int major);
    void addToRVCTPathVariable(const QString &postfix, const QStringList &values,
                               Utils::Environment &env) const;
    QStringList libPaths();
    void updateVersion();

    QByteArray m_predefinedMacros;
    QList<ProjectExplorer::HeaderPath> m_systemHeaderPaths;

    const S60ToolChainMixin m_mixin;
    const ProjectExplorer::ToolChainType m_type;
    bool m_versionUpToDate;
    int m_major;
    int m_minor;
    int m_build;

private:
    QString m_binPath;
    QStringList m_additionalEnvironment;
};

// ==========================================================================
// RVCT2ToolChain
// ==========================================================================

class RVCT2ToolChain : public RVCTToolChain
{
public:
    explicit RVCT2ToolChain(const S60Devices::Device &device,
                            ProjectExplorer::ToolChainType type);
    QByteArray rvctBinEnvironmentVariable();

    QByteArray predefinedMacros();

protected:
    bool equals(const ToolChain *other) const;
};

// ==========================================================================
// RVCT4ToolChain
// ==========================================================================

class RVCT4ToolChain : public RVCT2ToolChain
{
public:
    explicit RVCT4ToolChain(const S60Devices::Device &device,
                            ProjectExplorer::ToolChainType type);

    QByteArray rvctBinEnvironmentVariable();

    QByteArray predefinedMacros();

protected:
    bool equals(const ToolChain *other) const;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // RVCTTOOLCHAIN_H

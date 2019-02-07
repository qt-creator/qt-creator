/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "clangdiagnosticconfig.h"

namespace CppTools {

Core::Id ClangDiagnosticConfig::id() const
{
    return m_id;
}

void ClangDiagnosticConfig::setId(const Core::Id &id)
{
    m_id = id;
}

QString ClangDiagnosticConfig::displayName() const
{
    return m_displayName;
}

void ClangDiagnosticConfig::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
}

QStringList ClangDiagnosticConfig::clangOptions() const
{
    return m_clangOptions;
}

void ClangDiagnosticConfig::setClangOptions(const QStringList &options)
{
    m_clangOptions = options;
}

bool ClangDiagnosticConfig::isReadOnly() const
{
    return m_isReadOnly;
}

void ClangDiagnosticConfig::setIsReadOnly(bool isReadOnly)
{
    m_isReadOnly = isReadOnly;
}

bool ClangDiagnosticConfig::operator==(const ClangDiagnosticConfig &other) const
{
    return m_id == other.m_id
        && m_displayName == other.m_displayName
        && m_clangOptions == other.m_clangOptions
        && m_clangTidyMode == other.m_clangTidyMode
        && m_clangTidyChecks == other.m_clangTidyChecks
        && m_clazyChecks == other.m_clazyChecks
        && m_isReadOnly == other.m_isReadOnly
        && m_useBuildSystemWarnings == other.m_useBuildSystemWarnings;
}

bool ClangDiagnosticConfig::operator!=(const ClangDiagnosticConfig &other) const
{
    return !(*this == other);
}

bool ClangDiagnosticConfig::useBuildSystemWarnings() const
{
    return m_useBuildSystemWarnings;
}

void ClangDiagnosticConfig::setUseBuildSystemWarnings(bool useBuildSystemWarnings)
{
    m_useBuildSystemWarnings = useBuildSystemWarnings;
}

ClangDiagnosticConfig::TidyMode ClangDiagnosticConfig::clangTidyMode() const
{
    return m_clangTidyMode;
}

void ClangDiagnosticConfig::setClangTidyMode(TidyMode mode)
{
    m_clangTidyMode = mode;
}

QString ClangDiagnosticConfig::clangTidyChecks() const
{
    return m_clangTidyChecks;
}

void ClangDiagnosticConfig::setClangTidyChecks(const QString &checks)
{
    m_clangTidyChecks = checks;
}

QString ClangDiagnosticConfig::clazyChecks() const
{
    return m_clazyChecks;
}

void ClangDiagnosticConfig::setClazyChecks(const QString &checks)
{
    m_clazyChecks = checks;
}

} // namespace CppTools

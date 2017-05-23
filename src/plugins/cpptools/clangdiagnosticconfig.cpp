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

QStringList ClangDiagnosticConfig::commandLineWarnings() const
{
    return m_commandLineWarnings;
}

void ClangDiagnosticConfig::setCommandLineWarnings(const QStringList &warnings)
{
    m_commandLineWarnings = warnings;
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
        && m_commandLineWarnings == other.m_commandLineWarnings
        && m_isReadOnly == other.m_isReadOnly;
}

} // namespace CppTools

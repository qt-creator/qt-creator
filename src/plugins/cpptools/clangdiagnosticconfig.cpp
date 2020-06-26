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
#include "cpptoolsreuse.h"

#include <utils/qtcassert.h>

#include <QSettings>

namespace CppTools {

Utils::Id ClangDiagnosticConfig::id() const
{
    return m_id;
}

void ClangDiagnosticConfig::setId(const Utils::Id &id)
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
        && m_clazyMode == other.m_clazyMode
        && m_clazyChecks == other.m_clazyChecks
        && m_isReadOnly == other.m_isReadOnly
        && m_useBuildSystemWarnings == other.m_useBuildSystemWarnings;
}

bool ClangDiagnosticConfig::operator!=(const ClangDiagnosticConfig &other) const
{
    return !(*this == other);
}

ClangDiagnosticConfig::ClazyMode ClangDiagnosticConfig::clazyMode() const
{
    return m_clazyMode;
}

void ClangDiagnosticConfig::setClazyMode(const ClazyMode &clazyMode)
{
    m_clazyMode = clazyMode;
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

bool ClangDiagnosticConfig::isClangTidyEnabled() const
{
    return m_clangTidyMode != TidyMode::UseCustomChecks || clangTidyChecks() != "-*";
}

QString ClangDiagnosticConfig::clazyChecks() const
{
    return m_clazyChecks;
}

void ClangDiagnosticConfig::setClazyChecks(const QString &checks)
{
    m_clazyChecks = checks;
}

bool ClangDiagnosticConfig::isClazyEnabled() const
{
    return m_clazyMode != ClazyMode::UseCustomChecks || !m_clazyChecks.isEmpty();
}

static QString convertToNewClazyChecksFormat(const QString &checks)
{
    // Before Qt Creator 4.9 valid values for checks were: "", "levelN".
    // Starting with Qt Creator 4.9, checks are a comma-separated string of checks: "x,y,z".

    if (checks.isEmpty())
        return {};
    if (checks.size() == 6 && checks.startsWith("level"))
        return {};
    return checks;
}

static const char diagnosticConfigsArrayKey[] = "ClangDiagnosticConfigs";
static const char diagnosticConfigIdKey[] = "id";
static const char diagnosticConfigDisplayNameKey[] = "displayName";
static const char diagnosticConfigWarningsKey[] = "diagnosticOptions";
static const char useBuildSystemFlagsKey[] = "useBuildSystemFlags";
static const char diagnosticConfigsTidyChecksKey[] = "clangTidyChecks";
static const char diagnosticConfigsTidyModeKey[] = "clangTidyMode";
static const char diagnosticConfigsClazyModeKey[] = "clazyMode";
static const char diagnosticConfigsClazyChecksKey[] = "clazyChecks";

void diagnosticConfigsToSettings(QSettings *s, const ClangDiagnosticConfigs &configs)
{
    s->beginWriteArray(diagnosticConfigsArrayKey);
    for (int i = 0, size = configs.size(); i < size; ++i) {
        const ClangDiagnosticConfig &config = configs.at(i);
        s->setArrayIndex(i);
        s->setValue(diagnosticConfigIdKey, config.id().toSetting());
        s->setValue(diagnosticConfigDisplayNameKey, config.displayName());
        s->setValue(diagnosticConfigWarningsKey, config.clangOptions());
        s->setValue(useBuildSystemFlagsKey, config.useBuildSystemWarnings());
        s->setValue(diagnosticConfigsTidyModeKey, int(config.clangTidyMode()));
        s->setValue(diagnosticConfigsTidyChecksKey, config.clangTidyChecks());
        s->setValue(diagnosticConfigsClazyModeKey, int(config.clazyMode()));
        s->setValue(diagnosticConfigsClazyChecksKey, config.clazyChecks());
    }
    s->endArray();
}

ClangDiagnosticConfigs diagnosticConfigsFromSettings(QSettings *s)
{
    ClangDiagnosticConfigs configs;

    const int size = s->beginReadArray(diagnosticConfigsArrayKey);
    for (int i = 0; i < size; ++i) {
        s->setArrayIndex(i);

        ClangDiagnosticConfig config;
        config.setId(Utils::Id::fromSetting(s->value(diagnosticConfigIdKey)));
        config.setDisplayName(s->value(diagnosticConfigDisplayNameKey).toString());
        config.setClangOptions(s->value(diagnosticConfigWarningsKey).toStringList());
        config.setUseBuildSystemWarnings(s->value(useBuildSystemFlagsKey, false).toBool());
        const int tidyModeValue = s->value(diagnosticConfigsTidyModeKey).toInt();
        if (tidyModeValue == 0) { // Convert from settings of <= Qt Creator 4.10
            config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::UseCustomChecks);
            config.setClangTidyChecks("-*");
        } else {
            config.setClangTidyMode(static_cast<ClangDiagnosticConfig::TidyMode>(tidyModeValue));
            config.setClangTidyChecks(s->value(diagnosticConfigsTidyChecksKey).toString());
        }

        config.setClazyMode(static_cast<ClangDiagnosticConfig::ClazyMode>(
            s->value(diagnosticConfigsClazyModeKey).toInt()));
        const QString clazyChecks = s->value(diagnosticConfigsClazyChecksKey).toString();
        config.setClazyChecks(convertToNewClazyChecksFormat(clazyChecks));
        configs.append(config);
    }
    s->endArray();

    return configs;
}

} // namespace CppTools

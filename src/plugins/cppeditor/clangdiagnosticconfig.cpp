// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdiagnosticconfig.h"
#include "cpptoolsreuse.h"

#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>

using namespace Utils;

namespace CppEditor {

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

const QStringList ClangDiagnosticConfig::clangOptions() const
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
        && m_tidyChecksOptions == other.m_tidyChecksOptions
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

QString ClangDiagnosticConfig::clangTidyChecksAsJson() const
{
    QString jsonString = "{Checks: '" + checks(ClangToolType::Tidy)
            + ",-clang-diagnostic-*', CheckOptions: [";

    // The check is either listed verbatim or covered by the "<prefix>-*" pattern.
    const auto checkIsEnabled = [this](const QString &check) {
        for (QString subString = check; !subString.isEmpty();
             subString.chop(subString.length() - subString.lastIndexOf('-'))) {
            const int idx = m_clangTidyChecks.indexOf(subString);
            if (idx == -1)
                continue;
            if (idx > 0 && m_clangTidyChecks.at(idx - 1) == '-')
                continue;
            if (subString == check || QStringView(m_clangTidyChecks)
                    .mid(idx + subString.length()).startsWith(QLatin1String("-*"))) {
                return true;
            }
        }
        return false;
    };

    QString optionString;
    for (auto it = m_tidyChecksOptions.cbegin(); it != m_tidyChecksOptions.cend(); ++it) {
        if (!checkIsEnabled(it.key()))
            continue;
        for (auto optIt = it.value().begin(); optIt != it.value().end(); ++optIt) {
            if (!optionString.isEmpty())
                optionString += ',';
            optionString += "{key: '" + it.key() + '.' + optIt.key()
                    + "', value: '" + optIt.value() + "'}";
        }
    }
    jsonString += optionString;
    return jsonString += "]}";
}

void ClangDiagnosticConfig::setTidyCheckOptions(const QString &check,
                                                const TidyCheckOptions &options)
{
    m_tidyChecksOptions[check] = options;
}

ClangDiagnosticConfig::TidyCheckOptions
ClangDiagnosticConfig::tidyCheckOptions(const QString &check) const
{
    return m_tidyChecksOptions.value(check);
}

void ClangDiagnosticConfig::setTidyChecksOptionsFromSettings(const QVariant &options)
{
    const QVariantMap topLevelMap = options.toMap();
    for (auto it = topLevelMap.begin(); it != topLevelMap.end(); ++it) {
        const QVariantMap optionsMap = it.value().toMap();
        TidyCheckOptions options;
        for (auto optIt = optionsMap.begin(); optIt != optionsMap.end(); ++optIt)
            options.insert(optIt.key(), optIt.value().toString());
        m_tidyChecksOptions.insert(it.key(), options);
    }
}

QVariant ClangDiagnosticConfig::tidyChecksOptionsForSettings() const
{
    QVariantMap topLevelMap;
    for (auto it = m_tidyChecksOptions.cbegin(); it != m_tidyChecksOptions.cend(); ++it) {
        QVariantMap optionsMap;
        for (auto optIt = it.value().begin(); optIt != it.value().end(); ++optIt)
            optionsMap.insert(optIt.key(), optIt.value());
        topLevelMap.insert(it.key(), optionsMap);
    }
    return topLevelMap;
}

QString ClangDiagnosticConfig::checks(ClangToolType tool) const
{
    return tool == ClangToolType::Tidy ? m_clangTidyChecks : m_clazyChecks;
}

void ClangDiagnosticConfig::setChecks(ClangToolType tool, const QString &checks)
{
    if (tool == ClangToolType::Tidy)
        m_clangTidyChecks = checks;
    else
        m_clazyChecks = checks;
}

bool ClangDiagnosticConfig::isEnabled(ClangToolType tool) const
{
    if (tool == ClangToolType::Tidy)
        return m_clangTidyMode != TidyMode::UseCustomChecks || checks(ClangToolType::Tidy) != "-*";
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
static const char diagnosticConfigsTidyChecksOptionsKey[] = "clangTidyChecksOptions";
static const char diagnosticConfigsTidyModeKey[] = "clangTidyMode";
static const char diagnosticConfigsClazyModeKey[] = "clazyMode";
static const char diagnosticConfigsClazyChecksKey[] = "clazyChecks";

void diagnosticConfigsToSettings(QtcSettings *s, const ClangDiagnosticConfigs &configs)
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
        s->setValue(diagnosticConfigsTidyChecksKey, config.checks(ClangToolType::Tidy));
        s->setValue(diagnosticConfigsTidyChecksOptionsKey, config.tidyChecksOptionsForSettings());
        s->setValue(diagnosticConfigsClazyModeKey, int(config.clazyMode()));
        s->setValue(diagnosticConfigsClazyChecksKey, config.checks(ClangToolType::Clazy));
    }
    s->endArray();
}

ClangDiagnosticConfigs diagnosticConfigsFromSettings(QtcSettings *s)
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
            config.setChecks(ClangToolType::Tidy, "-*");
        } else {
            config.setClangTidyMode(static_cast<ClangDiagnosticConfig::TidyMode>(tidyModeValue));
            config.setChecks(ClangToolType::Tidy, s->value(diagnosticConfigsTidyChecksKey).toString());
            config.setTidyChecksOptionsFromSettings(
                        s->value(diagnosticConfigsTidyChecksOptionsKey));
        }

        config.setClazyMode(static_cast<ClangDiagnosticConfig::ClazyMode>(
            s->value(diagnosticConfigsClazyModeKey).toInt()));
        const QString clazyChecks = s->value(diagnosticConfigsClazyChecksKey).toString();
        config.setChecks(ClangToolType::Clazy, convertToNewClazyChecksFormat(clazyChecks));
        configs.append(config);
    }
    s->endArray();

    return configs;
}

} // namespace CppEditor

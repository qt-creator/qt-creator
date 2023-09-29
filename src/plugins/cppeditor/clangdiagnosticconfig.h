// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <utils/id.h>

#include <QHash>
#include <QMap>
#include <QStringList>
#include <QVector>

namespace Utils { class QtcSettings; }

namespace CppEditor {

enum class ClangToolType { Tidy, Clazy };

// TODO: Split this class as needed for ClangCodeModel and ClangTools
class CPPEDITOR_EXPORT ClangDiagnosticConfig
{
public:
    Utils::Id id() const;
    void setId(const Utils::Id &id);

    QString displayName() const;
    void setDisplayName(const QString &displayName);

    bool isReadOnly() const;
    void setIsReadOnly(bool isReadOnly);

    const QStringList clangOptions() const;
    void setClangOptions(const QStringList &options);

    bool useBuildSystemWarnings() const;
    void setUseBuildSystemWarnings(bool useBuildSystemWarnings);

    // Clang-Tidy
    enum class TidyMode
    {
        UseCustomChecks = 1,
        UseDefaultChecks = 3,
    };
    TidyMode clangTidyMode() const;
    void setClangTidyMode(TidyMode mode);

    QString clangTidyChecksAsJson() const;

    using TidyCheckOptions = QMap<QString, QString>;
    void setTidyCheckOptions(const QString &check, const TidyCheckOptions &options);
    TidyCheckOptions tidyCheckOptions(const QString &check) const;
    void setTidyChecksOptionsFromSettings(const QVariant &options);
    QVariant tidyChecksOptionsForSettings() const;

    // Clazy
    enum class ClazyMode
    {
        UseDefaultChecks,
        UseCustomChecks,
    };
    ClazyMode clazyMode() const;
    void setClazyMode(const ClazyMode &clazyMode);

    QString checks(ClangToolType tool) const;
    void setChecks(ClangToolType tool, const QString &checks);

    bool isEnabled(ClangToolType tool) const;

    bool operator==(const ClangDiagnosticConfig &other) const;
    bool operator!=(const ClangDiagnosticConfig &other) const;

private:
    Utils::Id m_id;
    QString m_displayName;
    QStringList m_clangOptions;
    TidyMode m_clangTidyMode = TidyMode::UseDefaultChecks;
    QString m_clangTidyChecks;
    QHash<QString, TidyCheckOptions> m_tidyChecksOptions;
    QString m_clazyChecks;
    ClazyMode m_clazyMode = ClazyMode::UseDefaultChecks;
    bool m_isReadOnly = false;
    bool m_useBuildSystemWarnings = false;
};

using ClangDiagnosticConfigs = QVector<ClangDiagnosticConfig>;

ClangDiagnosticConfigs CPPEDITOR_EXPORT diagnosticConfigsFromSettings(Utils::QtcSettings *s);
void CPPEDITOR_EXPORT diagnosticConfigsToSettings(Utils::QtcSettings *s,
                                                  const ClangDiagnosticConfigs &configs);

} // namespace CppEditor

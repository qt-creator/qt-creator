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

#pragma once

#include "cpptools_global.h"

#include <utils/id.h>

#include <QStringList>
#include <QVector>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace CppTools {

// TODO: Split this class as needed for ClangCodeModel and ClangTools
class CPPTOOLS_EXPORT ClangDiagnosticConfig
{
public:
    Utils::Id id() const;
    void setId(const Utils::Id &id);

    QString displayName() const;
    void setDisplayName(const QString &displayName);

    bool isReadOnly() const;
    void setIsReadOnly(bool isReadOnly);

    QStringList clangOptions() const;
    void setClangOptions(const QStringList &options);

    bool useBuildSystemWarnings() const;
    void setUseBuildSystemWarnings(bool useBuildSystemWarnings);

    // Clang-Tidy
    enum class TidyMode
    {
        // Disabled, // Used by Qt Creator 4.10 and below.
        UseCustomChecks = 1,
        UseConfigFile,
        UseDefaultChecks,
    };
    TidyMode clangTidyMode() const;
    void setClangTidyMode(TidyMode mode);

    QString clangTidyChecks() const;
    void setClangTidyChecks(const QString &checks);

    bool isClangTidyEnabled() const;

    // Clazy
    enum class ClazyMode
    {
        UseDefaultChecks,
        UseCustomChecks,
    };
    ClazyMode clazyMode() const;
    void setClazyMode(const ClazyMode &clazyMode);

    QString clazyChecks() const;
    void setClazyChecks(const QString &checks);

    bool isClazyEnabled() const;

    bool operator==(const ClangDiagnosticConfig &other) const;
    bool operator!=(const ClangDiagnosticConfig &other) const;

private:
    Utils::Id m_id;
    QString m_displayName;
    QStringList m_clangOptions;
    TidyMode m_clangTidyMode = TidyMode::UseDefaultChecks;
    QString m_clangTidyChecks;
    QString m_clazyChecks;
    ClazyMode m_clazyMode = ClazyMode::UseDefaultChecks;
    bool m_isReadOnly = false;
    bool m_useBuildSystemWarnings = false;
};

using ClangDiagnosticConfigs = QVector<ClangDiagnosticConfig>;

ClangDiagnosticConfigs CPPTOOLS_EXPORT diagnosticConfigsFromSettings(QSettings *s);
void CPPTOOLS_EXPORT diagnosticConfigsToSettings(QSettings *s,
                                                 const ClangDiagnosticConfigs &configs);

} // namespace CppTools

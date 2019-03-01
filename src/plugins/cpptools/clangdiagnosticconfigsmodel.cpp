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

#include "clangdiagnosticconfigsmodel.h"

#include "cpptoolsreuse.h"
#include "cpptoolsconstants.h"

#include <utils/algorithm.h>

#include <QCoreApplication>
#include <QUuid>

namespace CppTools {

static void addConfigForQuestionableConstructs(ClangDiagnosticConfigsModel &model)
{
    ClangDiagnosticConfig config;
    config.setId("Builtin.Questionable");
    config.setDisplayName(QCoreApplication::translate(
                              "ClangDiagnosticConfigsModel",
                              "Clang-only checks for questionable constructs"));
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{
        QStringLiteral("-Wall"),
        QStringLiteral("-Wextra"),
    });

    model.appendOrUpdate(config);
}

static void addConfigForPedanticWarnings(ClangDiagnosticConfigsModel &model)
{
    ClangDiagnosticConfig config;
    config.setId("Builtin.Pedantic");
    config.setDisplayName(QCoreApplication::translate("ClangDiagnosticConfigsModel",
                                                      "Clang-only pedantic checks"));
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{QStringLiteral("-Wpedantic")});

    model.appendOrUpdate(config);
}

constexpr const char *DEFAULT_TIDY_CHECKS = "-*,"
                                            "bugprone-*,"
                                            "cppcoreguidelines-*,"
                                            "misc-*,"
                                            "modernize-*,"
                                            "performance-*,"
                                            "readability-*,"
                                            "-cppcoreguidelines-owning-memory,"
                                            "-readability-braces-around-statements,"
                                            "-readability-implicit-bool-conversion,"
                                            "-readability-named-parameter";

static void addConfigForAlmostEveryWarning(ClangDiagnosticConfigsModel &model)
{
    ClangDiagnosticConfig config;
    config.setId(Constants::CPP_CLANG_BUILTIN_CONFIG_ID_EVERYTHING_WITH_EXCEPTIONS);
    config.setDisplayName(QCoreApplication::translate(
                              "ClangDiagnosticConfigsModel",
                              "Clang-only checks for almost everything"));
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{
        QStringLiteral("-Weverything"),
        QStringLiteral("-Wno-c++98-compat"),
        QStringLiteral("-Wno-c++98-compat-pedantic"),
        QStringLiteral("-Wno-unused-macros"),
        QStringLiteral("-Wno-newline-eof"),
        QStringLiteral("-Wno-exit-time-destructors"),
        QStringLiteral("-Wno-global-constructors"),
        QStringLiteral("-Wno-gnu-zero-variadic-macro-arguments"),
        QStringLiteral("-Wno-documentation"),
        QStringLiteral("-Wno-shadow"),
        QStringLiteral("-Wno-switch-enum"),
        QStringLiteral("-Wno-missing-prototypes"), // Not optimal for C projects.
        QStringLiteral("-Wno-used-but-marked-unused"), // e.g. QTest::qWait
    });

    model.appendOrUpdate(config);
}

static void addConfigForTidy(ClangDiagnosticConfigsModel &model)
{
    ClangDiagnosticConfig config;
    config.setId("Builtin.Tidy");
    config.setDisplayName(QCoreApplication::translate("ClangDiagnosticConfigsModel",
                                                      "Clang-Tidy thorough checks"));
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{QStringLiteral("-w")});
    config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::ChecksPrefixList);
    config.setClangTidyChecks(QString::fromUtf8(DEFAULT_TIDY_CHECKS));

    model.appendOrUpdate(config);
}

static void addConfigForClangAnalyze(ClangDiagnosticConfigsModel &model)
{
    ClangDiagnosticConfig config;
    config.setId("Builtin.TidyClangAnalyze");
    config.setDisplayName(QCoreApplication::translate(
                              "ClangDiagnosticConfigsModel",
                              "Clang-Tidy static analyzer checks"));
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{
        QStringLiteral("-w"),
    });
    config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::ChecksPrefixList);
    config.setClangTidyChecks("-*,clang-analyzer-*");

    model.appendOrUpdate(config);
}

static void addConfigForClazy(ClangDiagnosticConfigsModel &model)
{
    ClangDiagnosticConfig config;
    config.setId("Builtin.Clazy");
    config.setDisplayName(QCoreApplication::translate("ClangDiagnosticConfigsModel",
                                                      "Clazy level0 checks"));
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{QStringLiteral("-w")});
    config.setClazyChecks(clazyChecksForLevel(0));

    model.appendOrUpdate(config);
}

static void addConfigForTidyAndClazy(ClangDiagnosticConfigsModel &model)
{
    ClangDiagnosticConfig config;
    config.setId("Builtin.TidyAndClazy");
    config.setDisplayName(QCoreApplication::translate("ClangDiagnosticConfigsModel",
                                                      "Clang-Tidy and Clazy preselected checks"));
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{QStringLiteral("-w")});
    config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::ChecksPrefixList);

    config.setClangTidyChecks(QString::fromUtf8(DEFAULT_TIDY_CHECKS));
    config.setClazyChecks(clazyChecksForLevel(0));

    model.appendOrUpdate(config);
}

static void addConfigForBuildSystem(ClangDiagnosticConfigsModel &model)
{
    ClangDiagnosticConfig config;
    config.setId("Builtin.BuildSystem");
    config.setDisplayName(QCoreApplication::translate("ClangDiagnosticConfigsModel",
                                                      "Build-system warnings"));
    config.setIsReadOnly(true);
    config.setUseBuildSystemWarnings(true);

    model.appendOrUpdate(config);
}

static void addBuiltinConfigs(ClangDiagnosticConfigsModel &model)
{
    addConfigForPedanticWarnings(model);
    addConfigForQuestionableConstructs(model);
    addConfigForAlmostEveryWarning(model);
    addConfigForTidy(model);
    addConfigForClangAnalyze(model);
    addConfigForClazy(model);
    addConfigForTidyAndClazy(model);
    addConfigForBuildSystem(model);
}

ClangDiagnosticConfigsModel::ClangDiagnosticConfigsModel(const ClangDiagnosticConfigs &customConfigs)
{
    addBuiltinConfigs(*this);
    m_diagnosticConfigs.append(customConfigs);
}

int ClangDiagnosticConfigsModel::size() const
{
    return m_diagnosticConfigs.size();
}

const ClangDiagnosticConfig &ClangDiagnosticConfigsModel::at(int index) const
{
    return m_diagnosticConfigs.at(index);
}

void ClangDiagnosticConfigsModel::appendOrUpdate(const ClangDiagnosticConfig &config)
{
    const int index = indexOfConfig(config.id());

    if (index >= 0 && index < m_diagnosticConfigs.size())
        m_diagnosticConfigs.replace(index, config);
    else
        m_diagnosticConfigs.append(config);
}

void ClangDiagnosticConfigsModel::removeConfigWithId(const Core::Id &id)
{
    m_diagnosticConfigs.removeOne(configWithId(id));
}

ClangDiagnosticConfigs ClangDiagnosticConfigsModel::allConfigs() const
{
    return m_diagnosticConfigs;
}

ClangDiagnosticConfigs ClangDiagnosticConfigsModel::customConfigs() const
{
    return Utils::filtered(allConfigs(), [](const ClangDiagnosticConfig &config) {
        return !config.isReadOnly();
    });
}

bool ClangDiagnosticConfigsModel::hasConfigWithId(const Core::Id &id) const
{
    return indexOfConfig(id) != -1;
}

const ClangDiagnosticConfig &ClangDiagnosticConfigsModel::configWithId(const Core::Id &id) const
{
    return m_diagnosticConfigs.at(indexOfConfig(id));
}

QString
ClangDiagnosticConfigsModel::displayNameWithBuiltinIndication(const ClangDiagnosticConfig &config)
{
    return config.isReadOnly()
            ? QCoreApplication::translate("ClangDiagnosticConfigsModel", "%1 [built-in]")
                .arg(config.displayName())
            : config.displayName();
}

QVector<Core::Id> ClangDiagnosticConfigsModel::changedOrRemovedConfigs(
    const ClangDiagnosticConfigs &oldConfigs, const ClangDiagnosticConfigs &newConfigs)
{
    ClangDiagnosticConfigsModel newConfigsModel(newConfigs);
    QVector<Core::Id> changedConfigs;

    for (const ClangDiagnosticConfig &old: oldConfigs) {
        const int i = newConfigsModel.indexOfConfig(old.id());
        if (i == -1)
            changedConfigs.append(old.id()); // Removed
        else if (newConfigsModel.allConfigs().value(i) != old)
            changedConfigs.append(old.id()); // Changed
    }

    return changedConfigs;
}

ClangDiagnosticConfig ClangDiagnosticConfigsModel::createCustomConfig(
    const ClangDiagnosticConfig &config, const QString &displayName)
{
    ClangDiagnosticConfig copied = config;
    copied.setId(Core::Id::fromString(QUuid::createUuid().toString()));
    copied.setDisplayName(displayName);
    copied.setIsReadOnly(false);

    return copied;
}

QStringList ClangDiagnosticConfigsModel::globalDiagnosticOptions()
{
    return {
        // Avoid undesired warnings from e.g. Q_OBJECT
        QStringLiteral("-Wno-unknown-pragmas"),
        QStringLiteral("-Wno-unknown-warning-option"),

        // qdoc commands
        QStringLiteral("-Wno-documentation-unknown-command")
    };
}

int ClangDiagnosticConfigsModel::indexOfConfig(const Core::Id &id) const
{
    return Utils::indexOf(m_diagnosticConfigs, [&](const ClangDiagnosticConfig &config) {
       return config.id() == id;
    });
}

} // namespace CppTools

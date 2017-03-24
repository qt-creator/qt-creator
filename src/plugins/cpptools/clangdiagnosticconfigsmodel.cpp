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

#include "cpptoolsconstants.h"

#include <utils/algorithm.h>

#include <QCoreApplication>

namespace CppTools {

static QStringList commonOptions()
{
    return { QStringLiteral("-Wno-unknown-pragmas") };
}

static void addConfigForQuestionableConstructs(ClangDiagnosticConfigsModel &model)
{
    ClangDiagnosticConfig config;
    config.setId("Builtin.Questionable");
    config.setDisplayName(QCoreApplication::translate("ClangDiagnosticConfigsModel",
                                                      "Warnings for questionable constructs"));
    config.setIsReadOnly(true);
    config.setCommandLineOptions(QStringList{
        QStringLiteral("-Wall"),
        QStringLiteral("-Wextra"),
    } + commonOptions());

    model.appendOrUpdate(config);
}

static void addConfigForPedanticWarnings(ClangDiagnosticConfigsModel &model)
{
    ClangDiagnosticConfig config;
    config.setId("Builtin.Pedantic");
    config.setDisplayName(QCoreApplication::translate("ClangDiagnosticConfigsModel",
                                                      "Pedantic Warnings"));
    config.setIsReadOnly(true);
    config.setCommandLineOptions(QStringList{QStringLiteral("-Wpedantic")} + commonOptions());

    model.appendOrUpdate(config);
}

static void addConfigForAlmostEveryWarning(ClangDiagnosticConfigsModel &model)
{
    ClangDiagnosticConfig config;
    config.setId(Constants::CPP_CLANG_BUILTIN_CONFIG_ID_EVERYTHING_WITH_EXCEPTIONS);
    config.setDisplayName(QCoreApplication::translate("ClangDiagnosticConfigsModel",
                                                      "Warnings for almost everything"));
    config.setIsReadOnly(true);
    config.setCommandLineOptions(QStringList{
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
        QStringLiteral("-Wno-missing-prototypes"), // Not optimal for C projects.
    } + commonOptions());

    model.appendOrUpdate(config);
}

static void addBuiltinConfigs(ClangDiagnosticConfigsModel &model)
{
    addConfigForPedanticWarnings(model);
    addConfigForQuestionableConstructs(model);
    addConfigForAlmostEveryWarning(model);
}

ClangDiagnosticConfigsModel::ClangDiagnosticConfigsModel(const ClangDiagnosticConfigs &customConfigs)
{
    addBuiltinConfigs(*this);

    foreach (const ClangDiagnosticConfig &config, customConfigs)
        m_diagnosticConfigs.append(config);
}

int ClangDiagnosticConfigsModel::size() const
{
    return m_diagnosticConfigs.size();
}

const ClangDiagnosticConfig &ClangDiagnosticConfigsModel::at(int index) const
{
    return m_diagnosticConfigs.at(index);
}

void ClangDiagnosticConfigsModel::prepend(const ClangDiagnosticConfig &config)
{
    m_diagnosticConfigs.prepend(config);
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

ClangDiagnosticConfigs ClangDiagnosticConfigsModel::configs() const
{
    return m_diagnosticConfigs;
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

int ClangDiagnosticConfigsModel::indexOfConfig(const Core::Id &id) const
{
    return Utils::indexOf(m_diagnosticConfigs, [&](const ClangDiagnosticConfig &config) {
       return config.id() == id;
    });
}

} // namespace CppTools

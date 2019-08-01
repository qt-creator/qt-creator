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

#include "clangtidyclazyrunner.h"

#include "clangtoolssettings.h"

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cpptoolsreuse.h>

#include <utils/synchronousprocess.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(LOG, "qtc.clangtools.runner", QtWarningMsg)

using namespace CppTools;

namespace ClangTools {
namespace Internal {

static QStringList commonArguments(const QStringList &options,
                                   const QString &logFile,
                                   const ClangDiagnosticConfig diagnosticConfig)
{
    QStringList arguments;
    if (LOG().isDebugEnabled())
        arguments << QLatin1String("-v");

    const QStringList serializeArgs{"-serialize-diagnostics", logFile};
    if (options.contains("--driver-mode=cl"))
        arguments << clangArgsForCl(serializeArgs);
    else
        arguments << serializeArgs;

    arguments << ClangDiagnosticConfigsModel::globalDiagnosticOptions()
              << diagnosticConfig.clangOptions();

    return arguments;
}

static QStringList tidyPluginArguments(const ClangDiagnosticConfig diagnosticConfig)
{
    QStringList arguments;

    const ClangDiagnosticConfig::TidyMode tidyMode = diagnosticConfig.clangTidyMode();
    if (tidyMode != ClangDiagnosticConfig::TidyMode::Disabled) {
        arguments << XclangArgs({"-add-plugin", "clang-tidy"});
        if (tidyMode != ClangDiagnosticConfig::TidyMode::File) {
            const QString tidyChecks = diagnosticConfig.clangTidyChecks();
            arguments << XclangArgs({"-plugin-arg-clang-tidy", "-checks=" + tidyChecks});
        }
    }

    return arguments;
}

static QStringList clazyPluginArguments(const ClangDiagnosticConfig diagnosticConfig)
{
    QStringList arguments;

    const QString clazyChecks = diagnosticConfig.clazyChecks();
    if (!clazyChecks.isEmpty()) {
        arguments << XclangArgs({"-add-plugin",
                                 "clazy",
                                 "-plugin-arg-clazy",
                                 "enable-all-fixits",
                                 "-plugin-arg-clazy",
                                 "no-autowrite-fixits",
                                 "-plugin-arg-clazy",
                                 diagnosticConfig.clazyChecks()});
    }

    return arguments;
}

ClangTidyRunner::ClangTidyRunner(const ClangDiagnosticConfig &config, QObject *parent)
    : ClangToolRunner(parent)
{
    setName(tr("Clang-Tidy"));
    setArgsCreator([this, config](const QStringList &baseOptions) {
        return commonArguments(baseOptions, m_logFile, config)
            << tidyPluginArguments(config)
            << baseOptions
            << QDir::toNativeSeparators(filePath());
    });
}

ClazyRunner::ClazyRunner(const ClangDiagnosticConfig &config, QObject *parent)
    : ClangToolRunner(parent)
{
    setName(tr("Clazy"));
    setArgsCreator([this, config](const QStringList &baseOptions) {
        return commonArguments(baseOptions, m_logFile, config)
            << clazyPluginArguments(config) << baseOptions
            << QDir::toNativeSeparators(filePath());
    });
}

} // namespace Internal
} // namespace ClangTools

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

namespace ClangTools {
namespace Internal {

ClangTidyClazyRunner::ClangTidyClazyRunner(const CppTools::ClangDiagnosticConfig &diagnosticConfig,
                                           const QString &clangExecutable,
                                           const QString &clangLogFileDir,
                                           const Utils::Environment &environment,
                                           QObject *parent)
    : ClangToolRunner(clangExecutable,
                      clangLogFileDir,
                      environment,
                      tr("Clang-Tidy and Clazy"),
                      parent)
    , m_diagnosticConfig(diagnosticConfig)
{
}

QStringList ClangTidyClazyRunner::constructCommandLineArguments(const QStringList &options)
{
    using namespace CppTools;
    QStringList arguments;

    if (LOG().isDebugEnabled())
        arguments << QLatin1String("-v");

    const QStringList serializeArgs{"-serialize-diagnostics", m_logFile};
    if (options.contains("--driver-mode=cl"))
        arguments << clangArgsForCl(serializeArgs);
    else
        arguments << serializeArgs;

    arguments << ClangDiagnosticConfigsModel::globalDiagnosticOptions()
              << m_diagnosticConfig.clangOptions();

    const ClangDiagnosticConfig::TidyMode tidyMode = m_diagnosticConfig.clangTidyMode();
    if (tidyMode != ClangDiagnosticConfig::TidyMode::Disabled) {
        arguments << XclangArgs({"-add-plugin", "clang-tidy"});
        if (tidyMode != ClangDiagnosticConfig::TidyMode::File) {
            const QString tidyChecks = m_diagnosticConfig.clangTidyChecks();
            arguments << XclangArgs({"-plugin-arg-clang-tidy", "-checks=" + tidyChecks});
        }
    }

    const QString clazyChecks = m_diagnosticConfig.clazyChecks();
    if (!clazyChecks.isEmpty()) {
        arguments << XclangArgs({"-add-plugin",
                                 "clang-lazy",
                                 "-plugin-arg-clang-lazy",
                                 "enable-all-fixits",
                                 "-plugin-arg-clang-lazy",
                                 "no-autowrite-fixits",
                                 "-plugin-arg-clang-lazy",
                                 m_diagnosticConfig.clazyChecks()});
    }

    arguments << options << QDir::toNativeSeparators(filePath());
    return arguments;
}

} // namespace Internal
} // namespace ClangTools

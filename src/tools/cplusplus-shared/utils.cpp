// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "utils.h"

#include <utils/environment.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>

namespace CplusplusToolsUtils {

void executeCommand(const QString &command, const QStringList &arguments, const QString &outputFile,
                    bool verbose)
{
    QTextStream out(stderr);
    if (command.isEmpty()) {
        out << "Error: " << Q_FUNC_INFO << "Got empty command to execute." << Qt::endl;
        exit(EXIT_FAILURE);
    }

    const QString fullCommand = command + QLatin1Char(' ') + arguments.join(QLatin1Char(' '));
    if (verbose)
        out << "Executing: " << fullCommand << Qt::endl;

    QProcess process;
    if (!outputFile.isEmpty())
        process.setStandardOutputFile(outputFile, QIODevice::Truncate);
    process.start(command, arguments);
    if (!process.waitForStarted()) {
        out << QString::fromLatin1("Error: Process \"%1\" did not start within timeout: %2.")
                   .arg(fullCommand, process.errorString())
            << Qt::endl;
        exit(EXIT_FAILURE);
    }
    if (!process.waitForFinished()) {
        if (!verbose)
            out << process.readAll() << Qt::endl;
        out << QString::fromLatin1("Error: Process \"%1\" did not finish within timeout.").arg(fullCommand)
            << Qt::endl;
        exit(EXIT_FAILURE);
    }
    const int exitCode = process.exitCode();
    if (exitCode != 0) {
        out << process.readAllStandardError() << Qt::endl;
        out << QString::fromLatin1("Error: Process \"%1\" finished with non zero exit value %2")
                   .arg(fullCommand, exitCode)
            << Qt::endl;
        exit(EXIT_FAILURE);
    }
}

SystemPreprocessor::SystemPreprocessor(bool verbose)
    : m_verbose(verbose)
{
    m_knownCompilers[Utils::HostOsInfo::withExecutableSuffix("gcc")]
        = QLatin1String("-DCPLUSPLUS_WITHOUT_QT -U__BLOCKS__ -xc++ -E -include");
    m_knownCompilers[Utils::HostOsInfo::withExecutableSuffix("cl")]
        = QLatin1String("/DCPLUSPLUS_WITHOUT_QT /U__BLOCKS__ /TP /E /I . /FI");

    for (const QString &key:m_knownCompilers.keys()) {
        const Utils::FilePath executablePath
            = Utils::Environment::systemEnvironment().searchInPath(key);
        if (!executablePath.isEmpty()) {
            m_compiler = key;
            m_compilerArguments = m_knownCompilers[key].split(QLatin1Char(' '), Qt::SkipEmptyParts);
            m_compilerArguments
                << QDir::toNativeSeparators(QLatin1String(PATH_PREPROCESSOR_CONFIG));
            break;
        }
    }
}

void SystemPreprocessor::check() const
{
    QTextStream out(stderr);
    if (!QFileInfo::exists(QLatin1String(PATH_PREPROCESSOR_CONFIG))) {
        out << QString::fromLatin1("Error: File \"%1\" does not exist.")
                   .arg(QLatin1String(PATH_PREPROCESSOR_CONFIG))
            << Qt::endl;
        exit(EXIT_FAILURE);
    }
    if (m_compiler.isEmpty()) {
        const QString triedCompilers
            = QStringList(m_knownCompilers.keys()).join(QLatin1String(", "));
        out << QString::fromLatin1("Error: No compiler found. Tried %1.").arg(triedCompilers)
            << Qt::endl;
        exit(EXIT_FAILURE);
    }
}

void SystemPreprocessor::preprocessFile(const QString &inputFile, const QString &outputFile) const
{
    check();
    if (!QFileInfo::exists(inputFile)) {
        QTextStream out(stderr);
        out << QString::fromLatin1("Error: File \"%1\" does not exist.").arg(inputFile) << Qt::endl;
        exit(EXIT_FAILURE);
    }
    const QStringList arguments = QStringList(m_compilerArguments)
            << QDir::toNativeSeparators(inputFile);
    executeCommand(m_compiler, arguments, outputFile, m_verbose);
}

} // namespace

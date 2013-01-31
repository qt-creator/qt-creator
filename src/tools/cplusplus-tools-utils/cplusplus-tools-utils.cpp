/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/


#include "cplusplus-tools-utils.h"
#include "environment.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>

namespace CplusplusToolsUtils {

QString portableExecutableName(const QString &executable)
{
#if defined(Q_OS_WIN)
    return executable + QLatin1String(".exe");
#else
    return executable;
#endif
}

void executeCommand(const QString &command, const QStringList &arguments, const QString &outputFile,
                    bool verbose)
{
    QTextStream out(stderr);
    if (command.isEmpty()) {
        out << "Error: " << Q_FUNC_INFO << "Got empty command to execute." << endl;
        exit(EXIT_FAILURE);
    }

    const QString fullCommand = command + QLatin1Char(' ') + arguments.join(QLatin1String(" "));
    if (verbose)
        out << "Executing: " << fullCommand << endl;

    QProcess process;
    if (!outputFile.isEmpty())
        process.setStandardOutputFile(outputFile, QIODevice::Truncate);
    process.start(command, arguments);
    if (!process.waitForStarted()) {
        out << QString::fromLatin1("Error: Process \"%1\" did not start within timeout: %2.")
                .arg(fullCommand, process.errorString())
            << endl;
        exit(EXIT_FAILURE);
    }
    if (!process.waitForFinished()) {
        if (!verbose)
            out << process.readAll() << endl;
        out << QString::fromLatin1("Error: Process \"%1\" did not finish within timeout.")
               .arg(fullCommand)
            << endl;
        exit(EXIT_FAILURE);
    }
    const int exitCode = process.exitCode();
    if (exitCode != 0) {
        out << process.readAllStandardError() << endl;
        out << QString::fromLatin1("Error: Process \"%1\" finished with non zero exit value %2")
            .arg(fullCommand, exitCode) << endl;
        exit(EXIT_FAILURE);
    }
}

SystemPreprocessor::SystemPreprocessor(bool verbose)
    : m_verbose(verbose)
{
    m_knownCompilers[portableExecutableName(QLatin1String("gcc"))]
        = QLatin1String("-DCPLUSPLUS_WITHOUT_QT -U__BLOCKS__ -xc++ -E -include");
    m_knownCompilers[portableExecutableName(QLatin1String("cl"))]
        = QLatin1String("/DCPLUSPLUS_WITHOUT_QT /U__BLOCKS__ /TP /E /I . /FI");

    QMapIterator<QString, QString> i(m_knownCompilers);
    while (i.hasNext()) {
        i.next();
        const QString executablePath
            = Utils::Environment::systemEnvironment().searchInPath(i.key());
        if (!executablePath.isEmpty()) {
            m_compiler = i.key();
            m_compilerArguments = i.value().split(QLatin1String(" "), QString::SkipEmptyParts);
            m_compilerArguments
                << QDir::toNativeSeparators(QLatin1String(PATH_PREPROCESSOR_CONFIG));
            break;
        }
    }
}

void SystemPreprocessor::check() const
{
    QTextStream out(stderr);
    if (!QFile::exists(QLatin1String(PATH_PREPROCESSOR_CONFIG))) {
        out << QString::fromLatin1("Error: File \"%1\" does not exist.")
               .arg(QLatin1String(PATH_PREPROCESSOR_CONFIG))
            << endl;
        exit(EXIT_FAILURE);
    }
    if (m_compiler.isEmpty()) {
        const QString triedCompilers
            = QStringList(m_knownCompilers.keys()).join(QLatin1String(", "));
        out << QString::fromLatin1("Error: No compiler found. Tried %1.").arg(triedCompilers)
            << endl;
        exit(EXIT_FAILURE);
    }
}

void SystemPreprocessor::preprocessFile(const QString &inputFile, const QString &outputFile) const
{
    check();
    if (!QFile::exists(inputFile)) {
        QTextStream out(stderr);
        out << QString::fromLatin1("Error: File \"%1\" does not exist.").arg(inputFile) << endl;
        exit(EXIT_FAILURE);
    }
    const QStringList arguments = QStringList(m_compilerArguments)
            << QDir::toNativeSeparators(inputFile);
    executeCommand(m_compiler, arguments, outputFile, m_verbose);
}

} // namespace

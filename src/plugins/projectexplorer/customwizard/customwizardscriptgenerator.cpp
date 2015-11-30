/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "customwizardscriptgenerator.h"
#include "customwizard.h"
#include "customwizardparameters.h"

#include <utils/hostosinfo.h>

#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QTemporaryFile>
#include <QSharedPointer>

namespace ProjectExplorer {
namespace Internal {

// Parse helper: Determine the correct binary to run:
// Expand to full wizard path if it is relative and located
// in the wizard directory, else assume it can be found in path.
// On Windows, run non-exe files with 'cmd /c'.
QStringList fixGeneratorScript(const QString &configFile, QString binary)
{
    if (binary.isEmpty())
        return QStringList();
    // Expand to full path if it is relative and in the wizard
    // directory, else assume it can be found in path.
    QFileInfo binaryInfo(binary);
    if (!binaryInfo.isAbsolute()) {
        QString fullPath = QFileInfo(configFile).absolutePath();
        fullPath += QLatin1Char('/');
        fullPath += binary;
        const QFileInfo fullPathInfo(fullPath);
        if (fullPathInfo.isFile()) {
            binary = fullPathInfo.absoluteFilePath();
            binaryInfo = fullPathInfo;
        }
    } // not absolute
    QStringList rc(binary);
    if (Utils::HostOsInfo::isWindowsHost()) { // Windows: Cannot run scripts by QProcess, do 'cmd /c'
        const QString extension = binaryInfo.suffix();
        if (!extension.isEmpty() && extension.compare(QLatin1String("exe"),
                                                      Qt::CaseInsensitive) != 0) {
            rc.push_front(QLatin1String("/C"));
            rc.push_front(QString::fromLocal8Bit(qgetenv("COMSPEC")));
            if (rc.front().isEmpty())
                rc.front() = QLatin1String("cmd.exe");
        }
    }
    return rc;
}

// Helper for running the optional generation script.
static bool
    runGenerationScriptHelper(const QString &workingDirectory,
                              const QStringList &script,
                              const QList<GeneratorScriptArgument> &argumentsIn,
                              bool dryRun,
                              const QMap<QString, QString> &fieldMap,
                              QString *stdOut /* = 0 */, QString *errorMessage)
{
    QProcess process;
    const QString binary = script.front();
    QStringList arguments;
    const int binarySize = script.size();
    for (int i = 1; i < binarySize; i++)
        arguments.push_back(script.at(i));

    // Arguments: Prepend 'dryrun' and do field replacement
    if (dryRun)
        arguments.push_back(QLatin1String("--dry-run"));

    // Arguments: Prepend 'dryrun'. Do field replacement to actual
    // argument value to expand via temporary file if specified
    CustomWizardContext::TemporaryFilePtrList temporaryFiles;
    foreach (const GeneratorScriptArgument &argument, argumentsIn) {
        QString value = argument.value;
        const bool nonEmptyReplacements
                = argument.flags & GeneratorScriptArgument::WriteFile ?
                    CustomWizardContext::replaceFields(fieldMap, &value, &temporaryFiles) :
                    CustomWizardContext::replaceFields(fieldMap, &value);
        if (nonEmptyReplacements || !(argument.flags & GeneratorScriptArgument::OmitEmpty))
            arguments.push_back(value);
    }
    process.setWorkingDirectory(workingDirectory);
    if (CustomWizard::verbose())
        qDebug("In %s, running:\n%s\n%s\n", qPrintable(workingDirectory),
               qPrintable(binary),
               qPrintable(arguments.join(QLatin1Char(' '))));
    process.start(binary, arguments);
    if (!process.waitForStarted()) {
        *errorMessage = QString::fromLatin1("Unable to start generator script %1: %2").
                arg(binary, process.errorString());
        return false;
    }
    if (!process.waitForFinished()) {
        *errorMessage = QString::fromLatin1("Generator script %1 timed out").arg(binary);
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        *errorMessage = QString::fromLatin1("Generator script %1 crashed").arg(binary);
        return false;
    }
    if (process.exitCode() != 0) {
        const QString stdErr = QString::fromLocal8Bit(process.readAllStandardError());
        *errorMessage = QString::fromLatin1("Generator script %1 returned %2 (%3)").
                arg(binary).arg(process.exitCode()).arg(stdErr);
        return false;
    }
    if (stdOut) {
        *stdOut = QString::fromLocal8Bit(process.readAllStandardOutput());
        stdOut->remove(QLatin1Char('\r'));
        if (CustomWizard::verbose())
            qDebug("Output: '%s'\n", qPrintable(*stdOut));
    }
    return true;
}

/*!
    Performs the first step in custom wizard script generation.

    Does a dry run of the generation script to get a list of files.
    \sa runCustomWizardGeneratorScript, ProjectExplorer::CustomWizard
*/

Core::GeneratedFiles
    dryRunCustomWizardGeneratorScript(const QString &targetPath,
                                      const QStringList &script,
                                      const QList<GeneratorScriptArgument> &arguments,
                                      const QMap<QString, QString> &fieldMap,
                                      QString *errorMessage)
{
    // Run in temporary directory as the target path may not exist yet.
    QString stdOut;
    if (!runGenerationScriptHelper(QDir::tempPath(), script, arguments, true,
                             fieldMap, &stdOut, errorMessage))
        return Core::GeneratedFiles();
    Core::GeneratedFiles files;
    // Parse the output consisting of lines with ',' separated tokens.
    // (file name + attributes matching those of the <file> element)
    foreach (const QString &line, stdOut.split(QLatin1Char('\n'))) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            Core::GeneratedFile file;
            Core::GeneratedFile::Attributes attributes = Core::GeneratedFile::CustomGeneratorAttribute;
            const QStringList tokens = line.split(QLatin1Char(','));
            const int count = tokens.count();
            for (int i = 0; i < count; i++) {
                const QString &token = tokens.at(i);
                if (i) {
                    if (token == QLatin1String(customWizardFileOpenEditorAttributeC))
                        attributes |= Core::GeneratedFile::OpenEditorAttribute;
                    else if (token == QLatin1String(customWizardFileOpenProjectAttributeC))
                            attributes |= Core::GeneratedFile::OpenProjectAttribute;
                } else {
                    // Token 0 is file name. Wizard wants native names.
                    // Expand to full path if relative
                    const QFileInfo fileInfo(token);
                    const QString fullPath =
                            fileInfo.isAbsolute() ?
                            token :
                            (targetPath + QLatin1Char('/') + token);
                    file.setPath(fullPath);
                }
            }
            file.setAttributes(attributes);
            files.push_back(file);
        }
    }
    if (CustomWizard::verbose()) {
        QDebug nospace = qDebug().nospace();
        nospace << script << " generated:\n";
        foreach (const Core::GeneratedFile &f, files)
            nospace << ' ' << f.path() << f.attributes() << '\n';
    }
    return files;
}

/*!
    Performs the second step in custom wizard script generation by actually
    creating the files.

    In addition to the <file> elements
    that define template files in which macros are replaced, it is possible to have
    a custom wizard call a generation script (specified in the \a generatorscript
    attribute of the <files> element) which actually creates files.
    The command line of the script must follow the convention
    \code
    script [--dry-run] [options]
    \endcode

    Options containing field placeholders are configured in the XML files
    and will be passed with them replaced by their values.

    As \QC needs to know the file names before it actually creates the files to
    do overwrite checking etc., this is  2-step process:
    \list
    \li Determine file names and attributes: The script is called with the
      \c --dry-run option and the field values. It then prints the relative path
      names it intends to create followed by comma-separated attributes
     matching those of the \c <file> element, for example:
     \c myclass.cpp,openeditor
    \li The script is called with the parameters only in the working directory
    and then actually creates the files. If that involves directories, the script
    should create those, too.
    \endlist

    \sa dryRunCustomWizardGeneratorScript, ProjectExplorer::CustomWizard
 */

bool runCustomWizardGeneratorScript(const QString &targetPath,
                                    const QStringList &script,
                                    const QList<GeneratorScriptArgument> &arguments,
                                    const QMap<QString, QString> &fieldMap,
                                    QString *errorMessage)
{
    return runGenerationScriptHelper(targetPath, script, arguments,
                                     false, fieldMap,
                                     0, errorMessage);
}

} // namespace Internal
} // namespace ProjectExplorer

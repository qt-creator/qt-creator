/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "customwizardscriptgenerator.h"
#include "customwizard.h"
#include "customwizardparameters.h" // XML attributes

#include <QtCore/QProcess>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QTemporaryFile>
#include <QtCore/QSharedPointer>

namespace ProjectExplorer {
namespace Internal {

typedef QSharedPointer<QTemporaryFile> TemporaryFilePtr;

// Format pattern for temporary files
static inline QString tempFilePattern()
{
    QString tempPattern = QDir::tempPath();
    if (!tempPattern.endsWith(QLatin1Char('/')))
        tempPattern += QLatin1Char('/');
    tempPattern += QLatin1String("qtcreatorXXXXXX.txt");
    return tempPattern;
}

// Create a temporary file with content
static inline TemporaryFilePtr writeTemporaryFile(const QString &content)
{
    TemporaryFilePtr temporaryFile(new QTemporaryFile(tempFilePattern()));
    if (!temporaryFile->open())
        return TemporaryFilePtr();
    temporaryFile->write(content.toLocal8Bit());
    temporaryFile->close();
    return temporaryFile;
}

// Helper for running the optional generation script.
static bool
    runGenerationScriptHelper(const QString &workingDirectory,
                              QString binary, bool dryRun,
                              const QMap<QString, QString> &fieldMap,
                              QString *stdOut /* = 0 */, QString *errorMessage)
{
    typedef QSharedPointer<QTemporaryFile> TemporaryFilePtr;
    typedef QList<TemporaryFilePtr> TemporaryFilePtrList;
    typedef QMap<QString, QString>::const_iterator FieldConstIterator;

    QProcess process;
    QStringList arguments;
    // Check on the process
    const QFileInfo binaryInfo(binary);
    if (!binaryInfo.isFile()) {
        *errorMessage = QString::fromLatin1("The Generator script %1 does not exist").arg(binary);
        return false;
    }
#ifdef Q_OS_WIN // Windows: Cannot run scripts by QProcess, do 'cmd /c'
    const QString extension = binaryInfo.suffix();
    if (!extension.isEmpty() && extension.compare(QLatin1String("exe"), Qt::CaseInsensitive) != 0) {
        arguments.push_back(QLatin1String("/C"));
        arguments.push_back(binary);
        binary = QString::fromLocal8Bit(qgetenv("COMSPEC"));
        if (binary.isEmpty())
            binary = QLatin1String("cmd.exe");
    }
#endif // Q_OS_WIN
    // Arguments
    if (dryRun)
        arguments << QLatin1String("--dry-run");
    // Turn the field replacement map into a list of arguments "key=value".
    // Pass on free-format-texts as a temporary files indicated by a colon
    // separator "key:filename"
    TemporaryFilePtrList temporaryFiles;

    const FieldConstIterator cend = fieldMap.constEnd();
    for (FieldConstIterator it = fieldMap.constBegin(); it != cend; ++it) {
        const QString &value = it.value();
        // Is a temporary file required?
        const bool passAsTemporaryFile = value.contains(QLatin1Char('\n'));
        if (passAsTemporaryFile) {
            // Create a file and pass on as "key:filename"
            TemporaryFilePtr temporaryFile = writeTemporaryFile(value);
            if (temporaryFile.isNull()) {
                *errorMessage = QString::fromLatin1("Cannot create temporary file");
                return false;
            }
            temporaryFiles.push_back(temporaryFile);
            arguments << (it.key() + QLatin1Char(':') + QDir::toNativeSeparators(temporaryFile->fileName()));
        } else {
            // Normal value "key=value"
            arguments << (it.key() + QLatin1Char('=') + value);
        }
    }
    process.setWorkingDirectory(workingDirectory);
    if (CustomWizard::verbose())
        qDebug("In %s, running:\n%s\n%s\n", qPrintable(workingDirectory),
               qPrintable(binary),
               qPrintable(arguments.join(QString(QLatin1Char(' ')))));
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

// Do a dry run of the generation script to get a list of files
Core::GeneratedFiles
    dryRunCustomWizardGeneratorScript(const QString &targetPath,
                                      const QString &script,
                                      const QMap<QString, QString> &fieldMap,
                                      QString *errorMessage)
{
    // Run in temporary directory as the target path may not exist yet.
    QString stdOut;
    if (!runGenerationScriptHelper(QDir::tempPath(), script, true,
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
                    const QString fullPath = targetPath + QLatin1Char('/') + token;
                    file.setPath(QDir::toNativeSeparators(fullPath));
                }
            }
            file.setAttributes(attributes);
            files.push_back(file);
        }
    }
    if (CustomWizard::verbose())
        foreach(const Core::GeneratedFile &f, files)
            qDebug() << script << " generated: " << f.path() << f.attributes();
    return files;
}

bool runCustomWizardGeneratorScript(const QString &targetPath, const QString &script,
                                    const QMap<QString, QString> &fieldMap, QString *errorMessage)
{
    return runGenerationScriptHelper(targetPath, script, false, fieldMap,
                                     0, errorMessage);
}

} // namespace Internal
} // namespace ProjectExplorer

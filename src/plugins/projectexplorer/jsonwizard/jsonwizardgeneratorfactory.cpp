/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "jsonwizardgeneratorfactory.h"

#include "jsonwizard.h"

#include "../projectexplorerconstants.h"

#include <coreplugin/dialogs/promptoverwritedialog.h>

#include <utils/algorithm.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QStringList>

namespace ProjectExplorer {

// --------------------------------------------------------------------
// JsonWizardGenerator:
// --------------------------------------------------------------------

JsonWizardGenerator::OverwriteResult JsonWizardGenerator::promptForOverwrite(JsonWizard::GeneratorFiles *files,
                                                                             QString *errorMessage)
{
    QStringList existingFiles;
    bool oddStuffFound = false;

    foreach (const JsonWizard::GeneratorFile &f, *files) {
        const QFileInfo fi(f.file.path());
        if (fi.exists() && !(f.file.attributes() & Core::GeneratedFile::ForceOverwrite))
            existingFiles.append(f.file.path());
    }
    if (existingFiles.isEmpty())
        return OverwriteOk;

    // Before prompting to overwrite existing files, loop over files and check
    // if there is anything blocking overwriting them (like them being links or folders).
    // Format a file list message as ( "<file1> [readonly], <file2> [folder]").
    const QString commonExistingPath = Utils::commonPath(existingFiles);
    QString fileNamesMsgPart;
    foreach (const QString &fileName, existingFiles) {
        const QFileInfo fi(fileName);
        if (fi.exists()) {
            if (!fileNamesMsgPart.isEmpty())
                fileNamesMsgPart += QLatin1String(", ");
            const QString namePart = QDir::toNativeSeparators(fileName.mid(commonExistingPath.size() + 1));
            if (fi.isDir()) {
                oddStuffFound = true;
                fileNamesMsgPart += QCoreApplication::translate("ProjectExplorer::JsonWizardGenerator", "%1 [folder]")
                        .arg(namePart);
            } else if (fi.isSymLink()) {
                oddStuffFound = true;
                fileNamesMsgPart += QCoreApplication::translate("ProjectExplorer::JsonWizardGenerator", "%1 [symbolic link]")
                        .arg(namePart);
            } else if (!fi.isWritable()) {
                oddStuffFound = true;
                fileNamesMsgPart += QCoreApplication::translate("ProjectExplorer::JsonWizardGenerator", "%1 [read only]")
                        .arg(namePart);
            }
        }
    }

    if (oddStuffFound) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizardGenerator",
                                                    "The directory %1 contains files which cannot be overwritten:\n%2.")
                .arg(QDir::toNativeSeparators(commonExistingPath)).arg(fileNamesMsgPart);
        return OverwriteError;
    }

    // Prompt to overwrite existing files.
    Core::PromptOverwriteDialog overwriteDialog;

    // Scripts cannot handle overwrite
    overwriteDialog.setFiles(existingFiles);
    foreach (const JsonWizard::GeneratorFile &file, *files)
        if (!file.generator->canKeepExistingFiles())
            overwriteDialog.setFileEnabled(file.file.path(), false);
    if (overwriteDialog.exec() != QDialog::Accepted)
        return OverwriteCanceled;

    const QStringList existingFilesToKeep = overwriteDialog.uncheckedFiles();
    if (existingFilesToKeep.size() == files->size()) // All exist & all unchecked->Cancel.
        return OverwriteCanceled;

    // Set 'keep' attribute in files
    foreach (const QString &keepFile, existingFilesToKeep) {
        JsonWizard::GeneratorFile file
                = Utils::findOr(*files, JsonWizard::GeneratorFile(),
                                [&keepFile](const JsonWizard::GeneratorFile &f)
                                { return f.file.path() == keepFile; });
        if (!file.isValid())
            return OverwriteCanceled;
        file.file.setAttributes(file.file.attributes() | Core::GeneratedFile::KeepExistingFileAttribute);
    }
    return OverwriteOk;
}

bool JsonWizardGenerator::formatFiles(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files, QString *errorMessage)
{
    for (auto i = files->begin(); i != files->end(); ++i) {
        if (!i->generator->formatFile(wizard, &(i->file), errorMessage))
            return false;
    }
    return true;
}

bool JsonWizardGenerator::writeFiles(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files, QString *errorMessage)
{
    for (auto i = files->begin(); i != files->end(); ++i) {
        if (!i->generator->writeFile(wizard, &(i->file), errorMessage))
            return false;
    }
    return true;
}

bool JsonWizardGenerator::postWrite(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files, QString *errorMessage)
{
    for (auto i = files->begin(); i != files->end(); ++i) {
        if (!i->generator->postWrite(wizard, &(i->file), errorMessage))
            return false;
    }
    return true;
}

// --------------------------------------------------------------------
// JsonWizardGeneratorFactory:
// --------------------------------------------------------------------

void JsonWizardGeneratorFactory::setTypeIdsSuffixes(const QStringList &suffixes)
{
    m_typeIds = Utils::transform(suffixes, [](QString suffix)
        { return Core::Id::fromString(QString::fromLatin1(Constants::GENERATOR_ID_PREFIX) + suffix); });
}

void JsonWizardGeneratorFactory::setTypeIdsSuffix(const QString &suffix)
{
    setTypeIdsSuffixes(QStringList() << suffix);
}

} // namespace ProjectExplorer

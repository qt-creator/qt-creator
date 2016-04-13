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

#include "jsonwizardgeneratorfactory.h"

#include "jsonwizard.h"
#include "jsonwizardfilegenerator.h"
#include "jsonwizardscannergenerator.h"

#include "../editorconfiguration.h"
#include "../project.h"
#include "../projectexplorerconstants.h"

#include <coreplugin/dialogs/promptoverwritedialog.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/indenter.h>
#include <texteditor/normalindenter.h>
#include <texteditor/storagesettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/algorithm.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QTextCursor>
#include <QTextDocument>

using namespace Core;
using namespace TextEditor;

namespace ProjectExplorer {

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

static ICodeStylePreferences *codeStylePreferences(Project *project, Id languageId)
{
    if (!languageId.isValid())
        return 0;

    if (project)
        return project->editorConfiguration()->codeStyle(languageId);

    return TextEditorSettings::codeStyle(languageId);
}

// --------------------------------------------------------------------
// JsonWizardGenerator:
// --------------------------------------------------------------------

bool JsonWizardGenerator::formatFile(const JsonWizard *wizard, GeneratedFile *file, QString *errorMessage)
{
    Q_UNUSED(errorMessage);

    if (file->isBinary() || file->contents().isEmpty())
        return true; // nothing to do

    Utils::MimeDatabase mdb;
    Id languageId = TextEditorSettings::languageId(mdb.mimeTypeForFile(file->path()).name());

    if (!languageId.isValid())
        return true; // don't modify files like *.ui, *.pro

    auto baseProject = qobject_cast<Project *>(wizard->property("SelectedProject").value<QObject *>());
    ICodeStylePreferencesFactory *factory = TextEditorSettings::codeStyleFactory(languageId);

    Indenter *indenter = nullptr;
    if (factory)
        indenter = factory->createIndenter();
    if (!indenter)
        indenter = new NormalIndenter();

    ICodeStylePreferences *codeStylePrefs = codeStylePreferences(baseProject, languageId);
    indenter->setCodeStylePreferences(codeStylePrefs);
    QTextDocument doc(file->contents());
    QTextCursor cursor(&doc);
    cursor.select(QTextCursor::Document);
    indenter->indent(&doc, cursor, QChar::Null, codeStylePrefs->currentTabSettings());
    delete indenter;
    if (TextEditorSettings::storageSettings().m_cleanWhitespace) {
        QTextBlock block = doc.firstBlock();
        while (block.isValid()) {
            codeStylePrefs->currentTabSettings().removeTrailingWhitespace(cursor, block);
            block = block.next();
        }
    }
    file->setContents(doc.toPlainText());

    return true;
}

bool JsonWizardGenerator::writeFile(const JsonWizard *wizard, GeneratedFile *file, QString *errorMessage)
{
    Q_UNUSED(wizard);
    Q_UNUSED(file);
    Q_UNUSED(errorMessage);
    return true;
}

bool JsonWizardGenerator::postWrite(const JsonWizard *wizard, GeneratedFile *file, QString *errorMessage)
{
    Q_UNUSED(wizard);
    Q_UNUSED(file);
    Q_UNUSED(errorMessage);
    return true;
}

bool JsonWizardGenerator::polish(const JsonWizard *wizard, GeneratedFile *file, QString *errorMessage)
{
    Q_UNUSED(wizard);
    Q_UNUSED(file);
    Q_UNUSED(errorMessage);
    return true;
}

bool JsonWizardGenerator::allDone(const JsonWizard *wizard, GeneratedFile *file, QString *errorMessage)
{
    Q_UNUSED(wizard);
    Q_UNUSED(file);
    Q_UNUSED(errorMessage);
    return true;
}

JsonWizardGenerator::OverwriteResult JsonWizardGenerator::promptForOverwrite(JsonWizard::GeneratorFiles *files,
                                                                             QString *errorMessage)
{
    QStringList existingFiles;
    bool oddStuffFound = false;

    foreach (const JsonWizard::GeneratorFile &f, *files) {
        const QFileInfo fi(f.file.path());
        if (fi.exists()
                && !(f.file.attributes() & GeneratedFile::ForceOverwrite)
                && !(f.file.attributes() & GeneratedFile::KeepExistingFileAttribute))
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
    PromptOverwriteDialog overwriteDialog;

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
        file.file.setAttributes(file.file.attributes() | GeneratedFile::KeepExistingFileAttribute);
    }
    return OverwriteOk;
}

bool JsonWizardGenerator::formatFiles(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files,
                                      QString *errorMessage)
{
    for (auto i = files->begin(); i != files->end(); ++i) {
        if (!i->generator->formatFile(wizard, &(i->file), errorMessage))
            return false;
    }
    return true;
}

bool JsonWizardGenerator::writeFiles(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files,
                                     QString *errorMessage)
{
    for (auto i = files->begin(); i != files->end(); ++i) {
        if (!i->generator->writeFile(wizard, &(i->file), errorMessage))
            return false;
    }
    return true;
}

bool JsonWizardGenerator::postWrite(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files,
                                    QString *errorMessage)
{
    for (auto i = files->begin(); i != files->end(); ++i) {
        if (!i->generator->postWrite(wizard, &(i->file), errorMessage))
            return false;
    }
    return true;
}

bool JsonWizardGenerator::polish(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files,
                                 QString *errorMessage)
{
    for (auto i = files->begin(); i != files->end(); ++i) {
        if (!i->generator->polish(wizard, &(i->file), errorMessage))
            return false;
    }
    return true;
}

bool JsonWizardGenerator::allDone(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files,
                                  QString *errorMessage)
{
    for (auto i = files->begin(); i != files->end(); ++i) {
        if (!i->generator->allDone(wizard, &(i->file), errorMessage))
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
        { return Id::fromString(QString::fromLatin1(Constants::GENERATOR_ID_PREFIX) + suffix); });
}

void JsonWizardGeneratorFactory::setTypeIdsSuffix(const QString &suffix)
{
    setTypeIdsSuffixes(QStringList() << suffix);
}

// --------------------------------------------------------------------
// FileGeneratorFactory:
// --------------------------------------------------------------------

namespace Internal {

FileGeneratorFactory::FileGeneratorFactory()
{
    setTypeIdsSuffix(QLatin1String("File"));
}

JsonWizardGenerator *FileGeneratorFactory::create(Id typeId, const QVariant &data,
                                                  const QString &path, Id platform,
                                                  const QVariantMap &variables)
{
    Q_UNUSED(path);
    Q_UNUSED(platform);
    Q_UNUSED(variables);

    QTC_ASSERT(canCreate(typeId), return 0);

    auto gen = new JsonWizardFileGenerator;
    QString errorMessage;
    gen->setup(data, &errorMessage);

    if (!errorMessage.isEmpty()) {
        qWarning() << "FileGeneratorFactory setup error:" << errorMessage;
        delete gen;
        return 0;
    }

    return gen;
}

bool FileGeneratorFactory::validateData(Id typeId, const QVariant &data, QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);

    QScopedPointer<JsonWizardFileGenerator> gen(new JsonWizardFileGenerator);
    return gen->setup(data, errorMessage);
}

// --------------------------------------------------------------------
// ScannerGeneratorFactory:
// --------------------------------------------------------------------

ScannerGeneratorFactory::ScannerGeneratorFactory()
{
    setTypeIdsSuffix(QLatin1String("Scanner"));
}

JsonWizardGenerator *ScannerGeneratorFactory::create(Id typeId, const QVariant &data,
                                                     const QString &path, Id platform,
                                                     const QVariantMap &variables)
{
    Q_UNUSED(path);
    Q_UNUSED(platform);
    Q_UNUSED(variables);

    QTC_ASSERT(canCreate(typeId), return 0);

    auto gen = new JsonWizardScannerGenerator;
    QString errorMessage;
    gen->setup(data, &errorMessage);

    if (!errorMessage.isEmpty()) {
        qWarning() << "ScannerGeneratorFactory setup error:" << errorMessage;
        delete gen;
        return 0;
    }

    return gen;
}

bool ScannerGeneratorFactory::validateData(Id typeId, const QVariant &data, QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);

    QScopedPointer<JsonWizardScannerGenerator> gen(new JsonWizardScannerGenerator);
    return gen->setup(data, errorMessage);
}

} // namespace Internal
} // namespace ProjectExplorer

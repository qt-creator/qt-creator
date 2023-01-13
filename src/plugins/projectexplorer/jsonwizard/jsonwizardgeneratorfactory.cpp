// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsonwizardgeneratorfactory.h"

#include "jsonwizard.h"
#include "jsonwizardfilegenerator.h"
#include "jsonwizardscannergenerator.h"

#include "../editorconfiguration.h"
#include "../project.h"
#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"

#include <coreplugin/dialogs/promptoverwritedialog.h>

#include <texteditor/icodestylepreferences.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/storagesettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textindenter.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QTextCursor>
#include <QTextDocument>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace ProjectExplorer {

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

static ICodeStylePreferences *codeStylePreferences(Project *project, Id languageId)
{
    if (!languageId.isValid())
        return nullptr;

    if (project)
        return project->editorConfiguration()->codeStyle(languageId);

    return TextEditorSettings::codeStyle(languageId);
}

// --------------------------------------------------------------------
// JsonWizardGenerator:
// --------------------------------------------------------------------

bool JsonWizardGenerator::formatFile(const JsonWizard *wizard, GeneratedFile *file, QString *errorMessage)
{
    Q_UNUSED(errorMessage)

    if (file->isBinary() || file->contents().isEmpty())
        return true; // nothing to do

    Id languageId = TextEditorSettings::languageId(Utils::mimeTypeForFile(file->filePath()).name());

    if (!languageId.isValid())
        return true; // don't modify files like *.ui, *.pro

    auto baseProject = qobject_cast<Project *>(wizard->property("SelectedProject").value<QObject *>());
    ICodeStylePreferencesFactory *factory = TextEditorSettings::codeStyleFactory(languageId);

    QTextDocument doc(file->contents());
    QTextCursor cursor(&doc);
    Indenter *indenter = nullptr;
    if (factory) {
        indenter = factory->createIndenter(&doc);
        indenter->setFileName(file->filePath());
    }
    if (!indenter)
        indenter = new TextIndenter(&doc);
    ICodeStylePreferences *codeStylePrefs = codeStylePreferences(baseProject, languageId);
    indenter->setCodeStylePreferences(codeStylePrefs);

    cursor.select(QTextCursor::Document);
    indenter->indent(cursor,
                     QChar::Null,
                     codeStylePrefs->currentTabSettings());
    delete indenter;
    if (TextEditorSettings::storageSettings().m_cleanWhitespace) {
        QTextBlock block = doc.firstBlock();
        while (block.isValid()) {
            TabSettings::removeTrailingWhitespace(cursor, block);
            block = block.next();
        }
    }
    file->setContents(doc.toPlainText());

    return true;
}

bool JsonWizardGenerator::writeFile(const JsonWizard *wizard, GeneratedFile *file, QString *errorMessage)
{
    Q_UNUSED(wizard)
    Q_UNUSED(file)
    Q_UNUSED(errorMessage)
    return true;
}

bool JsonWizardGenerator::postWrite(const JsonWizard *wizard, GeneratedFile *file, QString *errorMessage)
{
    Q_UNUSED(wizard)
    Q_UNUSED(file)
    Q_UNUSED(errorMessage)
    return true;
}

bool JsonWizardGenerator::polish(const JsonWizard *wizard, GeneratedFile *file, QString *errorMessage)
{
    Q_UNUSED(wizard)
    Q_UNUSED(file)
    Q_UNUSED(errorMessage)
    return true;
}

bool JsonWizardGenerator::allDone(const JsonWizard *wizard, GeneratedFile *file, QString *errorMessage)
{
    Q_UNUSED(wizard)
    Q_UNUSED(file)
    Q_UNUSED(errorMessage)
    return true;
}

JsonWizardGenerator::OverwriteResult JsonWizardGenerator::promptForOverwrite(JsonWizard::GeneratorFiles *files,
                                                                             QString *errorMessage)
{
    FilePaths existingFiles;
    bool oddStuffFound = false;

    for (const JsonWizard::GeneratorFile &f : std::as_const(*files)) {
        if (f.file.filePath().exists()
                && !(f.file.attributes() & GeneratedFile::ForceOverwrite)
                && !(f.file.attributes() & GeneratedFile::KeepExistingFileAttribute))
            existingFiles.append(f.file.filePath());
    }
    if (existingFiles.isEmpty())
        return OverwriteOk;

    // Before prompting to overwrite existing files, loop over files and check
    // if there is anything blocking overwriting them (like them being links or folders).
    // Format a file list message as ( "<file1> [readonly], <file2> [folder]").
    const QString commonExistingPath = FileUtils::commonPath(existingFiles).toUserOutput();
    const int commonPathSize = commonExistingPath.size();
    QString fileNamesMsgPart;
    for (const FilePath &filePath : std::as_const(existingFiles)) {
        if (filePath.exists()) {
            if (!fileNamesMsgPart.isEmpty())
                fileNamesMsgPart += QLatin1String(", ");
            const QString namePart = filePath.toUserOutput().mid(commonPathSize);
            if (filePath.isDir()) {
                oddStuffFound = true;
                fileNamesMsgPart += Tr::tr("%1 [folder]").arg(namePart);
            } else if (filePath.isSymLink()) {
                oddStuffFound = true;
                fileNamesMsgPart += Tr::tr("%1 [symbolic link]").arg(namePart);
            } else if (!filePath.isWritableDir() && !filePath.isWritableFile()) {
                oddStuffFound = true;
                fileNamesMsgPart += Tr::tr("%1 [read only]").arg(namePart);
            }
        }
    }

    if (oddStuffFound) {
        *errorMessage = Tr::tr("The directory %1 contains files which cannot be overwritten:\n%2.")
                .arg(commonExistingPath).arg(fileNamesMsgPart);
        return OverwriteError;
    }

    // Prompt to overwrite existing files.
    PromptOverwriteDialog overwriteDialog;

    // Scripts cannot handle overwrite
    overwriteDialog.setFiles(existingFiles);
    for (const JsonWizard::GeneratorFile &file : std::as_const(*files))
        if (!file.generator->canKeepExistingFiles())
            overwriteDialog.setFileEnabled(file.file.filePath(), false);
    if (overwriteDialog.exec() != QDialog::Accepted)
        return OverwriteCanceled;

    const QSet<FilePath> existingFilesToKeep = Utils::toSet(overwriteDialog.uncheckedFiles());
    if (existingFilesToKeep.size() == files->size()) // All exist & all unchecked->Cancel.
        return OverwriteCanceled;

    // Set 'keep' attribute in files
    for (JsonWizard::GeneratorFile &file : *files) {
        if (!existingFilesToKeep.contains(file.file.filePath()))
            continue;

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
    Q_UNUSED(path)
    Q_UNUSED(platform)
    Q_UNUSED(variables)

    QTC_ASSERT(canCreate(typeId), return nullptr);

    auto gen = new JsonWizardFileGenerator;
    QString errorMessage;
    gen->setup(data, &errorMessage);

    if (!errorMessage.isEmpty()) {
        qWarning() << "FileGeneratorFactory setup error:" << errorMessage;
        delete gen;
        return nullptr;
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
    Q_UNUSED(path)
    Q_UNUSED(platform)
    Q_UNUSED(variables)

    QTC_ASSERT(canCreate(typeId), return nullptr);

    auto gen = new JsonWizardScannerGenerator;
    QString errorMessage;
    gen->setup(data, &errorMessage);

    if (!errorMessage.isEmpty()) {
        qWarning() << "ScannerGeneratorFactory setup error:" << errorMessage;
        delete gen;
        return nullptr;
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

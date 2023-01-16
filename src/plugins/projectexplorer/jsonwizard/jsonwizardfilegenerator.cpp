// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsonwizardfilegenerator.h"

#include "../projectexplorertr.h"
#include "jsonwizard.h"
#include "jsonwizardfactory.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/macroexpander.h>
#include <utils/templateengine.h>
#include <utils/algorithm.h>

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QVariant>

namespace ProjectExplorer {
namespace Internal {

bool JsonWizardFileGenerator::setup(const QVariant &data, QString *errorMessage)
{
    QTC_ASSERT(errorMessage && errorMessage->isEmpty(), return false);

    const QVariantList list = JsonWizardFactory::objectOrList(data, errorMessage);
    if (list.isEmpty())
        return false;

    for (const QVariant &d : list) {
        if (d.type() != QVariant::Map) {
            *errorMessage = Tr::tr("Files data list entry is not an object.");
            return false;
        }

        File f;

        const QVariantMap tmp = d.toMap();
        f.source = Utils::FilePath::fromSettings(tmp.value(QLatin1String("source")));
        f.target = Utils::FilePath::fromSettings(tmp.value(QLatin1String("target")));
        f.condition = tmp.value(QLatin1String("condition"), true);
        f.isBinary = tmp.value(QLatin1String("isBinary"), false);
        f.overwrite = tmp.value(QLatin1String("overwrite"), false);
        f.openInEditor = tmp.value(QLatin1String("openInEditor"), false);
        f.isTemporary = tmp.value(QLatin1String("temporary"), false);
        f.openAsProject = tmp.value(QLatin1String("openAsProject"), false);

        f.options = JsonWizard::parseOptions(tmp.value(QLatin1String("options")), errorMessage);
        if (!errorMessage->isEmpty())
            return false;

        if (f.source.isEmpty() && f.target.isEmpty()) {
            *errorMessage = Tr::tr("Source and target are both empty.");
            return false;
        }

        if (f.target.isEmpty())
            f.target = f.source;

        m_fileList << f;
    }

    return true;
}

Core::GeneratedFile JsonWizardFileGenerator::generateFile(const File &file,
    Utils::MacroExpander *expander, QString *errorMessage)
{
    // Read contents of source file
    const QFile::OpenMode openMode = file.isBinary.toBool() ?
        QIODevice::ReadOnly : (QIODevice::ReadOnly|QIODevice::Text);

    Utils::FileReader reader;
    if (!reader.fetch(file.source, openMode, errorMessage))
        return Core::GeneratedFile();

    // Generate file information:
    Core::GeneratedFile gf;
    gf.setFilePath(file.target);

    if (!file.keepExisting) {
        if (file.isBinary.toBool()) {
            gf.setBinary(true);
            gf.setBinaryContents(reader.data());
        } else {
            // TODO: Document that input files are UTF8 encoded!
            gf.setBinary(false);
            Utils::MacroExpander nested;

            // evaluate file options once:
            QHash<QString, QString> options;
            for (const JsonWizard::OptionDefinition &od : std::as_const(file.options)) {
                if (od.condition(*expander))
                    options.insert(od.key(), od.value(*expander));
            }

            nested.registerExtraResolver([&options](QString n, QString *ret) -> bool {
                if (!options.contains(n))
                    return false;
                *ret = options.value(n);
                return true;
            });
            nested.registerExtraResolver([expander](QString n, QString *ret) {
                return expander->resolveMacro(n, ret);
            });

            gf.setContents(Utils::TemplateEngine::processText(&nested, QString::fromUtf8(reader.data()),
                                                              errorMessage));
            if (!errorMessage->isEmpty()) {
                *errorMessage = Tr::tr("When processing \"%1\":<br>%2")
                        .arg(file.source.toUserOutput(), *errorMessage);
                return Core::GeneratedFile();
            }
        }
    }

    Core::GeneratedFile::Attributes attributes;
    if (JsonWizard::boolFromVariant(file.openInEditor, expander))
        attributes |= Core::GeneratedFile::OpenEditorAttribute;
    if (JsonWizard::boolFromVariant(file.openAsProject, expander))
        attributes |= Core::GeneratedFile::OpenProjectAttribute;
    if (JsonWizard::boolFromVariant(file.overwrite, expander))
        attributes |= Core::GeneratedFile::ForceOverwrite;
    if (JsonWizard::boolFromVariant(file.isTemporary, expander))
        attributes |= Core::GeneratedFile::TemporaryFile;

    if (file.keepExisting)
        attributes |= Core::GeneratedFile::KeepExistingFileAttribute;

    gf.setAttributes(attributes);
    return gf;
}

Core::GeneratedFiles JsonWizardFileGenerator::fileList(Utils::MacroExpander *expander,
                                                       const Utils::FilePath &wizardDir, const Utils::FilePath &projectDir,
                                                       QString *errorMessage)
{
    errorMessage->clear();

    const QList<File> enabledFiles
            = Utils::filtered(m_fileList, [&expander](const File &f) {
                                  return JsonWizard::boolFromVariant(f.condition, expander);
                              });

    const QList<File> concreteFiles
            = Utils::transform(enabledFiles,
                               [&expander, &wizardDir, &projectDir](const File &f) -> File {
                                  // Return a new file with concrete values based on input file:
                                  File file = f;

                                  file.keepExisting = file.source.isEmpty();
                                  file.target = projectDir.resolvePath(expander->expand(file.target));
                                  file.source = file.keepExisting
                                          ? file.target
                                          : wizardDir.resolvePath(expander->expand(file.source));
                                  file.isBinary = JsonWizard::boolFromVariant(file.isBinary, expander);

                                  return file;
                               });

    QList<File> fileList;
    QList<File> dirList;
    std::tie(fileList, dirList)
            = Utils::partition(concreteFiles, [](const File &f) { return !f.source.isDir(); });

    const QSet<Utils::FilePath> knownFiles = Utils::transform<QSet>(fileList, &File::target);

    for (const File &dir : std::as_const(dirList)) {
        Utils::FilePath sourceDir(dir.source);
        const Utils::FilePaths entries =
                sourceDir.dirEntries(QDir::NoDotAndDotDot | QDir::Files| QDir::Hidden);

        for (const Utils::FilePath &entry : entries) {
            const QString relativeFilePath = entry.relativeChildPath(sourceDir).path();
            const Utils::FilePath targetPath = dir.target / relativeFilePath;

            if (knownFiles.contains(targetPath))
                continue;

            // initialize each new file with properties (isBinary etc)
            // from the current directory json entry
            File newFile = dir;
            newFile.source = dir.source / relativeFilePath;
            newFile.target = targetPath;
            fileList.append(newFile);
        }
    }

    const Core::GeneratedFiles result
            = Utils::transform(fileList,
                               [this, &expander, &errorMessage](const File &f) {
                                   return generateFile(f, expander, errorMessage);
                               });

    if (Utils::contains(result,
                        [](const Core::GeneratedFile &gf) { return gf.filePath().isEmpty(); }))
        return Core::GeneratedFiles();

    return result;
}

bool JsonWizardFileGenerator::writeFile(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage)
{
    Q_UNUSED(wizard)
    if (!(file->attributes() & Core::GeneratedFile::KeepExistingFileAttribute)) {
        if (!file->write(errorMessage))
            return false;
    }
    return true;
}

} // namespace Internal
} // namespace ProjectExplorer

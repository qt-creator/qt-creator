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

#include "jsonwizardfilegenerator.h"

#include "../projectexplorer.h"
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

    QVariantList list = JsonWizardFactory::objectOrList(data, errorMessage);
    if (list.isEmpty())
        return false;

    foreach (const QVariant &d, list) {
        if (d.type() != QVariant::Map) {
            *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                        "Files data list entry is not an object.");
            return false;
        }

        File f;

        const QVariantMap tmp = d.toMap();
        f.source = tmp.value(QLatin1String("source")).toString();
        f.target = tmp.value(QLatin1String("target")).toString();
        f.condition = tmp.value(QLatin1String("condition"), true);
        f.isBinary = tmp.value(QLatin1String("isBinary"), false);
        f.overwrite = tmp.value(QLatin1String("overwrite"), false);
        f.openInEditor = tmp.value(QLatin1String("openInEditor"), false);
        f.openAsProject = tmp.value(QLatin1String("openAsProject"), false);

        f.options = JsonWizard::parseOptions(tmp.value(QLatin1String("options")), errorMessage);
        if (!errorMessage->isEmpty())
            return false;

        if (f.source.isEmpty() && f.target.isEmpty()) {
            *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                        "Source and target are both empty.");
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
    gf.setPath(file.target);

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
            foreach (const JsonWizard::OptionDefinition &od, file.options) {
                if (od.condition(*expander))
                    options.insert(od.key(), od.value(*expander));
            }

            nested.registerExtraResolver([&options](QString n, QString *ret) -> bool {
                if (!options.contains(n))
                    return false;
                *ret = options.value(n);
                return true;
            });
            nested.registerExtraResolver([expander](QString n, QString *ret) { return expander->resolveMacro(n, ret); });

            gf.setContents(Utils::TemplateEngine::processText(&nested, QString::fromUtf8(reader.data()),
                                                              errorMessage));
            if (!errorMessage->isEmpty()) {
                *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard", "When processing \"%1\":<br>%2")
                        .arg(file.source, *errorMessage);
                return Core::GeneratedFile();
            }
        }
    }

    Core::GeneratedFile::Attributes attributes = 0;
    if (JsonWizard::boolFromVariant(file.openInEditor, expander))
        attributes |= Core::GeneratedFile::OpenEditorAttribute;
    if (JsonWizard::boolFromVariant(file.openAsProject, expander))
        attributes |= Core::GeneratedFile::OpenProjectAttribute;
    if (JsonWizard::boolFromVariant(file.overwrite, expander))
        attributes |= Core::GeneratedFile::ForceOverwrite;

    if (file.keepExisting)
        attributes |= Core::GeneratedFile::KeepExistingFileAttribute;

    gf.setAttributes(attributes);
    return gf;
}

Core::GeneratedFiles JsonWizardFileGenerator::fileList(Utils::MacroExpander *expander,
                                                       const QString &wizardDir, const QString &projectDir,
                                                       QString *errorMessage)
{
    errorMessage->clear();

    QDir wizard(wizardDir);
    QDir project(projectDir);

    const QList<File> enabledFiles
            = Utils::filtered(m_fileList, [&expander](const File &f) {
                                  return JsonWizard::boolFromVariant(f.condition, expander);
                              });

    const QList<File> concreteFiles
            = Utils::transform(enabledFiles,
                               [&expander, &wizard, &project](const File &f) -> File {
                                  // Return a new file with concrete values based on input file:
                                  File file = f;

                                  file.keepExisting = file.source.isEmpty();
                                  file.target = project.absoluteFilePath(expander->expand(file.target));
                                  file.source = file.keepExisting ? file.target : wizard.absoluteFilePath(
                                      expander->expand(file.source));
                                  file.isBinary = JsonWizard::boolFromVariant(file.isBinary, expander);

                                  return file;
                               });

    QList<File> fileList;
    QList<File> dirList;
    std::tie(fileList, dirList)
            = Utils::partition(concreteFiles, [](const File &f) { return !QFileInfo(f.source).isDir(); });

    const QSet<QString> knownFiles
            = QSet<QString>::fromList(Utils::transform(fileList, [](const File &f) { return f.target; }));

    foreach (const File &dir, dirList) {
        QDir sourceDir(dir.source);
        QDirIterator it(dir.source, QDir::NoDotAndDotDot | QDir::Files| QDir::Hidden,
                        QDirIterator::Subdirectories);

        while (it.hasNext()) {
            const QString relativeFilePath = sourceDir.relativeFilePath(it.next());
            const QString targetPath = dir.target + QLatin1Char('/') + relativeFilePath;

            if (knownFiles.contains(targetPath))
                continue;

            // initialize each new file with properties (isBinary etc)
            // from the current directory json entry
            File newFile = dir;
            newFile.source = dir.source + QLatin1Char('/') + relativeFilePath;
            newFile.target = targetPath;
            fileList.append(newFile);
        }
    }

    const Core::GeneratedFiles result
            = Utils::transform(fileList,
                               [this, &expander, &errorMessage](const File &f) {
                                   return generateFile(f, expander, errorMessage);
                               });

    if (Utils::contains(result, [](const Core::GeneratedFile &gf) { return gf.path().isEmpty(); }))
        return Core::GeneratedFiles();

    return result;
}

bool JsonWizardFileGenerator::writeFile(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage)
{
    Q_UNUSED(wizard);
    if (!(file->attributes() & Core::GeneratedFile::KeepExistingFileAttribute)) {
        if (!file->write(errorMessage))
            return false;
    }
    return true;
}

} // namespace Internal
} // namespace ProjectExplorer

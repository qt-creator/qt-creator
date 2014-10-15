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
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "jsonwizardfilegenerator.h"

#include "../customwizard/customwizardpreprocessor.h"
#include "../projectexplorer.h"
#include "jsonwizard.h"
#include "jsonwizardfactory.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QCoreApplication>
#include <QDir>
#include <QVariant>

namespace ProjectExplorer {
namespace Internal {

static QString processTextFileContents(Utils::AbstractMacroExpander *expander,
                                       const QString &input, QString *errorMessage)
{
    errorMessage->clear();

    if (input.isEmpty())
        return input;

    QString tmp;
    if (!customWizardPreprocess(Utils::expandMacros(input, expander), &tmp, errorMessage))
        return QString();

    // Expand \n, \t and handle line continuation:
    QString result;
    result.reserve(tmp.count());
    bool isEscaped = false;
    for (int i = 0; i < tmp.count(); ++i) {
        const QChar c = tmp.at(i);

        if (isEscaped) {
            if (c == QLatin1Char('n'))
                result.append(QLatin1Char('\n'));
            else if (c == QLatin1Char('t'))
                result.append(QLatin1Char('\t'));
            else if (c != QLatin1Char('\n'))
                result.append(c);
            isEscaped = false;
        } else {
            if (c == QLatin1Char('\\'))
                isEscaped = true;
            else
                result.append(c);
        }
    }
    return result;
}

bool JsonWizardFileGenerator::setup(const QVariant &data, QString *errorMessage)
{
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

        QVariantMap tmp = d.toMap();
        f.source = tmp.value(QLatin1String("source")).toString();
        f.target = tmp.value(QLatin1String("target")).toString();
        f.condition = tmp.value(QLatin1String("condition"), true);
        f.isBinary = tmp.value(QLatin1String("isBinary"), false);
        f.overwrite = tmp.value(QLatin1String("overwrite"), false);
        f.openInEditor = tmp.value(QLatin1String("openInEditor"), false);
        f.openAsProject = tmp.value(QLatin1String("openAsProject"), false);

        if (f.source.isEmpty()) {
            *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                        "No source given for file in file list.");
            return false;
        }

        if (f.target.isEmpty())
            f.target = f.source;

        m_fileList << f;
    }

    return true;
}

Core::GeneratedFiles JsonWizardFileGenerator::fileList(Utils::AbstractMacroExpander *expander,
                                                       const QString &wizardDir, const QString &projectDir,
                                                       QString *errorMessage)
{
    errorMessage->clear();

    QDir wizard(wizardDir);
    QDir project(projectDir);

    Core::GeneratedFiles result;

    foreach (const File &f, m_fileList) {
        if (!JsonWizard::boolFromVariant(f.condition, expander))
            continue;

        // Read contents of source file
        const QString src = wizard.absoluteFilePath(Utils::expandMacros(f.source, expander));
        const QFile::OpenMode openMode
                = JsonWizard::boolFromVariant(f.isBinary, expander)
                ? QIODevice::ReadOnly : (QIODevice::ReadOnly|QIODevice::Text);

        Utils::FileReader reader;
        if (!reader.fetch(src, openMode, errorMessage))
            return Core::GeneratedFiles();

        // Generate file information:
        Core::GeneratedFile gf;
        gf.setPath(project.absoluteFilePath(Utils::expandMacros(f.target, expander)));

        if (JsonWizard::boolFromVariant(f.isBinary, expander)) {
            gf.setBinary(true);
            gf.setBinaryContents(reader.data());
        } else {
            // TODO: Document that input files are UTF8 encoded!
            gf.setBinary(false);
            gf.setContents(processTextFileContents(expander, QString::fromUtf8(reader.data()), errorMessage));
            if (!errorMessage->isEmpty()) {
                *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard", "When processing \"%1\":<br>%2")
                        .arg(src, *errorMessage);
                return Core::GeneratedFiles();
            }
        }

        Core::GeneratedFile::Attributes attributes = 0;
        if (JsonWizard::boolFromVariant(f.openInEditor, expander))
            attributes |= Core::GeneratedFile::OpenEditorAttribute;
        if (JsonWizard::boolFromVariant(f.openAsProject, expander))
            attributes |= Core::GeneratedFile::OpenProjectAttribute;
        if (JsonWizard::boolFromVariant(f.overwrite, expander))
            attributes |= Core::GeneratedFile::ForceOverwrite;
        gf.setAttributes(attributes);
        result.append(gf);
    }

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

bool JsonWizardFileGenerator::postWrite(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage)
{
    Q_UNUSED(wizard);
    Q_UNUSED(file);
    Q_UNUSED(errorMessage);
    return true;
}

bool JsonWizardFileGenerator::allDone(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage)
{
    Q_UNUSED(wizard);
    if (file->attributes() & Core::GeneratedFile::OpenProjectAttribute) {
        Project *project = ProjectExplorerPlugin::instance()->openProject(file->path(), errorMessage);
        if (!project) {
            *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                        "Failed to open \"%1\" as a project.")
                .arg(QDir::toNativeSeparators(file->path()));

            return false;
        }
    }
    if (file->attributes() & Core::GeneratedFile::OpenEditorAttribute) {
        if (!Core::EditorManager::openEditor(file->path(), file->editorId())) {
            if (errorMessage)
                *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                            "Failed to open an editor for \"%1\".")
                    .arg(QDir::toNativeSeparators(file->path()));
            return false;
        }
    }
    return true;
}

} // namespace Internal
} // namespace ProjectExplorer

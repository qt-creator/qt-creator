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

#include "basefilewizardfactory.h"

#include "basefilewizard.h"
#include "icontext.h"
#include "icore.h"
#include "ifilewizardextension.h"
#include "editormanager/editormanager.h"
#include "dialogs/promptoverwritedialog.h"
#include <extensionsystem/pluginmanager.h>
#include <utils/filewizardpage.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/wizard.h>

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QIcon>

enum { debugWizard = 0 };

namespace Core {

static int indexOfFile(const GeneratedFiles &f, const QString &path)
{
    const int size = f.size();
    for (int i = 0; i < size; ++i)
        if (f.at(i).path() == path)
            return i;
    return -1;
}

/*!
    \class Core::BaseFileWizard
    \brief The BaseFileWizard class implements a generic wizard for
    creating files.

    The following abstract functions must be implemented:
    \list
    \li create(): Called to create the QWizard dialog to be shown.
    \li generateFiles(): Generates file content.
    \endlist

    The behaviour can be further customized by overwriting the virtual function \c postGenerateFiles(),
    which is called after generating the files.

    \sa Core::GeneratedFile, Core::BaseFileWizardParameters, Core::StandardFileWizard
    \sa Core::Internal::WizardEventLoop
*/

Utils::Wizard *BaseFileWizardFactory::runWizardImpl(const QString &path, QWidget *parent,
                                                    Id platform,
                                                    const QVariantMap &extraValues)
{
    QTC_ASSERT(!path.isEmpty(), return 0);

    // Create dialog and run it. Ensure that the dialog is deleted when
    // leaving the func, but not before the IFileWizardExtension::process
    // has been called

    WizardDialogParameters::DialogParameterFlags dialogParameterFlags;

    if (flags().testFlag(ForceCapitalLetterForFileName))
        dialogParameterFlags |= WizardDialogParameters::ForceCapitalLetterForFileName;

    Utils::Wizard *wizard = create(parent, WizardDialogParameters(path, platform,
                                                                  requiredFeatures(),
                                                                  dialogParameterFlags,
                                                                  extraValues));
    QTC_CHECK(wizard);
    return wizard;
}

/*!
    \fn virtual QWizard *Core::BaseFileWizard::create(QWidget *parent,
                                                      const WizardDialogParameters &parameters) const

    Creates the wizard on the \a parent with the \a parameters.
*/

/*!
    \fn virtual Core::GeneratedFiles Core::BaseFileWizard::generateFiles(const QWizard *w,
                                                                         QString *errorMessage) const = 0
    Overwrite to query the parameters from the dialog and generate the files.

    \note This does not generate physical files, but merely the list of
    Core::GeneratedFile.
*/

/*!
    Physically writes files.

    Re-implement (calling the base implementation) to create files with CustomGeneratorAttribute set.
*/

bool BaseFileWizardFactory::writeFiles(const GeneratedFiles &files, QString *errorMessage) const
{
    const GeneratedFile::Attributes noWriteAttributes
        = GeneratedFile::CustomGeneratorAttribute|GeneratedFile::KeepExistingFileAttribute;
    foreach (const GeneratedFile &generatedFile, files)
        if (!(generatedFile.attributes() & noWriteAttributes ))
            if (!generatedFile.write(errorMessage))
                return false;
    return true;
}

/*!
    Overwrite to perform steps to be done after files are actually created.

    The default implementation opens editors with the newly generated files.
*/

bool BaseFileWizardFactory::postGenerateFiles(const QWizard *, const GeneratedFiles &l,
                                              QString *errorMessage) const
{
    return BaseFileWizardFactory::postGenerateOpenEditors(l, errorMessage);
}

/*!
    Opens the editors for the files whose attribute is set accordingly.
*/

bool BaseFileWizardFactory::postGenerateOpenEditors(const GeneratedFiles &l, QString *errorMessage)
{
    foreach (const GeneratedFile &file, l) {
        if (file.attributes() & GeneratedFile::OpenEditorAttribute) {
            if (!EditorManager::openEditor(file.path(), file.editorId())) {
                if (errorMessage)
                    *errorMessage = tr("Failed to open an editor for \"%1\".").arg(QDir::toNativeSeparators(file.path()));
                return false;
            }
        }
    }
    return true;
}

/*!
    Performs an overwrite check on a set of \a files. Checks if the file exists and
    can be overwritten at all, and then prompts the user with a summary.
*/

BaseFileWizardFactory::OverwriteResult BaseFileWizardFactory::promptOverwrite(GeneratedFiles *files,
                                                                QString *errorMessage) const
{
    if (debugWizard)
        qDebug() << Q_FUNC_INFO << files;

    QStringList existingFiles;
    bool oddStuffFound = false;

    static const QString readOnlyMsg = tr("[read only]");
    static const QString directoryMsg = tr("[folder]");
    static const QString symLinkMsg = tr("[symbolic link]");

    foreach (const GeneratedFile &file, *files) {
        const QString path = file.path();
        if (QFileInfo::exists(path))
            existingFiles.append(path);
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
            fileNamesMsgPart += QDir::toNativeSeparators(fileName.mid(commonExistingPath.size() + 1));
            do {
                if (fi.isDir()) {
                    oddStuffFound = true;
                    fileNamesMsgPart += QLatin1Char(' ') + directoryMsg;
                    break;
                }
                if (fi.isSymLink()) {
                    oddStuffFound = true;
                    fileNamesMsgPart += QLatin1Char(' ') + symLinkMsg;
                    break;
            }
                if (!fi.isWritable()) {
                    oddStuffFound = true;
                    fileNamesMsgPart += QLatin1Char(' ') + readOnlyMsg;
                }
            } while (false);
        }
    }

    if (oddStuffFound) {
        *errorMessage = tr("The project directory %1 contains files which cannot be overwritten:\n%2.")
                .arg(QDir::toNativeSeparators(commonExistingPath)).arg(fileNamesMsgPart);
        return OverwriteError;
    }
    // Prompt to overwrite existing files.
    PromptOverwriteDialog overwriteDialog;
    // Scripts cannot handle overwrite
    overwriteDialog.setFiles(existingFiles);
    foreach (const GeneratedFile &file, *files)
        if (file.attributes() & GeneratedFile::CustomGeneratorAttribute)
            overwriteDialog.setFileEnabled(file.path(), false);
    if (overwriteDialog.exec() != QDialog::Accepted)
        return OverwriteCanceled;
    const QStringList existingFilesToKeep = overwriteDialog.uncheckedFiles();
    if (existingFilesToKeep.size() == files->size()) // All exist & all unchecked->Cancel.
        return OverwriteCanceled;
    // Set 'keep' attribute in files
    foreach (const QString &keepFile, existingFilesToKeep) {
        const int i = indexOfFile(*files, keepFile);
        QTC_ASSERT(i != -1, return OverwriteCanceled);
        GeneratedFile &file = (*files)[i];
        file.setAttributes(file.attributes() | GeneratedFile::KeepExistingFileAttribute);
    }
    return OverwriteOk;
}

/*!
    Constructs a file name, adding the \a extension unless \a baseName already has
    one.
*/

QString BaseFileWizardFactory::buildFileName(const QString &path,
                                      const QString &baseName,
                                      const QString &extension)
{
    QString rc = path;
    const QChar slash = QLatin1Char('/');
    if (!rc.isEmpty() && !rc.endsWith(slash))
        rc += slash;
    rc += baseName;
    // Add extension unless user specified something else
    const QChar dot = QLatin1Char('.');
    if (!extension.isEmpty() && !baseName.contains(dot)) {
        if (!extension.startsWith(dot))
            rc += dot;
        rc += extension;
    }
    if (debugWizard)
        qDebug() << Q_FUNC_INFO << rc;
    return rc;
}

/*!
    Returns the preferred suffix for \a mimeType.
*/

QString BaseFileWizardFactory::preferredSuffix(const QString &mimeType)
{
    QString rc;
    Utils::MimeType mt = Utils::mimeTypeForName(mimeType);
    if (mt.isValid())
        rc = mt.preferredSuffix();
    if (rc.isEmpty())
        qWarning("%s: WARNING: Unable to find a preferred suffix for %s.",
                 Q_FUNC_INFO, mimeType.toUtf8().constData());
    return rc;
}

/*!
    \class Core::StandardFileWizard
    \brief The StandardFileWizard class is a convenience class for
    creating one file.

    It uses Utils::FileWizardDialog and introduces a new virtual to generate the
    files from path and name.

    \sa Core::GeneratedFile, Core::BaseFileWizardParameters, Core::BaseFileWizard
*/

} // namespace Core
